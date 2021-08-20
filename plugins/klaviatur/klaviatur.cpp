/****************************************************************************
    
    klaviatur.cpp - The plugin part for a MIDI keyboard plugin
    
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
#include <lv2plugin.hpp>
#include "klaviatur.peg"
#include "lv2_event_helpers.h"


using namespace LV2;
using namespace std;


/** This plugin is really just a simple MIDI THRU. The GUI writes events
    to the input port and the plugin copies them to the output. */
class Klaviatur : public Plugin<Klaviatur, URIMap<true>, EventRef<true> > {
public:
  
  Klaviatur(double rate) 
    : Plugin<Klaviatur, URIMap<true>, EventRef<true> >(k_n_ports),
      m_midi_type(uri_to_id(LV2_EVENT_URI,
			    "http://lv2plug.in/ns/ext/midi#MidiEvent")) {
    
  }
  
  
  void run(uint32_t nframes) {
    
    // initialise event buffer iterators
    LV2_Event_Buffer* inbuf = p<LV2_Event_Buffer>(k_midi_input);
    LV2_Event_Buffer* outbuf = p<LV2_Event_Buffer>(k_midi_output);
    lv2_event_buffer_reset(outbuf, inbuf->stamp_type, outbuf->data);
    LV2_Event_Iterator in, out;
    lv2_event_begin(&in, inbuf);
    lv2_event_begin(&out, outbuf);
    
    // copy events
    while (lv2_event_is_valid(&in)) {
      uint8_t* data;
      LV2_Event* ev = lv2_event_get(&in, &data);
      lv2_event_increment(&in);
      if (ev->type == 0)
	event_unref(ev);
      if (ev->type == m_midi_type && ev->size == 3)
	lv2_event_write_event(&out, ev, data);
    }
    
  }
  
protected:
  
  uint32_t m_midi_type;
  
};


static unsigned _ = Klaviatur::register_class(k_uri);
