/****************************************************************************
    
    rudolf556_gtk.cpp - GUI for the Rudolf 556 drum machine
    
    Copyright (C) 2008 Lars Luthman <mail@larsluthman.net>
    
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
#include <string>

#include <gtkmm.h>
#include <lv2gui.hpp>

#include "rudolf556widget.hpp"
#include "rudolf556.peg"


using namespace sigc;
using namespace Gtk;


class Rudolf556GUI : public LV2::GUI<Rudolf556GUI> {
public:
  
  Rudolf556GUI(const std::string& URI)
    : m_rud(bundle_path()) {
    pack_start(m_rud);
    m_rud.signal_control_changed.
      connect(mem_fun(*this, &Rudolf556GUI::write_control));
  }
  
  void port_event(uint32_t port, uint32_t buffer_size, 
		  uint32_t format, void const* buffer) {
    m_rud.set_control(port, *static_cast<float const*>(buffer));
  }

protected:
  
  Rudolf556Widget m_rud;
  
};


static int _ = 
  Rudolf556GUI::register_class((std::string(r_uri) + "/gui").c_str());

