/****************************************************************************
    
    monophonicmidinote.hpp - a MIDI event handler for use in DSSI plugins
    
    Copyright (C) 2005-2007 Lars Luthman <mail@larsluthman.net>
    
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef MONOPHONICMIDINOTE_HPP
#define MONOPHONICMIDINOTE_HPP

#include <iostream>

#include <dssi.h>

#include "frequencytable.hpp"


using namespace std;


class MonophonicMidiNote {
public:
  
  enum Gate {
    GateOn,
    GateOff,
    GateSame
  };
  
  inline MonophonicMidiNote();
  
  inline Gate run_event(const snd_seq_event_t& event, bool tie_notes);
  
  inline Gate get_gate() const;
  inline LADSPA_Data get_frequency() const;
  inline const LADSPA_Data& get_velocity() const;
  
private:
  
  Gate m_gate;
  LADSPA_Data m_pitch;
  LADSPA_Data m_velocity;
  LADSPA_Data m_pitchbend;
  FrequencyTable m_table;
  Slide m_freq_slide;
  Slide m_vel_slide;
  
  struct Key {
    static const unsigned char NoKey = 255;
    Key() : above(NoKey), below(NoKey), vel(0), held(false) { }
    unsigned char above;
    unsigned char below;
    LADSPA_Data vel;
    bool held;
  } m_keys[128];
  unsigned char m_active_key;

};


MonophonicMidiNote::MonophonicMidiNote() 
  : m_gate(GateSame),
    m_pitch(0.0f),
    m_velocity(0.5),
    m_pitchbend(1.0f) {
  
}


MonophonicMidiNote::Gate 
MonophonicMidiNote::run_event(const snd_seq_event_t& event, bool tie_notes,
			      unsigned int slide_time) {
  
  Gate g = GateSame;
  
  
  // note on
  if (event.type == SND_SEQ_EVENT_NOTEON) {
    
    // update variables
    const unsigned char& key = event.data.note.note;
    m_pitch = m_table[key];
    m_velocity = event.data.note.velocity / 128.0f;
    
    // if this key is already held, remove it from the key stack
    if (m_keys[key].held) {
      if (key == m_active_key)
	m_active_key = m_keys[key].below;
      if (m_keys[key].below != Key::NoKey)
	m_keys[m_keys[key].below].above = m_keys[key].above;
      if (m_keys[key].above != Key::NoKey)
	m_keys[m_keys[key].above].below = m_keys[key].below;
    }
    
    // if this is not a tied note, trigger the envelope and reset velocity
    // and frequency by setting their slide times to 0
    if (m_active_key == Key::NoKey || !tie_notes) {
      g = GateOn;
      vel_slide_time = 0;
      if (porta_on <= 0)
	freq_slide_time = 0;
    }
    
    // add this key at the top of the key stack
    m_keys[key].held = true;
    m_keys[key].vel = m_velocity;
    m_keys[key].below = m_active_key;
    m_keys[key].above = Key::NoKey;
    if (m_active_key != Key::NoKey)
      m_keys[m_active_key].above = key;
    
    m_active_key = key;
  }
  
  // note off
  else if (event.type == SND_SEQ_EVENT_NOTEOFF) {
    //m_note_is_on = false;
    
    unsigned char& key = event.data.note.note;
    
    // if we are not tieing notes, clear the note stack
    if (!tie_notes) {
      m_active_key = Key::NoKey;
      for (unsigned i = 0; i < 128; ++i) {
	m_keys[i].held = false;
	m_keys[i].above = Key::NoKey;
	m_keys[i].below = Key::NoKey;
      }
    }
    
    // if we are, just remove this note from the note stack
    else if (m_keys[key].held) {
      m_keys[key].held = false;
      if (key == m_active_key)
	m_active_key = m_keys[key].below;
      if (m_keys[key].below != Key::NoKey)
	m_keys[m_keys[key].below].above = m_keys[key].above;
      if (m_keys[key].above != Key::NoKey)
	m_keys[m_keys[key].above].below = m_keys[key].below;
    }
    
    // turn the envelope off if this was the last note, otherwise just
    // update the pitch and velocity
    if (m_active_key == Key::NoKey) 
      m_adsr.off(m_last_frame);
    else {
      m_velocity = m_keys[m_active_key].vel;
      m_pitch = m_table[m_active_key];
    }
  }
  
  // all notes off
  else if (event.type == SND_SEQ_EVENT_CONTROLLER &&
	   event.data.control.param == 123) {
    m_active_key = Key::NoKey;
    for (unsigned i = 0; i < 128; ++i) {
      m_keys[i].held = false;
      m_keys[i].above = Key::NoKey;
      m_keys[i].below = Key::NoKey;
    }
    m_adsr.off(m_last_frame);
  }
  
  // pitchbend
  else if (event.type == SND_SEQ_EVENT_PITCHBEND) {
    m_pitchbend = pow(twelfth_root_of_2, 
		      event.data.control.value / 4096.0f);
  }
  
  // all sound off
  else if (event.type == SND_SEQ_EVENT_CONTROLLER &&
	   event.data.control.param == 120) {
    m_active_key = Key::NoKey;
    for (unsigned i = 0; i < 128; ++i) {
      m_keys[i].held = false;
      m_keys[i].above = Key::NoKey;
      m_keys[i].below = Key::NoKey;
    }
    m_adsr.fast_off(m_last_frame);
  }
  
  return GateSame;
}


MonophonicMidiNote::Gate MonophonicMidiNote::get_gate() const {
  return m_gate;
}


LADSPA_Data MonophonicMidiNote::get_frequency() const {
  return m_pitch * m_pitchbend;
}


const LADSPA_Data& MonophonicMidiNote::get_velocity() const {
  return m_velocity;
}


#endif
