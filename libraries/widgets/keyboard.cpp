/****************************************************************************
    
    keyboard.cpp - Simple MIDI keyboard widget
    
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
#include <sstream>

using namespace std;


#include "keyboard.hpp"


using namespace Gtk;
using namespace Gdk;
using namespace Glib;


Keyboard::Keyboard(unsigned int octaves, unsigned int octave_offset,
		   unsigned int keywidth, unsigned int bkeywidth, 
		   unsigned int keyheight, unsigned int bkeyheight, 
		   ClickBehaviour cb)
  : m_white("#FFFFFF"),
    m_black("#000000"),
    m_grey1("#AAAAAA"),
    m_grey2("#666666"),
    m_green1("#A0FFA0"),
    m_green2("#006000"),
    m_keys_on(128, false),
    m_octaves(octaves),
    m_keywidth(keywidth),
    m_bkeywidth(bkeywidth),
    m_keyheight(keyheight),
    m_bkeyheight(bkeyheight),
    m_octave_offset(octave_offset <= 9 ? octave_offset : 9),
    m_moused_key(255),
    m_click_behaviour(cb),
    m_motion_turns_on(true) {
  
  m_keymap['z'] = 0;
  m_keymap['s'] = 1;
  m_keymap['x'] = 2;
  m_keymap['d'] = 3;
  m_keymap['c'] = 4;
  m_keymap['v'] = 5;
  m_keymap['g'] = 6;
  m_keymap['b'] = 7;
  m_keymap['h'] = 8;
  m_keymap['n'] = 9;
  m_keymap['j'] = 10;
  m_keymap['m'] = 11;
  m_keymap['q'] = 12;
  m_keymap['2'] = 13;
  m_keymap['w'] = 14;
  m_keymap['3'] = 15;
  m_keymap['e'] = 16;
  m_keymap['r'] = 17;
  m_keymap['5'] = 18;
  m_keymap['t'] = 19;
  m_keymap['6'] = 20;
  m_keymap['y'] = 21;
  m_keymap['7'] = 22;
  m_keymap['u'] = 23;
  m_keymap['i'] = 24;
  m_keymap['9'] = 25;
  m_keymap['o'] = 26;
  m_keymap['0'] = 27;
  m_keymap['p'] = 28;
  
  set_size_request((octaves * 7 + 1) * keywidth + 1, keyheight + 1);
  
  RefPtr<Colormap> cmap = Colormap::get_system();
  cmap->alloc_color(m_white);
  cmap->alloc_color(m_black);
  cmap->alloc_color(m_grey1);
  cmap->alloc_color(m_grey2);
  cmap->alloc_color(m_green1);
  cmap->alloc_color(m_green2);
  
  // compute the text size
  RefPtr<Pango::Layout> l = Pango::Layout::create(get_pango_context());
  l->set_text("10");
  Pango::FontDescription fd;
  fd.set_family("monospace");
  fd.set_absolute_size(PANGO_SCALE * 100);
  l->set_font_description(fd);
  Pango::Rectangle r = l->get_logical_extents();
  int wanted_width = int((m_keywidth - 4) * 2.0 / 3.0);
  m_text_size = PANGO_SCALE * 100 / 
    (r.get_width() / (wanted_width * PANGO_SCALE));
  
  add_events(KEY_PRESS_MASK | KEY_RELEASE_MASK |
             BUTTON_PRESS_MASK | BUTTON_RELEASE_MASK | 
	     BUTTON1_MOTION_MASK | SCROLL_MASK);
  show_all();
}


bool Keyboard::on_key_press_event(GdkEventKey* event) {
  std::map<int, unsigned char>::const_iterator iter;
  iter = m_keymap.find(event->keyval);
  if (iter != m_keymap.end() && iter->second + m_octave_offset * 12 < 128)
    key_on(iter->second + m_octave_offset * 12);
  return true;
}


bool Keyboard::on_key_release_event(GdkEventKey* event) {
  std::map<int, unsigned char>::const_iterator iter;
  iter = m_keymap.find(event->keyval);
  if (iter != m_keymap.end() && 
      iter->second + m_octave_offset * 12 != m_moused_key)
    key_off(iter->second + m_octave_offset * 12);
  return true;
}


bool Keyboard::on_button_press_event(GdkEventButton* event) {
  grab_focus();
  if (event->button != 1)
    return false;
  unsigned char k = pixel_to_key(int(event->x), int(event->y));
  if (k != 255) {
    if (m_click_behaviour == CLICK_ON) {
      if (m_moused_key != 255)
        key_off(m_moused_key);
      m_moused_key = k;
      key_on(k);
    }
    else if (m_click_behaviour == CLICK_TOGGLE) {
      if (get_key_state(k)) {
        key_off(k);
        m_motion_turns_on = false;
      }
      else {
        key_on(k);
        m_motion_turns_on = true;
      }
      m_moused_key = k;
    }
  }
  return true;
}


bool Keyboard::on_button_release_event(GdkEventButton* event) {
  if (event->button != 1)
    return false;
  if (m_moused_key != 255 && m_click_behaviour == CLICK_ON)
    key_off(m_moused_key);
  m_moused_key = 255;
  return true;
}


bool Keyboard::on_motion_notify_event(GdkEventMotion* event) {
  
  unsigned char k = pixel_to_key(int(event->x), int(event->y), false, true);
  if (k != 255) {
    
    if (m_click_behaviour == CLICK_ON) {
      if (m_moused_key != 255 && m_moused_key != k)
        key_off(m_moused_key);
      m_moused_key = k;
      key_on(k);
    }
    
    else if (m_click_behaviour == CLICK_TOGGLE) {
      unsigned char a, b;
      if (m_moused_key != 255) {
        a = (m_moused_key < k ? m_moused_key : k);
        b = (m_moused_key >= k ? m_moused_key : k);
      }
      else {
        a = k;
        b = k;
      }
      for (unsigned char c = a; c <= b; ++c) {
        if (!is_black(c) || event->y < m_bkeyheight) {
          if (m_motion_turns_on)
            key_on(c);
          else
            key_off(c);
        }
      }
    }
    m_moused_key = k;
  }

  return true;
}


bool Keyboard::on_scroll_event(GdkEventScroll* event) {
  if (event->direction == GDK_SCROLL_UP && m_octave_offset < 11 - m_octaves) {
    ++m_octave_offset;
    queue_draw();
  }
  else if (event->direction == GDK_SCROLL_DOWN && m_octave_offset > 0) {
    --m_octave_offset;
    queue_draw();
  }
  return true;
}




void Keyboard::on_realize() {
  DrawingArea::on_realize();
  m_win = get_window();
  m_gc = GC::create(m_win);
  m_win->clear();
}


bool Keyboard::on_expose_event(GdkEventExpose* event) {
  
  unsigned int a = pixel_to_key(event->area.x, m_keyheight / 2, true);
  unsigned int b = pixel_to_key(event->area.x + event->area.width, 
                                 m_bkeyheight + 1, true);
  
  m_gc->set_foreground(m_white);
  
  b = (b < (m_octaves + m_octave_offset) * 12 ? 
       b : (m_octaves + m_octave_offset) * 12);
  b = b < 128 ? b : 127;
  int x = 0;
  for (unsigned int k = m_octave_offset * 12; k <= b; ++k) {
    if (k >= a)
      draw_white_key(k, x, m_keys_on[k]);
    x += m_keywidth;
    if (k % 12 == 0 || k % 12 == 2 || 
        k % 12 == 5 || k % 12 == 7 || k % 12 == 9)
      ++k;
  }
  

  x = 0;
  a = (a == 0 ? a : a - 1);
  b = (b == (m_octaves + m_octave_offset) * 12 ? b : b + 1);
  b = b < 128 ? b : 127;
  for (unsigned int k = m_octave_offset * 12; k <= b; ++k) {
    x += m_keywidth;
    if ((k % 12 == 0 || k % 12 == 2 || 
         k % 12 == 5 || k % 12 == 7 || k % 12 == 9) && 
	k != (m_octaves + m_octave_offset) * 12) {
      ++k;
      if (k >= a && k <= b)
        draw_black_key(x, m_keys_on[k]);
    }
  }


  return true;
}


void Keyboard::draw_white_key(unsigned char k, int x, bool on) {
  if (on)
    m_gc->set_foreground(m_green1);
  else
    m_gc->set_foreground(m_white);
  m_win->draw_rectangle(m_gc, true, x, 0, m_keywidth, m_keyheight);
  
  m_gc->set_foreground(m_black);
  m_win->draw_rectangle(m_gc, false, x, 0, m_keywidth, m_keyheight);
  
  if (!on) {
    m_gc->set_foreground(m_grey1);
    m_win->draw_line(m_gc, x + 1, 1, x + 1, m_keyheight - 1);
    
    if (k % 12 != 4 && k % 12 != 11 && k != 127 &&
	k != (m_octaves + m_octave_offset) * 12) {
      m_win->draw_line(m_gc, x + m_keywidth - m_bkeywidth / 2, m_bkeyheight,
                       x + m_keywidth - m_bkeywidth / 2 + m_bkeywidth - 1, 
                       m_bkeyheight);
      m_gc->set_foreground(m_grey2);
      m_win->draw_line(m_gc, x + m_keywidth - m_bkeywidth / 2 - 1, 1,
                       x + m_keywidth - m_bkeywidth / 2 - 1, m_bkeyheight - 1);
    }
    
    if (k % 12 != 5 && k % 12 != 0) {
      m_gc->set_foreground(m_grey1);
      m_win->draw_line(m_gc, x - m_bkeywidth / 2 + m_bkeywidth, 1,
                       x - m_bkeywidth / 2 + m_bkeywidth, m_bkeyheight - 1);
      m_win->draw_line(m_gc, x + 1, m_bkeyheight,
                       x - m_bkeywidth / 2 + m_bkeywidth - 1, m_bkeyheight);
    }
    
    m_gc->set_foreground(m_grey2);
    m_win->draw_line(m_gc, x + m_keywidth - 1, 1, 
                     x + m_keywidth - 1, m_keyheight - 1);
    m_win->draw_line(m_gc, x + 1, m_keyheight - 1, 
                     x + m_keywidth - 1, m_keyheight - 1);
    m_gc->set_foreground(m_black);
    m_win->draw_point(m_gc, x + m_keywidth - 1, m_keyheight - 1);
    m_win->draw_point(m_gc, x + 1, m_keyheight - 1);
  }
  
  if (k % 12 == 0) {
    m_gc->set_foreground(m_grey2);
    RefPtr<Pango::Layout> l = Pango::Layout::create(get_pango_context());
    ostringstream oss;
    oss<<int(k / 12);
    l->set_text(oss.str());
    Pango::FontDescription fd;
    fd.set_family("monospace");
    fd.set_absolute_size(m_text_size);
    l->set_font_description(fd);
    Pango::Rectangle r = l->get_pixel_logical_extents();
    int ly = m_keyheight - r.get_height() - (on ? 1 : 2);
    m_win->draw_layout(m_gc, x + 2, ly, l);
  }
}


void Keyboard::draw_black_key(int x, bool on) {
  m_gc->set_foreground(on ? m_green2 : m_black);
  m_win->draw_rectangle(m_gc, true, x - m_bkeywidth / 2, 0, 
                        m_bkeywidth - 1, m_bkeyheight - 1);
  m_gc->set_foreground(m_black);
  m_win->draw_rectangle(m_gc, false, x - m_bkeywidth / 2, 0, 
                        m_bkeywidth - 1, m_bkeyheight - 1);
  
  if (!on) {
    m_gc->set_foreground(m_grey1);
    m_win->draw_line(m_gc, x - m_bkeywidth / 2 + 1, 1, 
                     x - m_bkeywidth / 2 + 1, m_bkeyheight - 2);
    m_gc->set_foreground(m_grey2);
    m_win->draw_line(m_gc, x - m_bkeywidth / 2 + m_bkeywidth - 2, 1, 
                     x - m_bkeywidth / 2 + m_bkeywidth - 2, 
                     m_bkeyheight - 2);
    m_win->draw_line(m_gc, x - m_bkeywidth / 2 + 1, m_bkeyheight - 2, 
                     x - m_bkeywidth / 2 + m_bkeywidth - 2, m_bkeyheight - 2);
  }
}


void Keyboard::key_on(unsigned char key) {
  assert(key < 128);
  bool changed = !m_keys_on[key];
  if (changed) {
    int x, y, width, height;
    key_to_rect(key, x, y, width, height);
    queue_draw_area(x, y, width, height);
    m_signal_key_on(key);
    m_keys_on[key] = true;
  }
}


void Keyboard::key_off(unsigned char key) {
  assert(key < 128);
  bool changed = m_keys_on[key];
  if (changed) {
    int x, y, width, height;
    key_to_rect(key, x, y, width, height);
    queue_draw_area(x, y, width, height);
    m_signal_key_off(key);
    m_keys_on[key] = false;
  }
}


bool Keyboard::get_key_state(unsigned char key) const {
  return m_keys_on[key];
}


sigc::signal<void, unsigned char>& Keyboard::signal_key_on() {
  return m_signal_key_on;
}


sigc::signal<void, unsigned char>& Keyboard::signal_key_off() {
  return m_signal_key_off;
}


unsigned char Keyboard::pixel_to_key(int x, int y, bool only_white_keys,
                                     bool clamp) const {
  if (clamp) {
    if (x < 0)
      return m_octave_offset * 12;
    if (x > int((m_octaves * 7 + 1) * m_keywidth + 1)) {
      unsigned int k = (m_octaves + m_octave_offset) * 12;
      return k < 128 ? k : 127;
    }
  }
  
  if (x < 0 || y < 0 || 
      x > int((m_octaves * 7 + 1) * m_keywidth + 1) || y > int(m_keyheight))
    return 255;
  
  unsigned char k = 255;
  
  int note = (x / m_keywidth) % 7;
  int octave = x / (m_keywidth * 7);
  
  // which white key is it on?
  switch (note) {
  case 0: k = 0; break;
  case 1: k = 2; break;
  case 2: k = 4; break;
  case 3: k = 5; break;
  case 4: k = 7; break;
  case 5: k = 9; break;
  case 6: k = 11; break;
  }
  
  // is it on a black key?
  if (!only_white_keys && y < int(m_bkeyheight)) {
    if (x % m_keywidth < (m_bkeywidth / 2) && k != 0 && k != 5)
      --k;
    else if (x % m_keywidth > (m_keywidth - m_bkeywidth / 2) &&
             k != 4 && k != 11 && (x / m_keywidth) != m_octaves * 12)
      ++k;
  }
  
  unsigned int i = 12 * (m_octave_offset + octave) + k;
  
  return i < 128 ? i : 255;
}


void Keyboard::key_to_rect(unsigned char k, int& x, int& y, 
                           int& width, int& height) const {
  
  // get the white key that the key is closest to
  int a = 0;
  switch (k % 12) {
  case 0: a = 0; break;
  case 1: a = 1; break;
  case 2: a = 1; break;
  case 3: a = 2; break;
  case 4: a = 2; break;
  case 5: a = 3; break;
  case 6: a = 4; break;
  case 7: a = 4; break;
  case 8: a = 5; break;
  case 9: a = 5; break;
  case 10: a = 6; break;
  case 11: a = 6; break;
  }
  a += 7 * (int(k / 12) - m_octave_offset);
  
  // is it a black key?
  if (k % 12 == 1 || k % 12 == 3 || 
      k % 12 == 6 || k % 12 == 8 || k % 12 == 10) {
    x = a * m_keywidth - m_bkeywidth / 2;
    y = 0;
    width = m_bkeywidth;
    height = m_bkeyheight;
  }
  
  // or is it a white key
  else {
    x = a * m_keywidth;
    y = 0;
    width = m_keywidth;
    height = m_keyheight;
  }
}


bool Keyboard::is_black(unsigned char k) const {
  return (k % 12 == 1 || k % 12 == 3 || k % 12 == 6 || 
          k % 12 == 8 || k % 12 == 10);
}
