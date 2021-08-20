/****************************************************************************
    
    lv2host.cpp - Simple LV2 plugin loader for Elven
    
    Copyright (C) 2006-2007 Lars Luthman <mail@larsluthman.net>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA

****************************************************************************/

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glibmm.h>
#include <turtleparser.hpp>
#include <query.hpp>
#include <namespaces.hpp>
#include <lv2_event_helpers.h>
#include <lv2_ui.h>

#include "lv2host.hpp"
#include "debug.hpp"
#include "midiutils.hpp"


using namespace std;
using namespace PAQ;
using namespace sigc;


std::string LV2Host::m_user_data_bundle(Glib::getenv("HOME") + 
					"/.lv2/elven_user_data.lv2");


LV2Host::LV2Host(const string& uri, unsigned long frame_rate) 
  : m_uri(uri),
    m_rate(frame_rate),
    m_handle(0),
    m_desc(0),
    m_sr_desc(0),
    m_msg_desc(0),
    m_ports_used(0),
    m_midimap(128, -1),
    m_ports_updated(false),
    m_next_free_preset(0) {

  DBG2("Creating user data bundle...");
  if (!create_user_data_bundle())
    DBG2("Failed to create user data bundle");

  DBG2("Creating plugin loader...");
  
  pthread_mutex_init(&m_mutex, 0);
  
  sem_init(&m_notification_sem, 0, 0);
  
  m_urimap_host_desc.callback_data = 0;
  m_urimap_host_desc.uri_to_id = &LV2Host::uri_to_id;
  
  m_event_host_desc.lv2_event_ref = &LV2Host::event_ref;
  m_event_host_desc.lv2_event_unref = &LV2Host::event_unref;
  
  m_context_host_desc.host_handle = this;
  m_context_host_desc.request_run = &LV2Host::request_run;
  
  // get the directories to look in
  vector<string> search_dirs = get_search_dirs();
  
  // iterate over all directories
  scan_manifests(search_dirs, mem_fun(*this, &LV2Host::match_uri));
  
  // if we didn't find it, assume that we were given a partial URI
  // XXX lots of scanning here, we shouldn't need to do 3 passes
  if (m_rdffiles.size() == 0) {
    DBG1("Did not find a complete match, looking for partial match");
    if (scan_manifests(search_dirs, 
		       mem_fun(*this, &LV2Host::match_partial_uri))) {
      DBG2("Found matching URI, re-scanning manifests");
      scan_manifests(search_dirs, mem_fun(*this, &LV2Host::match_uri));
    }
  }
  
  load_plugin();
}
  
  
LV2Host::~LV2Host() {
  if (m_handle) {
    DBG2("Destroying plugin instance");
    if (m_desc->cleanup)
      m_desc->cleanup(m_handle);
    dlclose(m_libhandle);
    sem_destroy(&m_notification_sem);
  }
}


const std::string& LV2Host::get_plugin_uri() const {
  return m_uri;
}


void LV2Host::list_plugins() {
  vector<string> search_dirs = get_search_dirs();
  scan_manifests(search_dirs, &LV2Host::print_uri);
}


void LV2Host::run_main() {
  if (!sem_trywait(&m_notification_sem)) {
    DBG2("Got notification from realtime thread about port change");
    while (!sem_trywait(&m_notification_sem));
    for (unsigned i = 0; i < m_ports.size(); ++i) {
      if (m_ports[i].notify) {
        // XXX this should only happen if the port value has actually changed
        DBG2("Sending port event for output port "<<i);
        signal_port_event(i, sizeof(float), 0, &m_ports[i].old_value);
      }
    }
  }
}


bool LV2Host::is_valid() const {
  return (m_handle != 0);
}


const vector<LV2Port>& LV2Host::get_ports() const {
  assert(m_handle != 0);
  return m_ports;
}


vector<LV2Port>& LV2Host::get_ports() {
  assert(m_handle);
  return m_ports;
}


long LV2Host::get_default_midi_port() const {
  return m_default_midi_port;
}


void LV2Host::activate() {
  assert(m_handle);
  DBG2("Activating plugin instance");
  if (m_desc->activate)
    m_desc->activate(m_handle);
}


void LV2Host::run(unsigned long nframes) {
  
  assert(m_handle);
  
  for (size_t i = 0; i < m_ports.size(); ++i)
    m_desc->connect_port(m_handle, i, m_ports[i].buffer);
  
  if (!pthread_mutex_trylock(&m_mutex)) {
    
    // copy any updated control port values into the port buffers
    if (m_ports_updated) {
      m_ports_updated = false;
      for (unsigned i = 0; i < m_ports.size(); ++i) {
	if (m_ports[i].type == ControlType && 
	    m_ports[i].direction == InputPort) {
	  DBG3("Setting control input "<<i<<" to "<<m_ports[i].value);
	  memcpy(m_ports[i].buffer, &m_ports[i].value, sizeof(float));
	}
      }
    }
    
    // read events from the vector
    for (unsigned i = 0; i < m_midi_events.size(); ++i) {
      if (m_midi_events[i]->written)
	continue;
      Event* e = m_midi_events[i];
      DBG3("Received MIDI event from the main thread for port "
	   <<m_ports[e->port].symbol<<": "
	   <<midi2str(e->event_size, e->data));
      LV2_Event_Buffer* midi = 
	static_cast<LV2_Event_Buffer*>(m_ports[e->port].buffer);
      // XXX DANGER! Can't start from the beginning of the buffer since other 
      // events may have been written to the buffer already!
      LV2_Event_Iterator iter;
      lv2_event_begin(&iter, midi);
      if (!lv2_event_write(&iter, 0, 0, e->type, e->event_size, e->data))
	break;
      e->written = true;
    }
    
    pthread_mutex_unlock(&m_mutex);
  }
  
  m_desc->run(m_handle, nframes);
  
  // send port notifications
  for (unsigned i = 0; i < m_ports.size(); ++i) {
    if (m_ports[i].notify && 
	m_ports[i].old_value != *static_cast<float*>(m_ports[i].buffer)) {
      DBG3("Port "<<i<<" changed - notifying main thread");
      sem_post(&m_notification_sem);
    }
    m_ports[i].old_value = *static_cast<float*>(m_ports[i].buffer);
  }
}


