/****************************************************************************
    
    main.cpp - Main source file for the LV2 host Elven
    
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

#include <clocale>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h>
#include <sstream>

#include <sys/wait.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <gtkmm.h>

#include "lv2guihost.hpp"
#include "lv2host.hpp"
#include <lv2_event_helpers.h>
#include "debug.hpp"
#include "midiutils.hpp"


using namespace std;
using namespace sigc;


vector<jack_port_t*> jack_ports;
jack_client_t* jack_client;
bool still_running;


void autoconnect(jack_client_t* client) {
  
  const char* env;
  const char** port_list;
  const char** our_ports;
  const char* name = jack_get_client_name(client);
  
  // MIDI input
  if ((env = getenv("ELVEN_MIDI_INPUT"))) {
    our_ports = jack_get_ports(client, (string(name) + ":*").c_str(),
			       JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
    if (our_ports && our_ports[0]) {
      
      // if it's a client, connect individual ports
      if (index(env, ':') == NULL &&
	  (port_list = jack_get_ports(client, (string(env) + ":*").c_str(),
				      JACK_DEFAULT_MIDI_TYPE, 
				      JackPortIsOutput)) && port_list[0]) {
	for (int i = 0; port_list[i] && our_ports[i]; ++i)
	  jack_connect(client, port_list[i], our_ports[i]);
	free(port_list);
      }
      
      // if not, connect all our ports to that single port
      else {
	for (int i = 0; our_ports[i]; ++i)
	  jack_connect(client, env, our_ports[i]);
      }
      
    }
    free(our_ports);
  }
    
  // audio input
  if ((env = getenv("ELVEN_AUDIO_INPUT"))) {
    our_ports = jack_get_ports(client, (string(name) + ":*").c_str(),
			       JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    if (our_ports && our_ports[0]) {
      
      // if it's a client, connect individual ports
      if (index(env, ':') == NULL &&
	  (port_list = jack_get_ports(client, (string(env) + ":*").c_str(),
				      JACK_DEFAULT_AUDIO_TYPE, 
				      JackPortIsOutput)) && port_list[0]) {
	for (int i = 0; port_list[i] && our_ports[i]; ++i)
	  jack_connect(client, port_list[i], our_ports[i]);
	free(port_list);
      }
      
      // if not, connect all our ports to that single port
      else {
	for (int i = 0; our_ports[i]; ++i)
	  jack_connect(client, env, our_ports[i]);
      }
      
    }
    free(our_ports);
  }
  
  // MIDI output
  if ((env = getenv("ELVEN_MIDI_OUTPUT"))) {
    our_ports = jack_get_ports(client, (string(name) + ":*").c_str(),
			       JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
    if (our_ports && our_ports[0]) {
      
      // if it's a client, connect individual ports
      if (index(env, ':') == NULL &&
	  (port_list = jack_get_ports(client, (string(env) + ":*").c_str(),
				      JACK_DEFAULT_MIDI_TYPE, 
				      JackPortIsInput)) && port_list[0]) {
	for (int i = 0; port_list[i] && our_ports[i]; ++i)
	  jack_connect(client, our_ports[i], port_list[i]);
	free(port_list);
      }
      
      // if not, connect all our ports to that single port
      else {
	for (int i = 0; our_ports[i]; ++i)
	  jack_connect(client, our_ports[i], env);
      }
      
    }
    free(our_ports);
  }
    
  // audio output
  if ((env = getenv("ELVEN_AUDIO_OUTPUT"))) {
    our_ports = jack_get_ports(client, (string(name) + ":*").c_str(),
			       JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
    if (our_ports && our_ports[0]) {
      
      // if it's a client, connect individual ports
      if (index(env, ':') == NULL &&
	  (port_list = jack_get_ports(client, (string(env) + ":*").c_str(),
				      JACK_DEFAULT_AUDIO_TYPE, 
				      JackPortIsInput)) && port_list[0]) {
	for (int i = 0; port_list[i] && our_ports[i]; ++i)
	  jack_connect(client, our_ports[i], port_list[i]);
	free(port_list);
      }
      
      // if not, connect all our ports to that single port
      else {
	for (int i = 0; our_ports[i]; ++i)
	  jack_connect(client, our_ports[i], env);
      }
      
    }
    free(our_ports);
  }  
  
}


string escape_space(const string& str) {
  string str2 = str;
  int pos = 0;
  while ((pos = str2.find("\\", pos)) != string::npos) {
    str2.replace(pos, 1, "\\b");
    pos += 2;
  }
  pos = 0;
  while ((pos = str2.find(" ", pos)) != string::npos) {
    str2.replace(pos, 1, "\\s");
    pos += 2;
  }
  pos = 0;
  while ((pos = str2.find("\t", pos)) != string::npos) {
    str2.replace(pos, 1, "\\t");
    pos += 2;
  }
  pos = 0;
  while ((pos = str2.find("\n", pos)) != string::npos) {
    str2.replace(pos, 1, "\\n");
    pos += 2;
  }
  pos = 0;
  while ((pos = str2.find("\r", pos)) != string::npos) {
    str2.replace(pos, 1, "\\r");
    pos += 2;
  }
  return str2;
}


string unescape_space(const string& str) {
  string str2 = str;
  int pos = 0;
  while ((pos = str2.find("\\b", pos)) != string::npos) {
    str2.replace(pos, 2, "\\");
    pos += 1;
  }
  while ((pos = str2.find("\\r", pos)) != string::npos) {
    str2.replace(pos, 2, "\r");
    pos += 1;
  }
  while ((pos = str2.find("\\n", pos)) != string::npos) {
    str2.replace(pos, 2, "\n");
    pos += 1;
  }
  while ((pos = str2.find("\\t", pos)) != string::npos) {
    str2.replace(pos, 2, "\t");
    pos += 1;
  }
  while ((pos = str2.find("\\s", pos)) != string::npos) {
    str2.replace(pos, 2, " ");
    pos += 1;
  }
  pos = 0;
  return str2;
}


/** Translate from an LV2 MIDI buffer to a JACK MIDI buffer. */
void lv2midi2jackmidi(LV2Port& port, jack_port_t* jack_port, 
                      jack_nframes_t nframes) {
  
  DBG4("Translating MIDI events from LV2 to JACK for port "<<port.symbol);

  
  void* output_buf = jack_port_get_buffer(jack_port, nframes);
  LV2_Event_Buffer* input_buf = static_cast<LV2_Event_Buffer*>(port.buffer);
  LV2_Event_Iterator iter;
  lv2_event_begin(&iter, input_buf);
  
  jack_midi_clear_buffer(output_buf);
  
  // iterate over all MIDI events and write them to the JACK port
  for (size_t i = 0; i < input_buf->event_count; ++i) {
    
    // retrieve LV2 MIDI event
    uint8_t* data;
    LV2_Event* ev = lv2_event_get(&iter, &data);
    lv2_event_increment(&iter);
    
    if (ev->type == 1) {
      DBG3("Received MIDI event from the plugin on port "<<port.symbol
	   <<": "<<midi2str(ev->size, data));
      
      // write JACK MIDI event
      jack_midi_event_write(output_buf, jack_nframes_t(ev->frames), 
			    reinterpret_cast<jack_midi_data_t*>(data), 
			    ev->size);
    }
  }
}


