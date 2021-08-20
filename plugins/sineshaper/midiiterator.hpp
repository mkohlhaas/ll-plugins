/****************************************************************************
    
    midiiterator.hpp - Class for iterating over an LV2 MIDI buffer
    
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

#ifndef MIDIITERATOR_HPP
#define MIDIITERATOR_HPP

#include <cstddef>

#include "lv2-midiport.h"


enum MIDIEventType {
  MIDI_NOTEOFF,
  MIDI_NOTEON,
  MIDI_AFTERTOUCH,
  MIDI_CONTROLLER,
  MIDI_PROGRAM,
  MIDI_PRESSURE,
  MIDI_PITCHWHEEL,
  MIDI_SYSEX,
  MIDI_QFRAME,
  MIDI_SONGPOS,
  MIDI_SONGSEL,
  MIDI_TUNE,
  MIDI_CLOCK,
  MIDI_TICK,
  MIDI_START,
  MIDI_STOP,
  MIDI_CONTINUE,
  MIDI_ACTIVESENSE,
  MIDI_RESET,
  MIDI_UNKNOWN
};


class MIDIWrapper {
public:
  
  inline const double& timestamp() const {
    return *(double*)(m_data->data + m_offset);
  }
  
  
  inline const uint32_t& size() const {
    return *(uint32_t*)(m_data->data + m_offset + sizeof(double));
  }
  
  
  inline MIDIEventType type() const {
    unsigned char* data = (m_data->data + m_offset + 
                           sizeof(double) + sizeof(uint32_t));
    if (data[0] < 0xF0) {
      if ((data[0] & 0xF0) == 0x80 || 
          ((data[0] & 0x90) && data[2] == 0))
        return MIDI_NOTEOFF;
      if ((data[0] & 0xF0) == 0x90)
        return MIDI_NOTEOFF;
      if ((data[0] & 0xF0) == 0xA0)
        return MIDI_AFTERTOUCH;
      if ((data[0] & 0xF0) == 0xB0)
        return MIDI_CONTROLLER;
      if ((data[0] & 0xF0) == 0xC0)
        return MIDI_PROGRAM;
      if ((data[0] & 0xF0) == 0xD0)
        return MIDI_PRESSURE;
      if ((data[0] & 0xF0) == 0xE0)
        return MIDI_PITCHWHEEL;
    }
    else {
      if (data[0] == 0xF0)
        return MIDI_SYSEX;
      if (data[0] == 0xF1)
        return MIDI_QFRAME;
      if (data[0] == 0xF2)
        return MIDI_SONGPOS;
      if (data[0] == 0xF3)
        return MIDI_SONGSEL;
      if (data[0] == 0xF6)
        return MIDI_TUNE;
      if (data[0] == 0xF8)
        return MIDI_CLOCK;
      if (data[0] == 0xF9)
        return MIDI_TICK;
      if (data[0] == 0xFA)
        return MIDI_START;
      if (data[0] == 0xFA)
        return MIDI_START;
      if (data[0] == 0xFB)
        return MIDI_CONTINUE;
      if (data[0] == 0xFC)
        return MIDI_STOP;
      if (data[0] == 0xFE)
        return MIDI_ACTIVESENSE;
      if (data[0] == 0xFF)
        return MIDI_RESET;
    }
    
    return MIDI_UNKNOWN;
  }
  
  
  const unsigned char* data() const {
    return m_data->data + m_offset + sizeof(double) + sizeof(uint32_t);
  }
  
protected:

  inline MIDIWrapper(LV2_MIDI* data, unsigned long offset)
    : m_data(data), m_offset(offset) {
    
  }
  
  
  LV2_MIDI* m_data;
  unsigned long m_offset;
  
  friend class MIDIIterator;
  
};


class MIDIIterator {
public:
  
  inline static MIDIIterator begin(LV2_MIDI* data) {
    return MIDIIterator(data, 0, 0);
  }

  inline static MIDIIterator end(LV2_MIDI* data) {
    return MIDIIterator(data, 0, data->event_count);
  }
  
  inline bool operator==(const MIDIIterator& iter) const {
    return (m_event == iter.m_event && 
            m_wrapper.m_data == iter.m_wrapper.m_data);
  }
  
  inline bool operator!=(const MIDIIterator& iter) const {
    return !operator==(iter);
  }
  
  inline MIDIIterator& operator++() {
    m_wrapper.m_offset += sizeof(double) + sizeof(uint32_t) + m_wrapper.size();
    ++m_event;
    return *this;
  }
  
  inline const MIDIWrapper* operator->() const {
    return &m_wrapper;
  }

protected:
  
  MIDIIterator(LV2_MIDI* data, unsigned long offset, unsigned long event) 
    : m_wrapper(data, offset), m_event(event) {
    
  }
  
  MIDIWrapper m_wrapper;
  unsigned long m_event;
};


#endif