void LV2Host::deactivate() {
  assert(m_handle);
  DBG2("Deactivating the plugin instance");
  if (m_desc->deactivate)
    m_desc->deactivate(m_handle);
}


void LV2Host::set_control(uint32_t index, float value) {
  if (index < m_ports.size() && m_ports[index].type == ControlType &&
      m_ports[index].direction == InputPort) {
    if (m_ports[index].context == AudioContext) {
      pthread_mutex_lock(&m_mutex);
      m_ports[index].value = value;
      m_ports_updated = true;
      pthread_mutex_unlock(&m_mutex);
    }
    else if (m_ports[index].context == MessageContext) {
      m_ports[index].value = value;
      *static_cast<float*>(m_ports[index].buffer) = value;
      message_run();
    }
    signal_port_event(index, sizeof(float), 0, &value);
  }
}


void LV2Host::set_program(unsigned char program) {
  
  DBG2("Switch to program "<<program<<" requested");
  
  std::map<unsigned, LV2Preset>::const_iterator iter = m_presets.find(program);
  
  if (iter != m_presets.end()) {
    
    DBG2("Found preset \""<<iter->second.name
	 <<"\" with program number "<<program);
    
    // tell the GUI that the program has changed
    signal_program_changed(program);
    
    // set all port values in the preset
    const std::map<uint32_t, float>& preset = iter->second.values;
    std::map<uint32_t, float>::const_iterator piter;
    for (piter = preset.begin(); piter != preset.end(); ++piter) {
      if (piter->first < m_ports.size() && 
	  m_ports[piter->first].type == ControlType &&
	  m_ports[piter->first].direction == InputPort)
	set_control(piter->first, piter->second);
    }
    
    // call restore() in the plugin if there are any files in the preset
    if (iter->second.files) {
      DBG2("Preset has data files, restoring");
      if (!restore(const_cast<const LV2SR_File**>(iter->second.files)))
	DBG0("Failed to completely restore preset");
    }
    
  }
}


void LV2Host::save_program(unsigned char program, const char* name) {
  if (program > 127) {
    DBG0("Can not save program with number "<<int(program));
    return;
  }
  LV2Preset preset;
  preset.name = name;
  preset.elven_override = true;
  for (unsigned i = 0; i < m_ports.size(); ++i) {
    if (m_ports[i].type == ControlType && m_ports[i].direction == InputPort)
      preset.values[i] = m_ports[i].value;
  }
  DBG2("Program \""<<name<<"\" added with number "<<int(program));
  m_presets[program] = preset;
  signal_program_added(program, name);
  signal_program_changed(program);
  
  TurtleParser tp;
  RDFData data;
  if (!tp.parse_ttl_file(m_user_data_bundle + "/manifest.ttl", data)) {
    DBG0("Failed to parse user data manifest!");
    DBG0("Will not be able to save user-defined presets");
    return;
  }
  Variable presetfile;
  Namespace pr("<http://ll-plugins.nongnu.org/lv2/presets#>");
  vector<QueryResult> qr = select(presetfile)
    .where(string("<") + m_uri + ">", pr("presetFile"), presetfile)
    .run(data);
  string filename;
  if (qr.size() > 0) {
    filename = qr[0][presetfile]->name;
    filename = filename.substr(8, filename.size() - 9);
    DBG2("User data manifest already had preset file "<<filename);
  }
  else {
    filename = uri_to_preset_filename(m_uri);
    ofstream ofs;
    ofs.open((m_user_data_bundle + "/manifest.ttl").c_str(), 
	     ios_base::out | ios_base::app);
    if (!ofs.good()) {
      DBG0("Could not open user data manifest for writing!");
      DBG0("Will not be able to save user-defined presets");
      return;
    }
    ofs<<"<"<<m_uri<<">"<<endl
       <<"  <http://www.w3.org/2000/01/rdf-schema#seeAlso> <>;"<<endl
       <<"  "<<pr("presetFile")<<" <"<<filename<<">."<<endl;
    filename = m_user_data_bundle + "/" + filename;
  }
  string fullpath = filename;
  DBG2("Opening "<<fullpath);
  ofstream ofs(fullpath.c_str());
  if (!ofs.good()) {
    DBG0("Could not open user preset file for writing!");
    DBG0("Will not be able to save user-defined presets");
    return;
  }
  ofs<<"# Generated by Elven "<<VERSION<<endl
     <<"@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#>."<<endl
     <<"@prefix pr: <http://ll-plugins.nongnu.org/lv2/presets#>."<<endl
     <<endl
     <<"<"<<m_uri<<">"<<endl;
  map<unsigned, LV2Preset>::const_iterator iter;
  bool first = true;
  for (iter = m_presets.begin(); iter != m_presets.end(); ++iter) {
    if (!iter->second.elven_override)
      continue;
    if (first) {
      ofs<<"  pr:preset ";
      first = false;
    }
    else
      ofs<<","<<endl<<endl<<"  ";
    ofs<<"["<<endl
       <<"    rdfs:label \""<<iter->second.name<<"\";"<<endl
       <<"    pr:midiProgram "<<iter->first<<";"<<endl
       <<"    pr:portValues \"";
    map<uint32_t, float>::const_iterator iter2;
    for (iter2 = iter->second.values.begin();
	 iter2 != iter->second.values.end(); ++iter2) {
      ofs<<iter2->first<<':'<<iter2->second<<' ';
    }
    ofs<<"\";"<<endl
       <<"  ]";
  }
  ofs<<"."<<endl;
}