/** Translate from a JACK MIDI buffer to an LV2 MIDI buffer. */
void jackmidi2lv2midi(jack_port_t* jack_port, LV2Port& port,
                      LV2Host& host, jack_nframes_t nframes) {
  
  DBG4("Translating MIDI events from JACK to LV2 for port "<<port.symbol);
  
  static unsigned bank = 0;
  
  void* input_buf = jack_port_get_buffer(jack_port, nframes);
  jack_midi_event_t input_event;
  jack_nframes_t input_event_index = 0;
  jack_nframes_t input_event_count = jack_midi_get_event_count(input_buf);
  jack_nframes_t timestamp;
  LV2_Event_Buffer* output_buf = static_cast<LV2_Event_Buffer*>(port.buffer);
  lv2_event_buffer_reset(output_buf, 0, output_buf->data);
  LV2_Event_Iterator iter;
  lv2_event_begin(&iter, output_buf);
  output_buf->event_count = 0;
  
  // iterate over all incoming JACK MIDI events
  unsigned char* data = output_buf->data;
  for (unsigned int i = 0; i < input_event_count; ++i) {
    
    // retrieve JACK MIDI event
    jack_midi_event_get(&input_event, input_buf, i);
    
    DBG3("Received MIDI event from JACK on port "<<port.symbol
         <<": "<<midi2str(input_event.size, input_event.buffer));
    
    if ((data - output_buf->data) + sizeof(double) + 
        sizeof(size_t) + input_event.size >= output_buf->capacity)
      break;
    
    // check if it's a bank select MSB
    if ((input_event.size == 3) && ((input_event.buffer[0] & 0xF0) == 0xB0) &&
        (input_event.buffer[1] == 0)) {
      bank = (bank & 0x7F) + (input_event.buffer[2] & 0x7F) << 7;
    }
    
    // LSB
    else if ((input_event.size == 3) && 
             ((input_event.buffer[0] & 0xF0) == 0xB0) &&
             (input_event.buffer[1] == 32)) {
      bank = (bank & (0x7F << 7)) + (input_event.buffer[2] & 0x7F);
    }
    
    // or a mapped CC
    else if ((input_event.size == 3) && 
             ((input_event.buffer[0] & 0xF0) == 0xB0) &&
             (host.get_midi_map()[input_event.buffer[1]] != -1)) {
      int port = host.get_midi_map()[input_event.buffer[1]];
      float* pbuf = static_cast<float*>(host.get_ports()[port].buffer);
      float& min = host.get_ports()[port].min_value;
      float& max = host.get_ports()[port].max_value;
      *pbuf = min + (max - min) * input_event.buffer[2] / 127.0;
      DBG3("Mapped CC event to port "<<port);
      // XXX notify the main thread somehow
      //host.queue_control(port, *pbuf, false);
    }
    
    // or a program change
    /*
      else if ((input_event.size == 2) && 
      ((input_event.buffer[0] & 0xF0) == 0xC0)) {
      host.select_program(128 * bank + input_event.buffer[1]);
      }
    */
    
    else {
      // write LV2 MIDI event
      lv2_event_write(&iter, input_event.time, 0, 1, 
		      input_event.size, input_event.buffer);
      
      // XXX add normalisation again
      // normalise note events if needed
      /*if ((input_event.size == 3) && ((data[0] & 0xF0) == 0x90) && 
          (data[2] == 0))
        data[0] = 0x80 | (data[0] & 0x0F);
      */
    }
  }
  
}


