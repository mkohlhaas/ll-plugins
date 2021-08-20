/****************************************************************************
    
    klaviatur_gtk.cpp - GUI for the Klaviatur LV2 plugin
    
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

#include <iostream>
#include <string>

#include <gtkmm.h>

#include "lv2gui.hpp"
#include "klaviatur.peg"
#include "keyboard.hpp"


using namespace sigc;
using namespace Gtk;
using namespace LV2;


namespace {
  
  class SLabel : public Label {
  public:
    SLabel(const std::string& label, const AlignmentEnum& align = ALIGN_LEFT) 
      : Label(std::string("<small>") + label + "</small>", align) {
      set_use_markup(true);
    }
  };

}



class KlaviaturGUI : public GUI<KlaviaturGUI, URIMap<true>, WriteMIDI<true> > {
public:
  
  KlaviaturGUI(const std::string& URI) 
    : m_cc(0, 128, 1),
      m_pitch(-8192, 8192, 1),
      m_vel(1, 128, 1),
      m_kb(3, 3, 17, 10, 55, 36) {
    
    pack_start(m_vbox);
    
    // initialise control widgets
    m_kb.set_flags(m_kb.get_flags() | CAN_FOCUS);
    m_cc.set_digits(0);
    m_cc.set_draw_value(false);
    m_cc.set_value(0);
    m_pitch.set_digits(0);
    m_pitch.set_draw_value(false);
    m_pitch.set_value(0);
    m_vel.set_digits(0);
    m_vel.set_draw_value(false);
    m_vel.set_value(64);
    m_cc_sbn.set_range(0, 127);
    m_cc_sbn.set_increments(1, 16);
    m_cc_sbn.set_digits(0);
    m_cc_sbn.set_snap_to_ticks(true);
    
    // layout
    Table* table = manage(new Table(3, 3));
    table->set_border_width(6);
    table->set_spacings(6);
    table->attach(*manage(new SLabel("CC:", ALIGN_LEFT)), 0, 1, 0, 1, FILL); 
    table->attach(m_cc_sbn, 1, 2, 0, 1, FILL);
    table->attach(m_cc, 2, 3, 0, 1);
    table->attach(*manage(new SLabel("Pitch:", ALIGN_LEFT)), 0, 2, 1, 2, FILL);
    table->attach(m_pitch, 2, 3, 1, 2);
    table->attach(*manage(new SLabel("Velocity:", ALIGN_LEFT)),0, 2, 2, 3, FILL);
    table->attach(m_vel, 2, 3, 2, 3);
    Expander* exp = manage(new Expander);
    exp->set_label_widget(*manage(new SLabel("Controls:")));
    exp->add(*table);
    m_vbox.pack_start(*exp);
    m_vbox.pack_start(m_kb);
    m_kb.grab_focus();
    
    // connect signals
    m_kb.signal_key_on().
      connect(mem_fun(*this, &KlaviaturGUI::handle_keypress));
    m_kb.signal_key_off().
      connect(mem_fun(*this, &KlaviaturGUI::handle_keyrelease));
    m_cc.signal_value_changed().
      connect(mem_fun(*this, &KlaviaturGUI::handle_cc_change));
    m_cc_sbn.signal_value_changed().
      connect(mem_fun(*this, &KlaviaturGUI::handle_cc_change));
    m_pitch.signal_value_changed().
      connect(mem_fun(*this, &KlaviaturGUI::handle_pitch_change));
  }
  
protected:

  void handle_keypress(unsigned char key) {
    unsigned char data[3] = { 0x90, key, uint8_t(m_vel.get_value()) };
    write_midi(k_midi_input, 3, data);
  }
  
  
  void handle_keyrelease(unsigned char key) {
    unsigned char data[3] = { 0x80, key, 64 };
    write_midi(k_midi_input, 3, data);
  }
  
  
  void handle_cc_change() {
    unsigned char data[3] = { 0xB0, uint8_t(m_cc_sbn.get_value()),
                              uint8_t(m_cc.get_value()) };
    write_midi(k_midi_input, 3, data);
  }
  
  
  void handle_pitch_change() {
    int value = int(m_pitch.get_value()) + 8192;
    unsigned char data[3] = { 0xE0, uint8_t(value & 127), uint8_t(value >> 7) };
    write_midi(k_midi_input, 3, data);
  }
  
  
  HScale m_pitch;
  HScale m_cc;
  HScale m_vel;
  SpinButton m_cc_sbn;
  Keyboard m_kb;
  VBox m_vbox;
  
};


static int _ = 
  KlaviaturGUI::register_class((std::string(k_uri) + "/gui").c_str());