const std::vector<int>& LV2Host::get_midi_map() const {
  return m_midimap;
}


const std::string& LV2Host::get_icon_path() const {
  return m_iconpath;
}


const std::string& LV2Host::get_gui_path() const {
  return m_plugingui;
}


const std::string& LV2Host::get_gui_uri() const {
  return m_guiuri;
}


const std::string& LV2Host::get_bundle_dir() const {
  return m_bundledir;
}


const std::string& LV2Host::get_name() const {
  return m_name;
}


void LV2Host::message_run() {
  if (m_msg_desc)
    m_msg_desc->blocking_run(m_handle, 0);
  else
    DBG2("Plugin has no message context.");
}


void LV2Host::queue_event(uint32_t port, uint16_t type, uint32_t size, 
			  const uint8_t* data) {
  if (!type) {
    DBG0("Trying to queue event of type 0, which is not allowed");
  }
  else if (port < m_ports.size() && m_ports[port].type == MidiType && 
	   m_ports[port].direction == InputPort) {
    DBG2("Queueing type "<<type<<" event for port "<<port);
    pthread_mutex_lock(&m_mutex);
    unsigned i;
    for (i = 0; i < m_midi_events.size(); ++i) {
      if (!m_midi_events[i]->written)
	break;
      delete m_midi_events[i];
    }
    m_midi_events.erase(m_midi_events.begin(), m_midi_events.begin() + i);
    m_midi_events.push_back(new Event(port, type, size, data));
    pthread_mutex_unlock(&m_mutex);
  }
  else
    DBG0("Trying to write event to invalid port "<<port);
}


void LV2Host::queue_events(uint32_t port, const LV2_Event_Buffer* buffer) {
  if (port < m_ports.size() && m_ports[port].type == MidiType &&
      m_ports[port].direction == InputPort) {
    if (m_ports[port].context == MessageContext) {
      m_desc->connect_port(m_handle, port, 
			   const_cast<LV2_Event_Buffer*>(buffer));
      DBG2("Passing entire event buffer to message port");
      message_run();
      m_desc->connect_port(m_handle, port, m_ports[port].buffer);
    }
    else if (m_ports[port].context == AudioContext) {
      LV2_Event_Iterator iter;
      // yes, this const_cast is safe
      lv2_event_begin(&iter, const_cast<LV2_Event_Buffer*>(buffer));
      while (lv2_event_is_valid(&iter)) {
	uint8_t* data;
	LV2_Event* ev = lv2_event_get(&iter, &data);
	queue_event(port, ev->type, ev->size, data);
	lv2_event_increment(&iter);
      }
    }
  }
}


bool LV2Host::save(const std::string& directory, LV2SR_File*** files) {
  if (m_sr_desc && m_sr_desc->save) {
    char* error = m_sr_desc->save(m_handle, directory.c_str(), files);
    if (error) {
      DBG0("Failed to save plugin state: "<<error);
      free(error);
      return false;
    }
    return true;
  }
  return false;
}


bool LV2Host::restore(const LV2SR_File** files) {
  if (m_sr_desc && m_sr_desc->restore) {
    char* error = m_sr_desc->restore(m_handle, files);
    if (error) {
      DBG0("Failed to restore plugin state: "<<error);
      free(error);
      return false;
    }
    return true;
  }
  return false;
}


const std::map<unsigned, LV2Preset>& LV2Host::get_presets() const {
  return m_presets;
}


std::vector<std::string> LV2Host::get_search_dirs() {
  vector<string> search_dirs;
  const char* lv2p_c = getenv("LV2_PATH");
  if (!lv2p_c) {
    DBG1("LV2_PATH is not defined, will search default directories");
    search_dirs.push_back(string(getenv("HOME")) + "/.lv2");
    search_dirs.push_back("/usr/lib/lv2");
    search_dirs.push_back("/usr/local/lib/lv2");
  }
  else {
    string lv2_path = lv2p_c;
    int split;
    while ((split = lv2_path.find(':')) != (int)string::npos) {
      search_dirs.push_back(lv2_path.substr(0, split));
      lv2_path = lv2_path.substr(split + 1);
    }
    search_dirs.push_back(lv2_path);
  }
  
  return search_dirs;
}


