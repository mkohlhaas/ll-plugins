/****************************************************************************
    
    arpeggiator.cpp - simple MIDI arpeggiator plugin
    
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

#include <cstring>

#include <lv2plugin.hpp>
#include <lv2_event_helpers.h>


using namespace std;
using namespace LV2;


/** A plugin that takes chord MIDI input and writes arpeggio MIDI output.
    Since we're dealing with events we need the URIMap and EventRef mixins. */
class Arpeggiator : public Plugin<Arpeggiator, URIMap<true>, EventRef<true> > {
public:
  
  /** Constructor. */
  Arpeggiator(double rate) 
    : Plugin<Arpeggiator, URIMap<true>, EventRef<true> >(4),
      m_rate(rate),
      m_num_keys(0),
      m_frame_counter(0),
      m_next_key(0),
      m_last_key(0),
      m_midi_type(uri_to_id(LV2_EVENT_URI, 
			    "http://lv2plug.in/ns/ext/midi#MidiEvent")),
      m_running(false) {
    
  }
  
  
  /** Reset the arpeggio. */
  void activate() {
    m_num_keys = 0;
    m_frame_counter = 0;
    m_next_key = 0;
  }

  
  /** The run() function reads event input and writes event output is there
      are any held keys. */
  void run(uint32_t nframes) {
    
    // get the arpeggio direction
    bool upwards = *p(1) > 0.5;
    
    // initialise event buffer iterators
    LV2_Event_Iterator iter_in, iter_out;
    lv2_event_begin(&iter_in, p<LV2_Event_Buffer>(2));
    LV2_Event_Buffer* outbuf = p<LV2_Event_Buffer>(3);
    lv2_event_buffer_reset(outbuf, outbuf->stamp_type, outbuf->data);
    lv2_event_begin(&iter_out, outbuf);
    
    // iterate over incoming MIDI events
    double event_time;
    uint32_t event_size;
    unsigned char* event_data;
    double frames_done = 0;
    while (true) {
      
      // get timestamp for next event (or buffer end)
      uint32_t event_time = nframes;
      LV2_Event* ev = 0;
      if (lv2_event_is_valid(&iter_in)) {
	ev = lv2_event_get(&iter_in, &event_data);
	event_time = ev->frames;
	lv2_event_increment(&iter_in);
      }
      
      // generate output events up until next input event
      if (event_time > frames_done)
        generate_events(&iter_out, frames_done, event_time, upwards);
      frames_done = event_time;
      
      // if we're done, exit from the loop
      if (!ev)
	break;
      
      // handle MIDI input event
      if (ev->type == m_midi_type && ev->size == 3) {
        const unsigned char& key = event_data[1];
        
        // note off - erase the key from the array
        if ((event_data[0] & 0xF0) == 0x80) {
          unsigned pos = 0;
          for ( ; pos < m_num_keys && m_keys[pos] != key; ++pos);
          if (pos < m_num_keys) {
            --m_num_keys;
            if (upwards && m_next_key > pos)
              --m_next_key;
	    else if (!upwards && m_next_key == pos) {
	      if (m_next_key == 0)
		m_next_key = m_num_keys - 1;
	      else
		--m_next_key;
	    }
            for (unsigned i = pos; i < m_num_keys; ++i)
              m_keys[i] = m_keys[i + 1];
          }
        }
          
        // note on - insert the key in the array
        else if ((event_data[0] & 0xF0) == 0x90) {
          unsigned pos = 0;
          for ( ; pos < m_num_keys && m_keys[pos] < key; ++pos);
          if (m_keys[pos] != key || pos == m_num_keys) {
            for (unsigned i = m_num_keys; i > pos; --i)
              m_keys[i] = m_keys[i - 1];
            m_keys[pos] = key;
            ++m_num_keys;
            if (m_next_key > pos)
              ++m_next_key;
	    if (pos == m_num_keys - 1 && m_next_key == 0)
	      m_next_key = pos;
          }
	  if (!m_running) {
	    m_running = true;
	    if (upwards)
	      m_next_key = 0;
	    else
	      m_next_key = m_num_keys - 1;
	  }
        }
        
      }
      
      // handle type 0 input event
      else if (ev->type == 0)
	event_unref(ev);
      
    }
    
  }
  
  
  /** This function generates arpeggio output events in the frame offset
      range [from, to), upwards if @c upwards is @c true, downwards 
      otherwise. */
  void generate_events(LV2_Event_Iterator* midi_out,
		       uint32_t from, uint32_t to, bool upwards) {
    
    // don't generate any output if there are no keys held and we've sent out
    // the last Note Off
    if (!m_running)
      return;
    
    // don't generate any output if the note rate is non-positive
    if (*p(0) <= 0) {
      m_frame_counter = 0;
      return;
    }
    
    // compute the number of frames between notes
    uint32_t step_length = 60 * m_rate / *p(0);
    
    // compute the timestamp for next note
    uint32_t frame = from + m_frame_counter;
    frame = frame >= from ? frame : from;
    
    // write the output events
    for ( ; frame < to; frame += step_length) {
      unsigned char data[] = { 0x80, m_last_key, 0x60 };
      lv2_event_write(midi_out, frame, 0, m_midi_type, 3, data);
      if (m_num_keys == 0) {
	m_running = false;
	m_frame_counter = 0;
	return;
      }
      data[0] = 0x90;
      data[1] = m_keys[m_next_key];
      lv2_event_write(midi_out, frame, 0, m_midi_type, 3, data);
      m_last_key = data[1];
      if (upwards)
	m_next_key = (m_next_key + 1) % m_num_keys;
      else
	m_next_key = (m_next_key + m_num_keys - 1) % m_num_keys;
    }
    
    // save the frame position for next call
    m_frame_counter = frame - to;
  }
  
                       
protected:
  
  double m_rate;
  int m_keys[128];
  unsigned char m_num_keys;
  uint32_t m_frame_counter;
  unsigned char m_next_key;
  unsigned char m_last_key;
  uint32_t m_midi_type;
  bool m_running;
  
};



static unsigned _ = 
  Arpeggiator::register_class("http://ll-plugins.nongnu.org/lv2/arpeggiator#0"); 
