/****************************************************************************
    
    rudolf556widget.hpp - GUI for the Rudolf 556 drum machine
    
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

#ifndef RUDOLF556WIDGET_HPP
#define RUDOLF556WIDGET_HPP

#include <iostream>
#include <string>
#include <vector>

#include <gtkmm.h>


class Rudolf556Widget : public Gtk::DrawingArea {
public:
  
  Rudolf556Widget(const std::string& bundle);
  
  void set_control(unsigned control, float value);
  
  sigc::signal<void, unsigned, float> signal_control_changed;
  
protected:
  
  struct Control {
    float value;
    float x;
    float y;
  };
  
  void on_realize();
  bool on_expose_event(GdkEventExpose* event);
  bool on_button_press_event(GdkEventButton* event);
  bool on_scroll_event(GdkEventScroll* event);
  void on_drag_data_get(const Glib::RefPtr<Gdk::DragContext>& context,
			Gtk::SelectionData& selection_data, 
			guint info, guint time);
  
  unsigned find_control(float x, float y);
  bool deactivate_controls();
  
  std::string m_bundle;
  std::vector<Control> m_controls;
  unsigned m_active_control;

  sigc::connection m_deactivate_conn;
  
};


#endif
