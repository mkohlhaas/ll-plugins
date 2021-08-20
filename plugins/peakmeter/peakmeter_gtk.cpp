/****************************************************************************
    
    peakmeter_gtk.cpp - A GUI for the VU meter LV2 plugins
    
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
#include "vuwidget.hpp"


using namespace std;
using namespace Gtk;
using namespace sigc;


template <unsigned C>
class PeakMeterGUI : public LV2::GUI< PeakMeterGUI<C> > {
public:
  
  PeakMeterGUI(const std::string& URI) 
    : m_vu(C) {
    LV2::GUI< PeakMeterGUI<C> >::add(m_vu);
  }
  
  void port_event(uint32_t port, uint32_t buffer_size, 
		  uint32_t format, const void* buffer) {
    for (unsigned c = 0; c < C; ++c) {
      if (port == 2 * c + 1 && buffer_size == sizeof(float))
	m_vu.set_value(c, *static_cast<const float*>(buffer));
    }
  }

protected:
  
  VUWidget m_vu;
  
};



static int _1 = PeakMeterGUI<1>::register_class("http://ll-plugins.nongnu.org/lv2/dev/peakmeter/0/gui");
static int _2 = PeakMeterGUI<2>::register_class("http://ll-plugins.nongnu.org/lv2/dev/peakmeter-stereo/0/gui");
