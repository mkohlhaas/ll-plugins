/****************************************************************************
    
    sineshaperwidget.cpp - A GUI for the Sineshaper LV2 synth plugin
    
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
#include <limits>

#include "sineshaperwidget.hpp"
#include "skindial_gtkmm.hpp"
#include "sineshaper.peg"


using namespace Gtk;
using namespace std;


namespace {
  
  class BFrame : public Frame {
  public:
    BFrame(const string& label) {
      Label* lbl = manage(new Label(string("<b>") + label + "</b>"));
      lbl->set_use_markup(true);
      set_label_widget(*lbl);
    }
  };
  
  class SLabel : public Label {
  public:
    SLabel(const string& label) 
      : Label(string("<small>") + label + "</small>") {
      set_use_markup(true);
    }
  };

}


SineshaperWidget::SineshaperWidget(const std::string& bundle, 
				   bool show_programs)
  : HBox(false, 6),
    m_adjs(s_n_ports, 0),
    m_bundle(bundle),
    m_show_programs(show_programs) {
  
  set_border_width(6);
  
  m_dialg = Gdk::Pixbuf::create_from_file(bundle + "dial.png");
  
  VBox* knob_vbox = manage(new VBox(false, 6));
  
  Table* table = manage(new Table(3, 2));
  table->set_spacings(6);
  table->attach(*init_tuning_controls(), 0, 1, 0, 1);
  table->attach(*init_osc2_controls(), 1, 2, 0, 1);
  table->attach(*init_vibrato_controls(), 0, 1, 1, 2);
  table->attach(*init_portamento_controls(), 1, 2, 1, 2);
  table->attach(*init_tremolo_controls(), 0, 1, 2, 3);
  table->attach(*init_envelope_controls(), 1, 2, 2, 3);
  
  HBox* knob_hbox = manage(new HBox(false, 6));
  knob_hbox->pack_start(*init_amp_controls());
  knob_hbox->pack_start(*init_delay_controls());
  
  knob_vbox->pack_start(*table);
  knob_vbox->pack_start(*init_shaper_controls());
  knob_vbox->pack_start(*knob_hbox);
  
  pack_start(*knob_vbox);
  
  if (m_show_programs) {
    VBox* preset_vbox = manage(new VBox(false, 6));
    preset_vbox->pack_start(*init_preset_list(), PACK_EXPAND_WIDGET);
    Button* save = manage(new Button("Save preset"));
    save->signal_clicked().
      connect(mem_fun(*this, &SineshaperWidget::show_save));
    preset_vbox->pack_start(*save, PACK_SHRINK);
    Button* about = manage(new Button("About Sineshaper"));
    about->signal_clicked().
      connect(mem_fun(*this, &SineshaperWidget::show_about));
    preset_vbox->pack_start(*about, PACK_SHRINK);
    
    pack_start(*preset_vbox);
  }
  
}


void SineshaperWidget::set_control(uint32_t port, float value) {
  if (port == s_prt_on)
    m_prt_on->set_active(value > 0);
  else if (port == s_prt_tie)
    m_prt_tie->set_active(value > 0);
  if (port < m_adjs.size() && m_adjs[port])
    m_adjs[port]->set_value(value);
}


void SineshaperWidget::add_preset(uint32_t number, const char* name) {
  if (m_show_programs) {
    remove_preset(number);
    ListStore::iterator iter = m_preset_store->append();
    (*iter)[m_preset_columns.number] = number;
    (*iter)[m_preset_columns.name] = name;
  }
}
  

void SineshaperWidget::remove_preset(uint32_t number) {
  if (m_show_programs) {
    TreeNodeChildren c = m_preset_store->children();
    for (TreeIter iter = c.begin(); iter != c.end(); ++iter) {
      if ((*iter)[m_preset_columns.number] == number) {
	m_preset_store->erase(iter);
	break;
      }
    }
  }
}
  

void SineshaperWidget::clear_presets() {
  if (m_show_programs)
    m_preset_store->clear();
}
  

void SineshaperWidget::set_preset(uint32_t number) {
  if (m_show_programs) {
    if (number > 127)
      m_view->get_selection()->unselect_all();
    else {
      TreeNodeChildren c = m_preset_store->children();
      for (TreeIter iter = c.begin(); iter != c.end(); ++iter) {
	if ((*iter)[m_preset_columns.number] == number) {
	  m_view->get_selection()->select(iter);
	  break;
	}
      }
    }
  }
}
  

Widget* SineshaperWidget::init_tuning_controls() {
  Frame* frame = manage(new BFrame("Tuning"));
  frame->set_shadow_type(SHADOW_IN);
  
  Table* table = new Table(2, 2);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Tune", 0.5, 2.0, SkinDial::DoubleLog, 1.0, s_tun);
  create_spin(table, 1, "Octave", -10, 10, s_oct);
  
  return frame;
}


Widget* SineshaperWidget::init_osc2_controls() {
  Frame* frame = manage(new BFrame("Oscillator 2"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 3);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Tune", 0.5, 2.0, SkinDial::DoubleLog, 1.0, s_sub_tun);
  create_spin(table, 1, "Octave", -10, 10, s_sub_oct);
  create_knob(table, 2, "Mix", 0.0, 1.0, SkinDial::Linear, 0.5, s_osc_mix);

  return frame;
}


Widget* SineshaperWidget::init_vibrato_controls() {
  Frame* frame = manage(new BFrame("Vibrato"));
  frame->set_shadow_type(SHADOW_IN);
  
  Table* table = new Table(2, 2);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Freq", 0.0, 10.0, SkinDial::Linear, 1.0, s_vib_frq);
  create_knob(table, 1, "Depth", 0.0, 0.25, SkinDial::Linear, 0.1, s_vib_dpt);
  
  return frame;
}


Widget* SineshaperWidget::init_portamento_controls() {
  Frame* frame = manage(new BFrame("Portamento"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 2);
  table->set_col_spacings(3);
  frame->add(*table);
  
  VBox* vbox = manage(new VBox(false, 3));
  table->attach(*vbox, 0, 1, 0, 2);
  m_prt_on = create_check(vbox, "Portamento on", s_prt_on);
  m_prt_tie = create_check(vbox, "Tie overlapping notes", s_prt_tie);
  create_knob(table, 1, "Time", 0.001, 3, SkinDial::Logarithmic, 1, s_prt_tim);
  
  return frame;
}


Widget* SineshaperWidget::init_tremolo_controls() {
  Frame* frame = manage(new BFrame("Tremolo"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 2);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Freq", 0.0, 10.0, SkinDial::Linear, 1.0, s_trm_frq);
  create_knob(table, 1, "Depth", 0.0, 1.0, SkinDial::Linear, 0.1, s_trm_dpt);

  return frame;
}


Widget* SineshaperWidget::init_envelope_controls() {
  Frame* frame = manage(new BFrame("Envelope"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 4);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Attack", 0.0005, 1.0, SkinDial::Logarithmic, 1.0, 
	      s_att);
  create_knob(table, 1, "Decay", 0.0005, 1.0, SkinDial::Logarithmic, 1.0, 
	      s_dec);
  create_knob(table, 2, "Sustain", 0.0, 1.0, SkinDial::Linear, 1.0, s_sus);
  create_knob(table, 3, "Release", 0.0005, 3.0, SkinDial::Logarithmic, 1.0, 
	      s_rel);

  return frame;
}


Widget* SineshaperWidget::init_amp_controls() {
  Frame* frame = manage(new BFrame("Amp"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 3);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Env", 0.0, 1.0, SkinDial::Linear, 1.0, s_amp_env);
  create_knob(table, 1, "Drive", 0.0, 1.0, SkinDial::Linear, 1.0, s_drive);
  create_knob(table, 2, "Gain", 0.0, 2.0, SkinDial::Linear, 1.0, s_gain);

  return frame;
}


Widget* SineshaperWidget::init_delay_controls() {
  Frame* frame = manage(new BFrame("Delay"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 3);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Time", 0.0, 3.0, SkinDial::Linear, 1.0, s_del_tim);
  create_knob(table, 1, "Feedback", 0.0, 1.0, SkinDial::Linear, 1.0, s_del_fb);
  create_knob(table, 2, "Mix", 0.0, 1.0, SkinDial::Linear, 1.0, s_del_mix);

  return frame;
}


Widget* SineshaperWidget::init_shaper_controls() {
  Frame* frame = manage(new BFrame("Shaper"));
  frame->set_shadow_type(SHADOW_IN);

  Table* table = new Table(2, 6);
  table->set_col_spacings(3);
  frame->add(*table);
  
  create_knob(table, 0, "Env", 0.0, 1.0, SkinDial::Linear, 1.0, s_shp_env);
  create_knob(table, 1, "Total", 0.0, 6.0, SkinDial::Linear, 1.0, s_shp_tot);
  create_knob(table, 2, "Split", 0.0, 1.0, SkinDial::Linear, 1.0, s_shp_spl);
  create_knob(table, 3, "Shift", 0.0, 1.0, SkinDial::Linear, 1.0, s_shp_shf);
  create_knob(table, 4, "Freq", 0.0, 10.0, SkinDial::Linear, 1.0, s_lfo_frq);
  create_knob(table, 5, "Depth", 0.0, 1.0, SkinDial::Linear, 1.0, s_lfo_dpt);

  return frame;
}


Widget* SineshaperWidget::init_preset_list() {
  Frame* frame = manage(new BFrame("Presets"));
  frame->set_shadow_type(SHADOW_NONE);
  
  m_preset_store = ListStore::create(m_preset_columns);
  m_preset_store->set_sort_column_id(m_preset_columns.number, SORT_ASCENDING);
  ScrolledWindow* scrw = manage(new ScrolledWindow);
  scrw->set_shadow_type(SHADOW_IN);
  scrw->set_policy(POLICY_NEVER, POLICY_AUTOMATIC);
  m_view = manage(new TreeView(m_preset_store));
  m_view->set_rules_hint(true);
  m_view->append_column("No", m_preset_columns.number);
  m_view->append_column("Name", m_preset_columns.name);
  m_view->set_headers_visible(false);
  m_view->get_selection()->signal_changed().
    connect(mem_fun(*this, &SineshaperWidget::do_change_preset));
  
  scrw->add(*m_view);
  
  frame->add(*scrw);
  
  return frame;
}


Gtk::Widget* SineshaperWidget::create_knob(Gtk::Table* table, int col, 
					   const std::string& name, 
					   float min, float max, 
					   SkinDial::Mapping mapping,
					   float center, uint32_t port) {
  SkinDial* skd = manage(new SkinDial(min, max, m_dialg, mapping, center));
  table->attach(*skd, col, col + 1, 0, 1);
  Label* lbl = manage(new SLabel(name));
  table->attach(*lbl, col, col + 1, 1, 2);
  m_adjs[port] = skd->get_adjustment();
  slot<float> get_value = mem_fun(*skd->get_adjustment(), 
				  &Adjustment::get_value);
  slot<void, float> sccf = bind<0>(signal_control_changed, port);
  slot<void> scc = compose(sccf, get_value);
  skd->get_adjustment()->signal_value_changed().connect(scc);

  return skd;
}


Gtk::Widget* SineshaperWidget::create_spin(Gtk::Table* table, int col, 
					   const std::string& name, 
					   float min, float max, 
					   uint32_t port) {
  SpinButton* spb = manage(new SpinButton(1));
  spb->set_range(-10, 10);
  spb->set_increments(1, 1);
  table->attach(*spb, col, col + 1, 0, 1);
  Label* lbl = manage(new SLabel(name));
  table->attach(*lbl, col, col + 1, 1, 2);
  m_adjs[port] = spb->get_adjustment();
  slot<float> get_value = mem_fun(*spb->get_adjustment(), 
				  &Adjustment::get_value);
  slot<void, float> sccf = bind<0>(signal_control_changed, port);
  slot<void> scc = compose(sccf, get_value);
  spb->get_adjustment()->signal_value_changed().connect(scc);

  return spb;
}


Gtk::CheckButton* SineshaperWidget::create_check(Gtk::VBox* vbox, 
						 const std::string& name, 
						 uint32_t port) {
  CheckButton* chk = manage(new CheckButton);
  Label* lbl = manage(new SLabel(name));
  chk->add(*lbl);
  vbox->pack_start(*chk);
  slot<void, uint32_t, bool> btc = 
    mem_fun(*this, &SineshaperWidget::bool_to_control);
  slot<void, bool> btc2 = bind<0>(btc, port);
  slot<void> st = compose(btc2, mem_fun(*chk, &CheckButton::get_active));
  chk->signal_toggled().connect(st);
  return chk;
}
 
 
void SineshaperWidget::do_change_preset() {
  if (m_view->get_selection()->count_selected_rows() == 0)
    signal_preset_changed(numeric_limits<uint32_t>::max());
  else
    signal_preset_changed(m_view->get_selection()->
			  get_selected()->get_value(m_preset_columns.number));
}


Gtk::TreeIter SineshaperWidget::find_preset_row(unsigned char number) {
  TreeStore::Children c = m_preset_store->children();
  for (TreeStore::Children::iterator i = c.begin(); i != c.end(); ++i) {
    if ((*i)[m_preset_columns.number] == number)
      return i;
  }
  return c.end();
}


void SineshaperWidget::bool_to_control(uint32_t port, bool value) {
  if (value)
    signal_control_changed(port, 1);
  else
    signal_control_changed(port, 0);
}


void SineshaperWidget::show_save() {
  Dialog dlg("Save preset");
  dlg.add_button(Stock::CANCEL, RESPONSE_CANCEL);
  dlg.add_button(Stock::OK, RESPONSE_OK);
  Table tbl(2, 2);
  tbl.set_col_spacings(3);
  tbl.set_row_spacings(3);
  tbl.set_border_width(3);
  Label lbl1("Name:");
  Label lbl2("Number:");
  Entry ent;
  Adjustment adj(0, 0, 127);
  SpinButton spb(adj);
  TreeIter selected = m_view->get_selection()->get_selected();
  if (selected != m_preset_store->children().end())
    spb.set_value((*selected)[m_preset_columns.number]);
  tbl.attach(lbl1, 0, 1, 0, 1);
  tbl.attach(lbl2, 0, 1, 1, 2);
  tbl.attach(ent, 1, 2, 0, 1);
  tbl.attach(spb, 1, 2, 1, 2);
  dlg.get_vbox()->pack_start(tbl);
  dlg.show_all();
  while (dlg.run() == RESPONSE_OK) {
    if (find_preset_row((unsigned char)adj.get_value())) {
      MessageDialog msg("There is already a preset with this number. Are you "
			"sure that you want to overwrite it?", false,
			MESSAGE_QUESTION, BUTTONS_YES_NO);
      msg.show_all();
      if (msg.run() == RESPONSE_NO)
	continue;
    }
    signal_save_preset((uint32_t)adj.get_value(), ent.get_text().c_str());
    break;
  }
}


void SineshaperWidget::show_about() {
  AboutDialog dlg;
  dlg.set_name("Sineshaper");
  dlg.set_version(VERSION);
  dlg.set_logo(Gdk::Pixbuf::create_from_file(m_bundle + "icon.svg", 120, -1));
  dlg.set_copyright("\u00a9 2006-2008 Lars Luthman <mail@larsluthman.net>");
  dlg.set_website("http://ll-plugins.nongnu.org");
  dlg.set_license("This program is free software: you can redistribute it and/or modify\n"
		  "it under the terms of the GNU General Public License as published by\n"
		  "the Free Software Foundation, either version 3 of the License, or\n"
		  "(at your option) any later version.\n"
		  "\n"
		  "This program is distributed in the hope that it will be useful,\n"
		  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		  "GNU General Public License for more details.\n"
		  "\n"
		  "You should have received a copy of the GNU General Public License\n"
		  "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
  dlg.show();
  dlg.run();
}