/** The JACK process callback */
int process(jack_nframes_t nframes, void* arg) {
  
  LV2Host* host = static_cast<LV2Host*>(arg);
  
  // iterate over all ports and copy data from JACK ports to audio and MIDI
  // ports in the plugin
  for (size_t i = 0; i < host->get_ports().size(); ++i) {
    
    // does this plugin port have an associated JACK port?
    if (jack_ports[i]) {

      LV2Port& port = host->get_ports()[i];
      
      // audio port, just copy the buffer pointer.
      if (port.type == AudioType)
        port.buffer = jack_port_get_buffer(jack_ports[i], nframes);
      
      // MIDI input port, copy the events one by one
      else if (port.type == MidiType && port.direction == InputPort)
        jackmidi2lv2midi(jack_ports[i], port, *host, nframes);
      
    }
  }
  
  // run the plugin!
  host->run(nframes);
  
  // Copy events from MIDI output ports to JACK ports
  for (size_t i = 0; i < host->get_ports().size(); ++i) {
    if (jack_ports[i]) {
      LV2Port& port = host->get_ports()[i];
      if (port.type == MidiType && port.direction == OutputPort)
        lv2midi2jackmidi(port, jack_ports[i], nframes);
    }
  }
  
  return 0;
}


void thread_init(void*) {
  DebugInfo::thread_prefix()[pthread_self()] = "J ";
}


void sigchild(int signal) {
  DBG2("Child process terminated");
  if (signal == SIGCHLD)
    still_running = false;
}


