/****************************************************************************
    
    main.cpp - the main file for the Sineshaper GUI
    
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
#include <libglademm.h>
#include <ladspa.h>

#include "lv2uiclient.hpp"
#include "skindial_gtkmm.hpp"
#include "sineshapergui.hpp"


using namespace std;
using namespace Gdk;
using namespace Gtk;
using namespace Gnome::Glade;
using namespace Glib;


int main(int argc, char** argv) {
  
  // create the objects
  LV2UIClient lv2(argv[1], argv[3], argv[2], argv[4], true);
  if (!lv2.is_valid()) {
    cerr<<"Could not start OSC listener. You are not running the "<<endl
        <<"program manually, are you? It's supposed to be started by "<<endl
        <<"the plugin host."<<endl;
    return 1;
  }
  Main kit(argc, argv);
  RefPtr<Xml> xml = Xml::create(lv2.get_bundle_path() + "/sineshaper.glade");
  SineShaperGUI* main_win = xml->get_widget_derived("main_win", main_win);
  main_win->set_title(string("Sineshaper: ") + lv2.get_identifier());
  main_win->set_bundle_path(lv2.get_bundle_path());
  main_win->init();
  
  // connect signals
  main_win->connect_all(lv2);
  main_win->signal_select_program.
    connect(mem_fun(lv2, &LV2UIClient::send_program));
  main_win->signal_programs_changed.
    connect(bind<0>(bind<0>(mem_fun(lv2, &LV2UIClient::send_configure), 
                            "reloadprograms"), ""));
  lv2.program_received.
    connect(mem_fun(*main_win, &SineShaperGUI::program_selected));
  lv2.show_received.connect(mem_fun(*main_win, &SineShaperGUI::show_all));
  lv2.hide_received.connect(mem_fun(*main_win, &SineShaperGUI::hide));
  lv2.quit_received.connect(&Main::quit);
  lv2.add_program_received.
    connect(mem_fun(*main_win, &SineShaperGUI::add_program));
  lv2.remove_program_received.
    connect(mem_fun(*main_win, &SineShaperGUI::remove_program));
  lv2.clear_programs_received.
    connect(mem_fun(*main_win, &SineShaperGUI::clear_programs));
  main_win->signal_delete_event().connect(bind_return(hide(&Main::quit), true));
  
  // start
  lv2.send_update_request();
  Main::run();
  
  return 0;
}