bool LV2Host::scan_manifests(const std::vector<std::string>& search_dirs, 
                             scan_callback_t callback) {
  
  bool done = false;
  
  for (unsigned i = 0; i < search_dirs.size(); ++i) {
    
    DBG2("Searching "<<search_dirs[i]);
    
    // open the directory
    DIR* d = opendir(search_dirs[i].c_str());
    if (d == 0) {
      DBG1("Could not open "<<search_dirs[i]<<": "<<strerror(errno));
      continue;
    }
    
    // iterate over all directory entries
    dirent* e;
    while ((e = readdir(d))) {

      std::string plugindir;
      std::string ttlfile;
      std::string library;
      
      vector<QueryResult> qr;

      if (strlen(e->d_name) >= 4) {
        
        // is it named like an LV2 bundle?
        if (strcmp(e->d_name + (strlen(e->d_name) - 4), ".lv2"))
          continue;
        
        // is it actually a directory?
        plugindir = search_dirs[i] + "/" + e->d_name + "/";
        struct stat stat_info;
        if (stat(plugindir.c_str(), &stat_info)) {
          DBG1("Could not get file information about "<<plugindir);
          continue;
        }
        if (!S_ISDIR(stat_info.st_mode)) {
          DBG1(plugindir<<" is not a directory");
          continue;
        }
        
        // execute the callback
        ttlfile = plugindir + "manifest.ttl";
	struct stat buf;
	if (stat(ttlfile.c_str(), &buf) != ENOENT) {
	  if (callback(plugindir)) {
	    done = true;
	    break;
	  }
        }
	
      }
    }
    
    closedir(d);
    
    if (done)
      break;
  }
  
  return done;
}

 
bool LV2Host::match_uri(const string& bundle) {
  
  // parse
  TurtleParser tp;
  RDFData data;
  if (!tp.parse_ttl_file(bundle + "manifest.ttl", data)) {
    DBG1("Could not parse "<<bundle<<"manifest.ttl");
    return false;
  }
  
  // query
  Namespace lv2("<http://lv2plug.in/ns/lv2core#>");
  Variable datafile;
  string uriref = string("<") + m_uri + ">";
  vector<QueryResult> qr = select(datafile)
    .where(uriref, rdfs("seeAlso"), datafile)
    .run(data);
  for (unsigned i = 0; i < qr.size(); ++i) {
    const string& str = qr[i][datafile]->name;
    if (str.size() >= 10 && str.substr(0, 9) == "<file:///") {
      m_rdffiles.push_back(str.substr(8, str.size() - 9));
      DBG2("Found datafile "<<m_rdffiles[m_rdffiles.size() - 1]);
    }
    else
      DBG1("Unknown URI type: "<<qr[i][datafile]->name);
  }
  qr = select(datafile)
    .where(uriref, rdf("type"), lv2("Plugin"))
    .run(data);
  if (qr.size() > 0) {
    DBG2("Found datafile "<<bundle<<"manifest.ttl");
    m_rdffiles.push_back(bundle + "manifest.ttl");
    m_bundle = bundle;
  }

  return false;
}


bool LV2Host::match_partial_uri(const string& bundle) {
  
  // parse
  TurtleParser tp;
  RDFData data;
  if (!tp.parse_ttl_file(bundle + "manifest.ttl", data)) {
    DBG1("Could not parse "<<bundle<<"manifest.ttl");
    return false;
  }
  
  // query
  Namespace lv2("<http://lv2plug.in/ns/lv2core#>");
  Variable uriref;
  vector<QueryResult> qr = select(uriref)
    .where(uriref, rdf("type"), lv2("Plugin"))
    .run(data);
  for (unsigned i = 0; i < qr.size(); ++i) {
    if (qr[i][uriref]->name.find(m_uri) != string::npos) {
      m_uri = qr[i][uriref]->name;
      m_uri = m_uri.substr(1, m_uri.size() - 2);
      DBG2("Found matching URI: "<<m_uri);
      return true;
    }
  }

  return false;
}


