/****************************************************************************
    
    sineshaper_gtk.cpp - A GUI for the Sineshaper LV2 synth
    
    Copyright (C) 2006-2007  Lars Luthman <mail@larsluthman.net>
    
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

#include <iostream>

#include <gtkmm.h>

#include "lv2gui.hpp"
#include "sineshaperwidget.hpp"
#include "sineshaper.peg"


using namespace std;
using namespace Gtk;
using namespace sigc;


class SineshaperGUI : public LV2::GUI<SineshaperGUI, LV2::Presets<false> > {
public:
  
  SineshaperGUI(const std::string& URI)
    : m_sshp(bundle_path(), host_supports_presets()) {
    
    pack_start(m_sshp);

    m_sshp.signal_control_changed.
      connect(mem_fun(*this, &SineshaperGUI::write_control));
    m_sshp.signal_preset_changed.
      connect(mem_fun(*this, &SineshaperGUI::change_preset));
    m_sshp.signal_save_preset.
      connect(mem_fun(*this, &SineshaperGUI::save_preset));
  }
  
  void port_event(uint32_t port, uint32_t buffer_size, 
		  uint32_t format, const void* buffer) {
    m_sshp.set_control(port, *static_cast<const float*>(buffer));
  }
  
  void preset_added(uint32_t number, const char* name) {
    m_sshp.add_preset(number, name);
  }
  
  void preset_removed(uint32_t number) {
    m_sshp.remove_preset(number);
  }
  
  void presets_cleared() {
    m_sshp.clear_presets();
  }
  
  void current_preset_changed(uint32_t number) {
    m_sshp.set_preset(number);
  }
  
protected:
  
  SineshaperWidget m_sshp;
  
};


static int _ = SineshaperGUI::register_class((string(s_uri) + "/gui").c_str());
