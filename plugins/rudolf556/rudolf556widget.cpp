/****************************************************************************
    
    rudolf556widget.cpp - GUI for the Rudolf 556 drum machine
    
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

#include <cstring>

#include "rudolf556widget.hpp"


using namespace Gdk;
using namespace Glib;
using namespace Gtk;
using namespace sigc;
using namespace std;


Rudolf556Widget::Rudolf556Widget(const string& bundle) 
  : m_bundle(bundle),
    m_controls(18),
    m_active_control(18) {
  
  set_size_request(300, 257);
  add_events(SCROLL_MASK | BUTTON_PRESS_MASK);
  
  m_controls[0].x = 40.5;
  m_controls[0].y = 68.5;
  m_controls[1].x = 40.5;
  m_controls[1].y = 99;
  m_controls[2].x = 40.5;
  m_controls[2].y = 129;
  m_controls[3].x = 72.8;
  m_controls[3].y = 68.5;
  m_controls[4].x = 72.8;
  m_controls[4].y = 99;
  m_controls[5].x = 72.8;
  m_controls[5].y = 129;
  m_controls[6].x = 131;
  m_controls[6].y = 68.5;
  m_controls[7].x = 131;
  m_controls[7].y = 99;
  m_controls[8].x = 131;
  m_controls[8].y = 129;
  m_controls[9].x = 163.5;
  m_controls[9].y = 68.5;
  m_controls[10].x = 163.5;
  m_controls[10].y = 99;
  m_controls[11].x = 163.5;
  m_controls[11].y = 129;
  m_controls[12].x = 225;
  m_controls[12].y = 68.5;
  m_controls[13].x = 225;
  m_controls[13].y = 99;
  m_controls[14].x = 225;
  m_controls[14].y = 129;
  m_controls[15].x = 258;
  m_controls[15].y = 68.5;
  m_controls[16].x = 258;
  m_controls[16].y = 99;
  m_controls[17].x = 258;
  m_controls[17].y = 129; 
}


void Rudolf556Widget::set_control(unsigned control, float value) {
  if (control - 1 >= m_controls.size())
    return;
  value = value < 0 ? 0 : value;
  value = value > 1 ? 1 : value;
  m_controls[control - 1].value = value;
  queue_draw();
}


void Rudolf556Widget::on_realize() {
  DrawingArea::on_realize();
  RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_file(m_bundle + "rudolf556.png");
  int w = pixbuf->get_width();
  int h = pixbuf->get_height();
  RefPtr<Pixmap> pixmap = Pixmap::create(get_window(), w, h);
  RefPtr<Bitmap> bitmap;
  pixbuf->render_pixmap_and_mask(pixmap, bitmap, 1);
  RefPtr<GC> gc = GC::create(pixmap);
  Gdk::Color bg = get_style()->get_bg(STATE_NORMAL);
  gc->set_foreground(bg);
  pixmap->draw_rectangle(gc, true, 0, 0, w, h);
  pixmap->draw_pixbuf(gc, pixbuf, 0, 0, 0, 0, w, h, RGB_DITHER_NONE, 0, 0);
  RefPtr<Style> s = get_style()->copy();
  s->set_bg_pixmap(STATE_NORMAL, pixmap);
  s->set_bg_pixmap(STATE_ACTIVE, pixmap);
  s->set_bg_pixmap(STATE_PRELIGHT, pixmap);
  s->set_bg_pixmap(STATE_SELECTED, pixmap);
  s->set_bg_pixmap(STATE_INSENSITIVE, pixmap);
  set_style(s);
  get_window()->shape_combine_mask(bitmap, 0, 0);
}


bool Rudolf556Widget::on_expose_event(GdkEventExpose* event) {
  
  RefPtr<Gdk::Window> win = get_window();
  ::Cairo::RefPtr< ::Cairo::Context > cc = win->create_cairo_context();
  cc->set_line_cap(::Cairo::LINE_CAP_ROUND);
  
  float a = 0.125;
  for (unsigned i = 0; i < m_controls.size(); ++i) {
    float const& x = m_controls[i].x;
    float const& y = m_controls[i].y;
    float value = m_controls[i].value;
    value = value < 0 ? 0 : value;
    value = value > 1 ? 1 : value;
    cc->save();
    cc->translate(x, y);
    cc->rotate((0.25 + a + 0.75 * value) * 2 * M_PI);
    cc->move_to(12, 0);
    cc->line_to(14, 0);
    cc->restore();
    cc->set_source_rgba(0, 0, 0, 1);
    cc->set_line_width(4);
    cc->stroke();
    if (i == m_active_control) {
      cc->arc(x, y, 9.5, 0.0, 2 * M_PI);
      cc->set_source_rgba(1, 1, 0, 1);
      cc->set_line_width(2);
      cc->stroke();
    }
  }
  
  return true;
}


bool Rudolf556Widget::on_button_press_event(GdkEventButton* event) {
  if (event->button != 1)
    return false;
  int x = event->x;
  int y = event->y;
  
  // check if it's in the drag area
  if (x >= 10 && x <= 24 && y >= 14 && y <= 38) {
    vector<TargetEntry> dnd_targets;
    dnd_targets.push_back(TargetEntry("x-org.nongnu.ll-plugins/keynames"));
    dnd_targets.push_back(TargetEntry("text/plain"));
    drag_begin(TargetList::create(dnd_targets), Gdk::ACTION_COPY, 1, 
	       reinterpret_cast<GdkEvent*>(event));
  }
  
  unsigned c = find_control(x, y);
  if (c < m_controls.size()) {
    m_active_control = c;
    m_deactivate_conn.disconnect();
    m_deactivate_conn = signal_timeout().
      connect(mem_fun(*this, &Rudolf556Widget::deactivate_controls), 2000);
    queue_draw();
  }
  return true;
}


bool Rudolf556Widget::on_scroll_event(GdkEventScroll* event) {
  int x = event->x;
  int y = event->y;
  unsigned c = find_control(x, y);
  if (c >= m_controls.size())
    return true;
  
  m_active_control = c;
  m_deactivate_conn.disconnect();
  m_deactivate_conn = signal_timeout().
    connect(mem_fun(*this, &Rudolf556Widget::deactivate_controls), 2000);
  
  float step = 0.1;
  if (event->state & GDK_SHIFT_MASK)
    step *= 0.1;
  if (event->direction == GDK_SCROLL_UP) {
    m_controls[c].value += step;
    m_controls[c].value = m_controls[c].value > 1 ? 1 : m_controls[c].value;
    signal_control_changed(c + 1, m_controls[c].value);
    queue_draw();
  }
  else if (event->direction == GDK_SCROLL_DOWN) {
    m_controls[c].value -= step;
    m_controls[c].value = m_controls[c].value < 0 ? 0 : m_controls[c].value;
    signal_control_changed(c + 1, m_controls[c].value);
    queue_draw();
  }
  
  return true;
}


void Rudolf556Widget::on_drag_data_get(const RefPtr<DragContext>& context,
				       SelectionData& selection_data, 
				       guint info, guint time) {
  static char const keynames[] = 
    "60 Bass 1\n"
    "62 Bass 2\n"
    "64 Snare 1\n"
    "65 Snare 2\n"
    "67 Hihat 1\n"
    "69 Hihat 2\n";
  selection_data.set(selection_data.get_target(), 8, 
		     reinterpret_cast<const guint8*>(keynames),
		     strlen(keynames));
}


unsigned Rudolf556Widget::find_control(float x, float y) {
  for (unsigned i = 0; i < m_controls.size(); ++i) {
    float d = sqrt(pow(x - m_controls[i].x, 2) + pow(y - m_controls[i].y, 2));
    if (d < 15)
      return i;
  }
  return m_controls.size();
}


bool Rudolf556Widget::deactivate_controls() {
  m_active_control = m_controls.size();
  queue_draw();
  return false;
}