bool LV2Host::parse_ports(RDFData& data, const std::string& parent, 
			  const std::string& predicate, PortContext context) {
  
  Namespace lv2("<http://lv2plug.in/ns/lv2core#>");
  Namespace ll("<http://ll-plugins.nongnu.org/lv2/namespace#>");
  Namespace llext("<http://ll-plugins.nongnu.org/lv2/ext/>");
  Namespace ev("<http://lv2plug.in/ns/ext/event#>");

  Variable symbol, index, portclass, porttype, port;
  vector<QueryResult> qr = select(index, symbol, portclass, porttype)
    .where(parent, predicate, port)
    .where(port, lv2("index"), index)
    .where(port, lv2("symbol"), symbol)
    .run(data);
  
  if (qr.size() == 0) {
    DBG1("Can not find any ports for "<<parent);
    return false;
  }
  
  for (unsigned j = 0; j < qr.size(); ++j) {
    
    // index
    size_t p = atoi(qr[j][index]->name.c_str());
    if (p >= m_ports.size())
      m_ports.resize(p + 1);

    ++m_ports_used;
    m_ports[p].context = context;
    
    // symbol
    m_ports[p].symbol = qr[j][symbol]->name;
    DBG2("Found port "<<m_ports[p].symbol<<" with index "<<p);
    
    // direction and type
    m_ports[p].direction = NoDirection;
    m_ports[p].type = NoType;
    vector<QueryResult> qr2 = select(portclass)
      .where(parent, predicate, port)
      .where(port, lv2("index"), qr[j][index]->name)
      .where(port, rdf("type"), portclass)
      .run(data);
    
    for (unsigned k = 0; k < qr2.size(); ++k) {
      string pclass = qr2[k][portclass]->name;
      DBG2(m_ports[p].symbol<<" is an "<<pclass);
      if (pclass == lv2("InputPort"))
	m_ports[p].direction = InputPort;
      else if (pclass == lv2("OutputPort"))
	m_ports[p].direction = OutputPort;
      else if (pclass == lv2("AudioPort"))
	m_ports[p].type = AudioType;
      else if (pclass == ev("EventPort"))
	m_ports[p].type = MidiType;
      else if (pclass == lv2("ControlPort"))
	m_ports[p].type = ControlType;
      else
	DBG1("Unknown port class: "<<pclass);
    }
    
    if (m_ports[p].direction == NoDirection) {
      DBG0("No direction given for port "<<m_ports[p].symbol);
      return false;
    }
    
    if (m_ports[p].type == NoType) {
      DBG0("No known data type given for port "<<m_ports[p].symbol);
      return false;
    }
    
    m_ports[p].default_value = 0;
    m_ports[p].min_value = 0;
    m_ports[p].max_value = 1;
    //m_ports[p].notify = (m_ports[p].direction == InputPort &&
    //			   m_ports[p].type == ControlType);
    m_ports[p].notify = (m_ports[p].type == ControlType);
    
  }
  
  // default values
  Variable default_value;
  qr = select(index, default_value)
    .where(parent, predicate, port)
    .where(port, lv2("index"), index)
    .where(port, lv2("default"), default_value)
    .run(data);
  for (unsigned j = 0; j < qr.size(); ++j) {
    unsigned p = atoi(qr[j][index]->name.c_str());
    if (p >= m_ports.size())
      return false;
    m_ports[p].default_value = 
      atof(qr[j][default_value]->name.c_str());
  }
  
  // minimum values
  Variable min_value;
  qr = select(index, min_value)
    .where(parent, predicate, port)
    .where(port, lv2("index"), index)
    .where(port, lv2("minimum"), min_value)
    .run(data);
  for (unsigned j = 0; j < qr.size(); ++j) {
    unsigned p = atoi(qr[j][index]->name.c_str());
    if (p >= m_ports.size())
      return false;
    m_ports[p].min_value = 
      atof(qr[j][min_value]->name.c_str());
  }
    
  // maximum values
  Variable max_value;
  qr = select(index, max_value)
    .where(parent, predicate, port)
    .where(port, lv2("index"), index)
    .where(port, lv2("maximum"), max_value)
    .run(data);
  for (unsigned j = 0; j < qr.size(); ++j) {
    unsigned p = atoi(qr[j][index]->name.c_str());
    if (p >= m_ports.size())
      return false;
    m_ports[p].max_value = 
      atof(qr[j][max_value]->name.c_str());
    DBG2(m_ports[p].symbol<<" has the range ["<<m_ports[p].min_value
	 <<", "<<m_ports[p].max_value<<"]");
  }
  
  // check range sanity
  for (unsigned long i = 0; i < m_ports.size(); ++i) {
    if (m_ports[i].direction == OutputPort)
      continue;
    if (m_ports[i].min_value > m_ports[i].default_value)
      m_ports[i].min_value = m_ports[i].default_value;
    if (m_ports[i].max_value < m_ports[i].default_value)
      m_ports[i].max_value = m_ports[i].default_value;
    if (m_ports[i].max_value - m_ports[i].min_value <= 0)
      m_ports[i].max_value = m_ports[i].min_value + 10;
  }
  
  return true;
}


