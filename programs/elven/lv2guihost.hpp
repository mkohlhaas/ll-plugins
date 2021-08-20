/****************************************************************************
    
    lv2guihost.hpp - Simple LV2 GUI plugin loader for Elven
    
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

#ifndef LV2GUIHOST_HPP
#define LV2GUIHOST_HPP

#include <string>

#include <gtkmm.h>
#include <sigc++/sigc++.h>

#include <lv2.h>
#include <lv2_ui.h>
#include <lv2_ui_presets.h>
#include <lv2_uri_map.h>
#include <lv2_event.h>


class LV2GUIHost {
public:
  
  LV2GUIHost(const std::string& gui_path, const std::string& gui_uri,
	     const std::string& plugin_uri, const std::string& bundle_path);
  
  ~LV2GUIHost();
  
  bool is_valid() const;
  
  Gtk::Widget& get_widget();
  
  void port_event(uint32_t index, uint32_t buffer_size, 
		  uint32_t format, const void* buffer);
  
  void program_added(uint32_t number, const char* name);
  
  void program_removed(uint32_t number);
  
  void programs_cleared();
  
  void current_program_changed(uint32_t number);
  
  sigc::signal<void, uint32_t, float> write_control;

  sigc::signal<void, uint32_t, const LV2_Event_Buffer*> write_events;
  
  sigc::signal<void, uint32_t> request_program;

  sigc::signal<void, uint32_t, char const*> save_program;

protected:
  
  static void _write_port(LV2UI_Controller ctrl, uint32_t index, 
			  uint32_t buffer_size, uint32_t format, 
			  const void* buffer);
  
  static void _request_program(LV2UI_Controller ctrl, uint32_t number);

  static void _save_program(LV2UI_Controller ctrl, uint32_t number,
			    const char* name);

  static uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data,
			    const char* umap, const char* uri);
  
  
  /** This is needed to cast void* (returned by dlsym()) to a function
      pointer. */
  template <typename A, typename B> A nasty_cast(B b) {
    union {
      A a;
      B b;
    } u;
    u.b = b;
    return u.a;
  }

  
  LV2UI_Descriptor const* m_desc;
  LV2UI_Presets_GDesc const* m_pdesc;
  LV2UI_Handle m_ui;
  GtkWidget* m_cwidget;
  Gtk::Widget* m_widget;
  
  bool m_block_gui;
  
  LV2UI_Presets_Feature m_phdesc;
  LV2_URI_Map_Feature m_urimap_desc;
};


#endif
