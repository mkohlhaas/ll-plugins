/****************************************************************************
    
    lv2guihost.cpp - Simple LV2 GUI plugin loader for Elven
    
    Copyright (C) 2007 Lars Luthman <mail@larsluthman.net>
    
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

#include <cstring>
#include <iostream>

#include <dlfcn.h>

#include <lv2_event.h>

#include "debug.hpp"
#include "lv2guihost.hpp"


using namespace std;


LV2GUIHost::LV2GUIHost(const std::string& gui_path, 
		       const std::string& gui_uri,
		       const std::string& plugin_uri, 
		       const std::string& bundle_path)
  : m_desc(0),
    m_pdesc(0),
    m_ui(0),
    m_cwidget(0),
    m_widget(0),
    m_block_gui(false) {
  
  // initialise the host descriptor for the preset extension
  m_phdesc.change_preset = &LV2GUIHost::_request_program;
  m_phdesc.save_preset = &LV2GUIHost::_save_program;
  
  // initialise the host descriptor for the URI map extension
  m_urimap_desc.callback_data = 0;
  m_urimap_desc.uri_to_id = &LV2GUIHost::uri_to_id;
  
  // open the module
  DBG2("Loading "<<gui_path);
  void* module = dlopen(gui_path.c_str(), RTLD_LAZY);
  if (!module) {
    DBG0("Could not load "<<gui_path<<": "<<dlerror());
    return;
  }
  
  // get the GUI descriptor
  LV2UI_DescriptorFunction func = 
    nasty_cast<LV2UI_DescriptorFunction>(dlsym(module, "lv2ui_descriptor"));
  if (!func) {
    DBG0("Could not find symbol lv2ui_descriptor in "<<gui_path);
    return;
  }
  for (unsigned i = 0; (m_desc = func(i)); ++i) {
    DBG2("Found GUI "<<m_desc->URI);
    if (!strcmp(gui_uri.c_str(), m_desc->URI))
      break;
    DBG2(m_desc->URI<<" does not match "<<gui_uri);
  }
  if (!m_desc) {
    DBG0(gui_path<<" did not contain the plugin GUI "<<gui_uri);
    return;
  }
  
  // get extension data
  if (m_desc->extension_data) {
    m_pdesc = static_cast<LV2UI_Presets_GDesc const*>
      (m_desc->extension_data(LV2_UI_PRESETS_URI));
    if (m_pdesc)
      DBG2("The plugin GUI supports the preset feature");
    else
      DBG2("The plugin GUI does not support the preset feature");
  }
  else
    DBG2("The plugin GUI has no extension_data() callback");
  
  // build the feature list
  LV2_Feature** features = new LV2_Feature*[3];
  LV2_Feature presets_feature = { LV2_UI_PRESETS_URI, &m_phdesc };
  LV2_Feature urimap_feature = { LV2_URI_MAP_URI, &m_urimap_desc };
  features[0] = &presets_feature;
  features[1] = &urimap_feature;
  features[2] = 0;
  
  // create a GUI instance
  LV2UI_Controller ctrl = static_cast<LV2UI_Controller>(this);
  m_block_gui = true;
  m_ui = m_desc->instantiate(m_desc, plugin_uri.c_str(), bundle_path.c_str(),
			     &LV2GUIHost::_write_port, ctrl,
			     reinterpret_cast<LV2UI_Widget*>(&m_cwidget),
			     const_cast<const LV2_Feature**>(features));
  m_block_gui = false;
  if (!m_ui || !m_cwidget) {
    DBG0("Could not create an UI instance");
    m_desc = 0;
    m_widget = 0;
    m_ui = 0;
    return;
  }
  
  m_widget = Glib::wrap(m_cwidget);
}

  
LV2GUIHost::~LV2GUIHost() {
  if (m_ui)
    m_desc->cleanup(m_ui);
}

  
bool LV2GUIHost::is_valid() const {
  return (m_ui != 0);
}
  

Gtk::Widget& LV2GUIHost::get_widget() {
  return *m_widget;
}
  

void LV2GUIHost::port_event(uint32_t index, uint32_t buffer_size, 
			    uint32_t format, const void* buffer) {
  if (m_ui && m_desc && m_desc->port_event) {
    m_block_gui = true;
    m_desc->port_event(m_ui, index, buffer_size, format, buffer);
    m_block_gui = false;
  }
}
 
 
void LV2GUIHost::program_added(uint32_t number, const char* name) {
  if (m_ui && m_pdesc && m_pdesc->preset_added) {
    m_block_gui = true;
    m_pdesc->preset_added(m_ui, number, name);
    m_block_gui = false;
  }
}

  
void LV2GUIHost::program_removed(uint32_t number) {
  if (m_ui && m_pdesc && m_pdesc->preset_removed) {
    m_block_gui = true;
    m_pdesc->preset_removed(m_ui, number);
    m_block_gui = false;
  }
}
 
 
void LV2GUIHost::programs_cleared() {
  if (m_ui && m_pdesc && m_pdesc->presets_cleared) {
    m_block_gui = true;
    m_pdesc->presets_cleared(m_ui);
    m_block_gui = false;
  }
}

  
void LV2GUIHost::current_program_changed(uint32_t number) {
  if (m_ui && m_pdesc && m_pdesc->current_preset_changed) {
    m_block_gui = true;
    m_pdesc->current_preset_changed(m_ui, number);
    m_block_gui = false;
  }
}


void LV2GUIHost::_write_port(LV2UI_Controller ctrl, uint32_t index, 
			     uint32_t buffer_size, uint32_t format,
			     const void* buffer) {
  // XXX handle format stuff here
  LV2GUIHost* me = static_cast<LV2GUIHost*>(ctrl);
  if (me->m_block_gui)
    DBG1("GUI requested write to input port while a GUI callback was running");
  else {
    if (format == 0) {
      DBG2("GUI requested write to control input port "<<index);
      me->write_control(index, *static_cast<const float*>(buffer));
    }
    else if (format == 1) {
      DBG2("GUI requested write to event input port "<<index);
      me->write_events(index, static_cast<const LV2_Event_Buffer*>(buffer));
    }
    else
      DBG0("GUI requested write to input port "<<index<<
	   " in unknown format "<<format);
  }
}
  

void LV2GUIHost::_request_program(LV2UI_Controller ctrl, uint32_t number) {
  LV2GUIHost* me = static_cast<LV2GUIHost*>(ctrl);
  if (me->m_block_gui)
    DBG1("GUI requested preset change while a GUI callback was running");
  else {
    DBG2("GUI requested preset change to "<<int(number));
    me->request_program(number);
  }
}


void LV2GUIHost::_save_program(LV2UI_Controller ctrl, uint32_t number,
			       const char* name) {
  LV2GUIHost* me = static_cast<LV2GUIHost*>(ctrl);
  if (me->m_block_gui)
    DBG1("GUI requested preset save while a GUI callback was running");
  else {
    DBG2("GUI requested preset save to "<<int(number)<<", \""<<name<<"\"");
    me->save_program(number, name);
  }
}


uint32_t LV2GUIHost::uri_to_id(LV2_URI_Map_Callback_Data callback_data,
			       const char* umap, const char* uri) {
  if (umap && !strcmp(umap, "http://lv2plug.in/ns/extensions/ui") &&
      !strcmp(uri, "http://lv2plug.in/ns/extensions/ui#Events"))
    return 1;
  else if (umap && !strcmp(umap, LV2_EVENT_URI)) {
    if (!strcmp(uri, "http://lv2plug.in/ns/ext/midi#MidiEvent"))
      return 1;
    else if (!strcmp(uri, "http://lv2plug.in/ns/ext/osc#OscEvent"))
      return 2;
  }
  return 0;
}