bool LV2Host::load_plugin() {
  
  DBG2(__PRETTY_FUNCTION__);

  map<string, bool> supported_features;
  supported_features[LV2_SAVERESTORE_URI] = false;
  supported_features[LV2_URI_MAP_URI] = false;
  supported_features[LV2_CONTEXT_URI] = false;

  bool uses_message_context = false;
  
  // parse the datafile to get port info
  {
    
    vector<QueryResult> qr;
    string uriref = string("<") + m_uri + ">";
    
    // parse all data files
    RDFData data;
    TurtleParser tp;
    for (unsigned i = 0; i < m_rdffiles.size(); ++i) {
      DBG2("Parsing "<<m_rdffiles[i]);
      if (!tp.parse_ttl_file(m_rdffiles[i], data)) {
	DBG0("Failed to parse "<<m_rdffiles[i]);
	return false;
      }
    }
    
    Namespace lv2("<http://lv2plug.in/ns/lv2core#>");
    Namespace ll("<http://ll-plugins.nongnu.org/lv2/namespace#>");
    Namespace llext("<http://ll-plugins.nongnu.org/lv2/ext/>");
    Namespace ev("<http://lv2plug.in/ns/ext/event#>");
    Namespace mm("<http://ll-plugins.nongnu.org/lv2/ext/midimap#>");
    Variable name, license, binary;
    qr = select(name, binary)
      .where(uriref, rdf("type"), lv2("Plugin"))
      .where(uriref, lv2("binary"), binary)
      .where(uriref, doap("name"), name)
      .where(uriref, doap("license"), license)
      .run(data);
    if (qr.size() > 0) {
      m_name = qr[0][name]->name;
      m_binary = qr[0][binary]->name;
    }
    else {
      DBG0("A required property (binary, name or license) is missing");
      return false;
    }
    
    if (!parse_ports(data, uriref, lv2("port"), AudioContext))
      return false;
    
    // MIDI controller bindings
    Variable port, index, controller, cc_number;
    qr = select(index, cc_number)
      .where(uriref, lv2("port"), port)
      .where(port, lv2("index"), index)
      .where(port, mm("defaultMidiController"), controller)
      .where(controller, mm("controllerType"), mm("CC"))
      .where(controller, mm("controllerNumber"), cc_number)
      .run(data);
    for (unsigned j = 0; j < qr.size(); ++j) {
      unsigned p = atoi(qr[j][index]->name.c_str());
      unsigned cc = atoi(qr[j][cc_number]->name.c_str());
      if (p < m_ports.size() && cc < 128)
        m_midimap[cc] = p;
    }
    
    // default MIDI port
    qr = select(index)
      .where(uriref, mm("defaultMidiPort"), index)
      .run(data);
    if (qr.size() > 0)
      m_default_midi_port = atoi(qr[0][index]->name.c_str());
    else {
      m_default_midi_port = -1;
      for (unsigned long i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].type == MidiType && m_ports[i].direction == InputPort) {
          m_default_midi_port = i;
          break;
        }
      }
    }
    
    // check for features
    Variable feature;
    qr = select(feature)
      .where(uriref, lv2("optionalFeature"), feature)
      .run(data);
    for (size_t i = 0; i < qr.size(); ++i) {
      map<string, bool>::iterator iter;
      string uri = 
	qr[i][feature]->name.substr(1, qr[i][feature]->name.length() - 2);
      iter = supported_features.find(uri);
      if (iter == supported_features.end()) {
	DBG1("The optional feature "<<uri<<" is not supported by Elven.");
      }
      else {
	DBG2("The plugin supports the feature "<<uri);
	iter->second = true;
      }
    }
    qr = select(feature)
      .where(uriref, lv2("requiredFeature"), feature)
      .run(data);
    for (size_t i = 0; i < qr.size(); ++i) {
      map<string, bool>::iterator iter;
      string uri = 
	qr[i][feature]->name.substr(1, qr[i][feature]->name.length() - 2);
      iter = supported_features.find(uri);
      if (iter == supported_features.end()) {
	DBG0("The required feature "<<uri<<" is not supported by Elven.");
	return false;
      }
      else {
	DBG2("The plugin supports the feature "<<iter->first);
	iter->second = true;
      }
    }
    
    // if the context extension is supported, check for contexts
    if (supported_features[LV2_CONTEXT_URI]) {
      Variable cx_pred, context, cx_class;
      Namespace cx("<" LV2_CONTEXT_URI "#>");
      qr = select(cx_pred, context, cx_class)
	.where(uriref, cx_pred, context)
	.where(context, rdf("type"), cx_class)
	.filter(or_filter(cx_pred == cx("optionalContext"),
			  cx_pred == cx("requiredContext")))
	.run(data);
      
      if (qr.size() == 0) {
	DBG0("The plugin requires the context feature but has no additional "
	     "contexts defined");
	return false;
      }
      
      for (size_t i = 0; i < qr.size(); ++i) {
	if (qr[i][cx_class]->name != "<" LV2_CONTEXT_MESSAGE ">") {
	  if (qr[i][cx_pred]->name == cx("requiredContext")) {
	    DBG0("The required context "<<qr[i][cx_class]->name
		 <<" is not supported by Elven");
	    return false;
	  }
	  else
	    DBG1("The optional context "<<qr[i][cx_class]->name
		 <<" is not supported by Elven");
	}
	else {
	  DBG2("The plugin supports the message context");
	  uses_message_context = true;
	  if (!parse_ports(data, qr[i][context]->name, 
			   cx("port"), MessageContext))
	    return false;
	}
      }
      
    }
    
    
    // GUI plugin path
    Variable gui_uri, gui_path;
    Namespace gg("<" LV2_UI_URI "#>");
    qr = select(gui_uri, gui_path)
      .where(uriref, gg("ui"), gui_uri)
      .where(gui_uri, gg("binary"), gui_path)
      .where(gui_uri, rdf("type"), gg("GtkUI"))
      .run(data);
    if (qr.size() > 0) {
      m_guiuri = qr[0][gui_uri]->name.
	substr(1, qr[0][gui_uri]->name.length() - 2);
      m_plugingui = qr[0][gui_path]->name;
      m_plugingui = m_plugingui.substr(8, m_plugingui.size() - 9);
      DBG2("Found GUI plugin file "<<m_plugingui);
      Variable req;
      qr = select(req)
	.where(qr[0][gui_uri]->name, gg("requiredFeature"), req)
	.filter(req != gg("makeResident"))
	.filter(req != gg("makeSONameResident"))
	.filter(req != gg("Events"))
	.run(data);
      if (qr.size() > 0) {
	m_guiuri = "";
	m_plugingui = "";
	DBG2("GUI requires unsupported feature "<<qr[0][req]->name);
      }
    }
    else
      DBG2("Plugin has no GUI");
    
    // icon path
    Variable icon_path;
    qr = select(icon_path)
      .where(uriref, ll("svgIcon"), icon_path)
      .run(data);
    if (qr.size() > 0) {
      m_iconpath = qr[0][icon_path]->name;
      m_iconpath = m_iconpath.substr(8, m_iconpath.size() - 9);
      DBG2("Found SVG icon file "<<m_iconpath);
    }
    
    m_bundledir = m_bundle;
    
    // preset files
    Variable preset_path;
    Namespace pr("<http://ll-plugins.nongnu.org/lv2/presets#>");
    qr = select(preset_path)
      .where(uriref, pr("presetFile"), preset_path)
      .run(data);
    // XXX this can be done faster
    for (int pf = 0; pf < qr.size(); ++pf) {
      string presetfile = qr[pf][preset_path]->name;
      if (presetfile.substr(8, m_user_data_bundle.size()) == m_user_data_bundle)
	load_presets_from_uri(presetfile, true);
    }
    for (int pf = 0; pf < qr.size(); ++pf) {
      string presetfile = qr[pf][preset_path]->name;
      if (presetfile.substr(8, m_user_data_bundle.size()) != m_user_data_bundle)
	load_presets_from_uri(presetfile, false);
    }
    merge_presets();
  }
  
  // if we got this far the data is OK. time to load the library
  m_libhandle = dlopen(m_binary.substr(8, m_binary.size() - 9).c_str(), 
		       RTLD_NOW);
  if (!m_libhandle) {
    DBG0("Could not dlopen "<<m_binary<<": "<<dlerror());
    return false;
  }
  
  // get the descriptor
  LV2_Descriptor_Function dfunc = get_symbol<LV2_Descriptor_Function>("lv2_descriptor");
  if (!dfunc) {
    DBG0(m_binary<<" has no LV2 descriptor function");
    dlclose(m_libhandle);
    return false;
  }
  for (unsigned long j = 0; (m_desc = dfunc(j)); ++j) {
    if (m_uri == m_desc->URI)
      break;
  }
  if (!m_desc) {
    DBG0(m_binary<<" does not contain the plugin "<<m_uri);
    dlclose(m_libhandle);
    return false;
  }
  
  // get the save/restore descriptor (if there is one)
  if (supported_features[LV2_SAVERESTORE_URI]) {
    if (m_desc->extension_data)
      m_sr_desc = (LV2SR_Descriptor*)(m_desc->
				      extension_data(LV2_SAVERESTORE_URI));
    if (!m_sr_desc) {
      DBG0("The plugin does not use the save/restore extension like the RDF "
	   <<"said it should");
      return false;
    }
  }
  
  // get the message context descriptor (if there is one)
  if (uses_message_context) {
    if (m_desc->extension_data)
      m_msg_desc = (LV2_Blocking_Context*)(m_desc->
					   extension_data(LV2_CONTEXT_MESSAGE));
    if (!m_msg_desc) {
      DBG0("The plugin does not use have a message context like the RDF "
	   <<"said it should");
      return false;
    }
  }
  
  // instantiate the plugin
  LV2_Feature urimap_feature = { LV2_URI_MAP_URI, &m_urimap_host_desc };
  LV2_Feature saverestore_feature = { LV2_SAVERESTORE_URI, 0 };
  LV2_Feature event_feature = { LV2_EVENT_URI, &m_event_host_desc };
  LV2_Feature context_feature = { LV2_CONTEXT_URI, &m_context_host_desc };
  LV2_Feature message_feature = { LV2_CONTEXT_MESSAGE, 0 };
  const LV2_Feature* features[] = { &urimap_feature,
				    &saverestore_feature,
				    &event_feature,
				    &context_feature,
				    &message_feature,
				    0 };
  m_handle = m_desc->instantiate(m_desc, m_rate, m_bundle.c_str(), features);
  
  if (!m_handle) {
    DBG0("Could not instantiate the plugin");
    dlclose(m_libhandle);
    m_libhandle = 0;
    m_desc = 0;
    return false;
  }
  
  return true;
}