void print_version() {
  clog<<"Elven is an (E)xperimental (LV)2 (E)xecution e(N)vironment.\n"
      <<"Version " VERSION 
      <<", (C) 2006-2007 Lars Luthman <mail@larsluthman.net>\n"
      <<"Released under the GNU General Public License, version 3 or later.\n"
      <<endl;
}


void print_usage(const char* argv0) {
  clog<<"usage:   "<<argv0<<" --help\n"
      <<"         "<<argv0<<" --list\n"
      <<"         "<<argv0<<" [--debug DEBUGLEVEL] [--nogui] PLUGIN_URI\n\n"
      <<"example: "<<argv0
      <<" http://ll-plugins.nongnu.org/lv2/dev/klaviatur/0.0.0\n"
      <<endl;
}


void print_help(const char* argv0) {
  clog<<"Elven will first try to find a plugin with the given URI, if\n"
      <<"it fails it will try to find a plugin that contains the given\n"
      <<"URI as a substring. Thus '"<<argv0<<" klav' will load the plugin\n"
      <<"with the URI http://ll-plugins.nongnu.org/lv2/dev/klaviatur/0.0.0,\n"
      <<"unless Elven happens to find another plugin whose URI contains\n"
      <<"the substring 'klav' first."<<endl;
}


int main(int argc, char** argv) {
  
  setlocale(LC_NUMERIC, "C");
  
  // prevent GTK from ruining our locale settings
  gtk_disable_setlocale();
  
  Gtk::Main kit(argc, argv);
  
  DebugInfo::prefix() = "H:";
  DebugInfo::thread_prefix()[pthread_self()] = "M ";
  
  bool load_gui = true;
  
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }
  
  int i;
  for (i = 1; i < argc; ++i) {
    
    // print help
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      print_version();
      print_usage(argv[0]);
      print_help(argv[0]);
      return 0;
    }
    
    // print version info
    if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      print_version();
      return 0;
    }
    
    // list all available plugins
    else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list")) {
      LV2Host::list_plugins();
      return 0;
    }
    
    // set debugging level (higher level -> more messages)
    else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")) {
      if (i == argc - 1) {
        DBG0("No debug level given!");
        return 1;
      }
      DebugInfo::level() = atoi(argv[i + 1]);
      ++i;
    }
    
    // don't load a GUI plugin
    else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nogui")) {
      load_gui = false;
    }
    
    else
      break;
  }
  
  if (i >= argc) {
    print_usage(argv[0]);
    return 1;
  }
    
  // initialise JACK client
  if (!(jack_client = jack_client_open("Elven", jack_options_t(0), 0))) {
    DBG0("Could not initialise JACK client!");
    return -1;
  }
      
  // load plugin
  string plugin_uri = argv[i];
  LV2Host lv2h(plugin_uri, jack_get_sample_rate(jack_client));
  
  if (lv2h.is_valid()) {
    
    DBG2("Plugin host is OK");
    
    bool has_map = false;
    for (unsigned j = 0; j < 127; ++j) {
      long port = lv2h.get_midi_map()[j];
      if (port == -1)
        continue;
      if (!has_map)
        DBG2("MIDI map:");
      has_map = true;
      DBG2("  "<<j<<" -> "<<port<<" ("<<lv2h.get_ports()[port].symbol<<")");
    }
    if (has_map)
      DBG2("");
    
    DBG2("Default MIDI port: "<<lv2h.get_default_midi_port());
    
    // initialise port buffers
    for (size_t p = 0; p < lv2h.get_ports().size(); ++p) {
      jack_port_t* port = 0;
      LV2Port& lv2port = lv2h.get_ports()[p];
      
      // add JACK MIDI port and allocate internal MIDI buffer
      if (lv2port.type == MidiType) {
        port = jack_port_register(jack_client, lv2port.symbol.c_str(),
                                  JACK_DEFAULT_MIDI_TYPE,
                                  (lv2port.direction == InputPort ?
                                   JackPortIsInput : JackPortIsOutput), 0);
        LV2_Event_Buffer* mbuf = lv2_event_buffer_new(8192, 0);
        lv2port.buffer = mbuf;
      }
      
      // add JACK audio port
      else if (lv2port.type == AudioType) {
        port = jack_port_register(jack_client, lv2port.symbol.c_str(),
                                  JACK_DEFAULT_AUDIO_TYPE,
                                  (lv2port.direction == InputPort ?
                                   JackPortIsInput : JackPortIsOutput), 0);
      }
      
      // for control ports, just create buffers consisting of a single float
      else if (lv2port.type == ControlType) {
        lv2port.buffer = new float;
        *static_cast<float*>(lv2port.buffer) = lv2port.default_value;
	lv2port.value = lv2port.default_value;
      }
      
      jack_ports.push_back(port);
    }
    
    still_running = true;
    
    // start the GUI
    string gui_path;
    if (load_gui)
      gui_path = lv2h.get_gui_path();
    Gtk::Window* win = 0;
    LV2GUIHost* lv2gh = 0;
    if (gui_path.size()) {
      string gui_bundle;
      int pos = gui_path.rfind(".lv2/");
      if (pos != string::npos)
	gui_bundle = gui_path.substr(0, pos + 5);

      lv2gh = new LV2GUIHost(gui_path, lv2h.get_gui_uri(), 
			     lv2h.get_plugin_uri(), gui_bundle);
      if (!lv2gh->is_valid()) {
	delete lv2gh;
	lv2gh = 0;
      }
      else {
	string icon_path = lv2h.get_icon_path();
	win = new Gtk::Window;
	win->set_title(lv2h.get_name());
	win->add(lv2gh->get_widget());
	lv2gh->get_widget().show_all();
	if (icon_path.size())
	  win->set_icon(Gdk::Pixbuf::create_from_file(icon_path));
	const std::vector<LV2Port>& ports = lv2h.get_ports();
	
	// update the port controls in the GUI
	for (uint32_t i = 0; i < ports.size(); ++i) {
	  if (ports[i].type == ControlType && ports[i].direction == InputPort)
	    lv2gh->port_event(i, sizeof(float), 0, ports[i].buffer);
	}
	
	// update the program controls in the GUI
	const std::map<unsigned, LV2Preset>& presets = lv2h.get_presets();
	std::map<unsigned, LV2Preset>::const_iterator iter;
	for (iter = presets.begin(); iter != presets.end(); ++iter)
	  lv2gh->program_added(iter->first, iter->second.name.c_str());
	
	// connect signals (plugin -> GUI)
	lv2h.signal_port_event.
	  connect(mem_fun(*lv2gh, &LV2GUIHost::port_event));
	lv2h.signal_program_changed.
	  connect(mem_fun(*lv2gh, &LV2GUIHost::current_program_changed));
	lv2h.signal_program_added.
	  connect(mem_fun(*lv2gh, &LV2GUIHost::program_added));
	
	// GUI -> plugin
	lv2gh->write_control.connect(mem_fun(lv2h, &LV2Host::set_control));
	lv2gh->write_events.connect(mem_fun(lv2h, &LV2Host::queue_events));
	lv2gh->request_program.connect(mem_fun(lv2h, &LV2Host::set_program));
	lv2gh->save_program.connect(mem_fun(lv2h, &LV2Host::save_program));
      }
    }

    jack_set_process_callback(jack_client, &process, &lv2h);
    jack_set_thread_init_callback(jack_client, &thread_init, 0);
    if (lv2h.get_presets().size() > 0)
      lv2h.set_program(lv2h.get_presets().begin()->first);
    lv2h.activate();
    jack_activate(jack_client);
    
    autoconnect(jack_client);
    
    // wait until we are killed
    Glib::signal_timeout().
      connect(bind_return(mem_fun(lv2h, &LV2Host::run_main), true), 10);
    if (win) {
      kit.run(*win);
      win->show_all();
    }
    else
      kit.run();
    
    jack_client_close(jack_client);
    delete lv2gh;
    lv2h.deactivate();
  }
  
  else {
    return 1;
  }
  
  DBG2("Exiting");
  
  return 0;
}
