/****************************************************************************
    
    control2midi.cpp - A plugin that converts control input to MIDI events
    
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

#include <lv2plugin.hpp>
#include <lv2_event_helpers.h>

#include "control2midi.peg"


using namespace LV2;


/** This plugin generates MIDI CC events when the control input changes. */
class Control2MIDI : public Plugin<Control2MIDI, URIMap<true> > {
public:
  
  Control2MIDI(double) 
    : Plugin<Control2MIDI, URIMap<true> >(c_n_ports),
      m_last_value(0),
      m_last_cc(0),
      m_midi_type(uri_to_id(LV2_EVENT_URI,
			    "http://lv2plug.in/ns/ext/midi#MidiEvent")) { 
    
  }
  
  void run(uint32_t sample_count) {
    
    // get control values and initialise output iterator
    float& value = *p(c_input);
    float& min = *p(c_min);
    float& max = *p(c_max);
    float cc_float = *p(c_cc);
    cc_float = cc_float < 0 ? 0 : (cc_float > 127 ? 127 : cc_float);
    unsigned char cc = static_cast<unsigned char>(cc_float);
    LV2_Event_Buffer* midi = p<LV2_Event_Buffer>(c_output);
    lv2_event_buffer_reset(midi, midi->stamp_type, midi->data);
    LV2_Event_Iterator iter;
    lv2_event_begin(&iter, midi);
    
    // check range sanity
    if (max - min < 0.001)
      max = min + 0.001;
    if (value < min)
      value = min;
    else if (value > max)
      value = max;
    
    // if the input has changed, write a single MIDI CC event
    unsigned char cvalue = (unsigned char)((value - min) * 127 / (max - min));
    if (cc != m_last_cc || cvalue != m_last_value) {
      unsigned char bytes[] = { 0xB0, cc, cvalue };
      lv2_event_write(&iter, 0, 0, m_midi_type, 3, bytes);
      m_last_cc = cc;
      m_last_value = cvalue;
    }
    
  }
  
protected:
  
  unsigned char m_last_value;
  unsigned char m_last_cc;
  uint32_t m_midi_type;
  
};


static unsigned _ = Control2MIDI::register_class(c_uri);