bool LV2Host::print_uri(const string& bundle) {
  
  // parse
  TurtleParser tp;
  RDFData data;
  if (!tp.parse_ttl_file(bundle + "manifest.ttl", data)) {
    DBG1("Could not parse "<<bundle<<"manifest.ttl");
    return false;
  }
  
  // query
  Namespace lv2("<http://lv2plug.in/ns/lv2core#>");
  Variable uriref;
  vector<QueryResult> qr = select(uriref)
    .where(uriref, rdf("type"), lv2("Plugin"))
    .run(data);
  for (unsigned i = 0; i < qr.size(); ++i)
    cout<<qr[i][uriref]->name<<endl;

  return false;
}


uint32_t LV2Host::uri_to_id(LV2_URI_Map_Callback_Data callback_data,
			    const char* umap, const char* uri) {
  if (umap && !strcmp(umap, LV2_EVENT_URI)) {
    if (!strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent"))
      return 1;
    else if (!strcmp(uri, "http://lv2plug.in/ns/ext/osc#OscEvent"))
      return 2;
  }
  return 0;
}


uint32_t LV2Host::event_ref(LV2_Event_Callback_Data, LV2_Event* ev) {
  DBG2("Reference count for event "<<ev<<" increased");
  return 0;
}


uint32_t LV2Host::event_unref(LV2_Event_Callback_Data, LV2_Event* ev) {
  DBG2("Reference count for event "<<ev<<" decreased");
  return 0;
}


void LV2Host::request_run(void* host_handle, const char* context_uri) {
  LV2Host* me = static_cast<LV2Host*>(host_handle);
  if (string(context_uri) == LV2_CONTEXT_MESSAGE)
    DBG0("Message context is not yet fully supported");
  else
    DBG1("Asked to run unknown context "<<context_uri);
}


bool LV2Host::add_preset(const LV2Preset& preset, int program) {
  if (program < 0 || m_presets.find(program) != m_presets.end())
    m_tmp_presets.push_back(preset);
  else
    m_presets.insert(make_pair(program, preset));
  return true;
}


void LV2Host::load_presets_from_uri(const std::string& presetfile, bool user) {

  DBG2("Found preset file "<<presetfile);
  
  string uriref = string("<") + m_uri + ">";
  TurtleParser preset_parser;
  RDFData preset_data;
  if (!preset_parser.parse_ttl_url(presetfile, preset_data)) {
    DBG0("Could not parse presets from "<<presetfile);
    return;
  }
  
  // XXX make this work for presets without MIDI program mapping
  Variable preset, name, program;
  Namespace pr("<http://ll-plugins.nongnu.org/lv2/presets#>");
  vector<QueryResult> qr2 = select(preset, name, program)
    .where(uriref, pr("preset"), preset)
    .where(preset, rdfs("label"), name)
    .where(preset, pr("midiProgram"), program)
    .run(preset_data);
  
  // get all presets
  for (unsigned j = 0; j < qr2.size(); ++j) {
    
    LV2Preset tmp_p;
    tmp_p.elven_override = user;
    
    string preseturi = qr2[j][preset]->name;
    DBG2("Found the preset \""<<qr2[j][name]->name<<"\" "
	 <<" with MIDI program number "<<qr2[j][program]->name);
    int pnum = atoi(qr2[j][program]->name.c_str());
    tmp_p.name = qr2[j][name]->name;
    tmp_p.values.clear();
    // XXX should free any old .files here
    
    // get all port values for this preset
    Variable pv;
    vector<QueryResult> qr3 = select(pv)
      .where(preseturi, pr("portValues"), pv)
      .run(preset_data);
    if (qr3.size() > 0) {
      istringstream iss(qr3[0][pv]->name);
      int p;
      float v;
      char c;
      while (!iss.eof()) {
	iss>>p>>c>>v>>ws;
	if (p >= 0)
	  tmp_p.values[p] = v;
      }
    }
    
    // get all data files for this preset
    Variable fn, name, path;
    qr3 = select(name, path)
      .where(preseturi, pr("hasFile"), fn)
      .where(fn, pr("fileName"), name)
      .where(pv, pr("filePath"), path)
      .run(preset_data);
    if (qr3.size() > 0) {
      tmp_p.files = 
	(LV2SR_File**)calloc(qr3.size() + 1, sizeof(LV2SR_File*));
      for (unsigned k = 0; k < qr3.size(); ++k) {
	std::string fname = qr3[k][name]->name.c_str();
	std::string fpath = qr3[k][path]->name.c_str();
	fpath = fpath.substr(8, fpath.size() - 9); 
	
	LV2SR_File* file = (LV2SR_File*)calloc(1, sizeof(LV2SR_File));
	file->name = strdup(fname.c_str());
	file->path = strdup(fpath.c_str());
	tmp_p.files[k] = file;
      }
    }
    
    add_preset(tmp_p, pnum);
  }  
}


void LV2Host::merge_presets() {
  for (unsigned i = 0; i < m_tmp_presets.size(); ++i) {
    // XXX this can be optimised
    while (true) {
      if (m_presets.find(m_next_free_preset) == m_presets.end())
	break;
      ++m_next_free_preset;
    }
    m_presets.insert(make_pair(m_next_free_preset, m_tmp_presets[i]));
  }
  m_tmp_presets.clear();
}


bool LV2Host::create_user_data_bundle() {
  
  // XXX this is horrible and unsafe - what if someone moves or changes
  // the directories while we're working in them?
  
  // create the ~/.lv2/ directory
  int result = mkdir((Glib::get_home_dir() + "/.lv2").c_str(),
		     S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (result != 0 && errno != EEXIST) {
    perror("Failed to create ~/.lv2 directory");
    return false;
  }
  
  // create the Elven user data bundle
  result = mkdir(m_user_data_bundle.c_str(), 
		     S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if (result != 0 && errno != EEXIST) {
    perror("Failed to create user data directory");
    return false;
  }
  
  // create the manifest if it doesn't exist already
  struct stat stat_info;
  result = stat((m_user_data_bundle + "/manifest.ttl").c_str(), &stat_info);
  int old_errno = errno;
  if (result != 0 && errno != ENOENT) {
    perror("Failed to check user data bundle manifest");
    return false;
  }
  if (old_errno == ENOENT) {
    ofstream ofs((m_user_data_bundle + "/manifest.ttl").c_str());
    if (!ofs.good()) {
      cerr<<"Failed to create user data bundle manifest"<<endl;
      return false;
    }
    ofs<<"# Elven user data bundle"<<endl;
  }
  else {
    DBG2("Manifest file already existed");
  }
  return true;
}


string LV2Host::uri_to_preset_filename(const string& uri) {
  string result = uri;
  for (unsigned i = 0; i < result.size(); ++i) {
    char c = result[i];
    if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9'))
      result[i] = '_';
  }
  result += ".ttl";
  return result;
}
