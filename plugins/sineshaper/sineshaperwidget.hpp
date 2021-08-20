/****************************************************************************
    
    sineshaperwidget.hpp - A GUI for the Sineshaper LV2 synth plugin
    
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

#ifndef SINESHAPERWIDGET_HPP
#define SINESHAPERWIDGET_HPP

#include <string>
#include <vector>

#include <stdint.h>

#include <gtkmm.h>

#include "skindial_gtkmm.hpp"


class SineshaperWidget : public Gtk::HBox {
public:
  
  SineshaperWidget(const std::string& bundle, bool show_programs);
  
  void set_control(uint32_t port, float value);
  
  void add_preset(uint32_t number, const char* name);
  
  void remove_preset(uint32_t number);
  
  void clear_presets();
  
  void set_preset(uint32_t number);
  
  sigc::signal<void, uint32_t, float> signal_control_changed;
  
  sigc::signal<void, uint32_t> signal_preset_changed;
  
  sigc::signal<void, uint32_t, const char*> signal_save_preset;
  
protected:
  
  class PresetColumns : public TreeModel::ColumnRecord {
  public:
    TreeModelColumn<unsigned> number;
    TreeModelColumn<Glib::ustring> name;
    
    PresetColumns() { 
      add(number); 
      add(name);
    }
  } m_preset_columns;
  

  Gtk::Widget* init_tuning_controls();
  Gtk::Widget* init_osc2_controls();
  Gtk::Widget* init_vibrato_controls();
  Gtk::Widget* init_portamento_controls();
  Gtk::Widget* init_tremolo_controls();
  Gtk::Widget* init_envelope_controls();
  Gtk::Widget* init_amp_controls();
  Gtk::Widget* init_delay_controls();
  Gtk::Widget* init_shaper_controls();
  Gtk::Widget* init_preset_list();
  
  Gtk::Widget* create_knob(Gtk::Table* table, int col, const std::string& name, 
			   float min, float max, SkinDial::Mapping mapping,
			   float center, uint32_t port);
  Gtk::Widget* create_spin(Gtk::Table* table, int col, const std::string& name, 
			   float min, float max, uint32_t port);
  Gtk::CheckButton* create_check(Gtk::VBox* vbox, const std::string& name, 
				 uint32_t port);
  
  Gtk::TreeIter find_preset_row(unsigned char number);
  
  void bool_to_control(uint32_t port, bool value);
  
  void do_change_preset();
  
  void show_save();

  void show_about();
  
  
  Glib::RefPtr<Gdk::Pixbuf> m_dialg;
  std::vector<Gtk::Adjustment*> m_adjs;
  Glib::RefPtr<ListStore> m_preset_store;
  Gtk::TreeView* m_view;
  Gtk::CheckButton* m_prt_on;
  Gtk::CheckButton* m_prt_tie;
  std::string m_bundle;
  bool m_show_programs;
  
};


#endif
