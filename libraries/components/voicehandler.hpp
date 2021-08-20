/****************************************************************************
    
    voicehandler.hpp - Voice manager for polyphonic MIDI synths
    
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

#ifndef VOICEHANDLER_HPP
#define VOICEHANDLER_HPP

#include <iostream>
#include <vector>

#include <lv2_event_helpers.h>


template <typename V> class VoiceHandler {
public:
  
  static const unsigned char NO_KEY = 255;
  
  struct VoiceInfo {
    VoiceInfo() : voice(0), key(NO_KEY), on(false) { }
    V* voice;
    unsigned char key;
    bool on;
  };
  
  inline VoiceHandler(unsigned voices, uint32_t rate);
  inline ~VoiceHandler();
  
  inline void set_voices(unsigned voices);
  
  inline void set_midi_port(LV2_Event_Buffer* port);
  
  inline void run(uint32_t frame);
  
  inline std::vector<VoiceInfo>& get_voices();
  
protected:
  
  LV2_Event_Buffer* m_port;
  uint32_t m_offset;
  std::vector<VoiceInfo> m_voices;
  uint32_t m_rate;
};


template <typename V> VoiceHandler<V>::VoiceHandler(unsigned voices,
                                                    uint32_t rate)
  : m_voices(voices),
    m_offset(0),
    m_rate(rate) {
  for (unsigned i = 0; i < m_voices.size(); ++i)
    m_voices[i].voice = new V(m_rate);
}


template <typename V> VoiceHandler<V>::~VoiceHandler() {
  for (unsigned i = 0; i < m_voices.size(); ++i)
    delete m_voices[i].voice;
}


template <typename V> void VoiceHandler<V>::set_voices(unsigned voices) {
  for (unsigned i = 0; i < m_voices.size(); ++i)
    delete m_voices[i].voice;
  m_voices.clear();
  m_voices.resize(voices);
  for (unsigned i = 0; i < m_voices.size(); ++i)
    m_voices[i].voice = new V(m_rate);
}


template <typename V> 
void VoiceHandler<V>::set_midi_port(LV2_Event_Buffer* port) {
  m_port = port;
  m_offset = 0;
}


template <typename V> void VoiceHandler<V>::run(uint32_t frame) {
  if (m_offset < m_port->size) {
    if (*(double*)(m_port->data + m_offset) <= frame) {
      m_offset += sizeof(double);
      size_t size = *(size_t*)(m_port->data + m_offset);
      m_offset += sizeof(size_t);
      unsigned char* data = m_port->data + m_offset;
      
      // all events we use have size 3, so ignore all that don't
      if (size == 3) {
        
        unsigned char status = data[0] & 0xF0;
        
        // note off
        if (status == 0x80) {
          std::cerr<<"NOTE OFF: "<<int(data[1])<<std::endl;
          for (unsigned i = 0; i < m_voices.size(); ++i) {
            if (m_voices[i].key == data[1]) {
              std::cerr<<"Turning voice "<<i<<" off"<<std::endl;
              m_voices[i].on = false;
              m_voices[i].key = NO_KEY;
              m_voices[i].voice->off(data[2]);
              break;
            }
          }
        }
        
        // note on
        if (status == 0x90) {
          unsigned i;
          std::cerr<<"NOTE ON: "<<int(data[1])<<" "<<int(data[2])<<std::endl;
          for (i = 0; i < m_voices.size(); ++i) {
            if (!m_voices[i].on) {
              std::cerr<<"Turning voice "<<i<<" on"<<std::endl;
              m_voices[i].on = true;
              m_voices[i].key = data[1];
              m_voices[i].voice->on(data[1], data[2]);
              break;
            }
            else {
              std::cerr<<"Voice "<<i<<" is busy playing key "<<m_voices[i].key<<std::endl;
            }
          }
          if (i == m_voices.size()) {
            std::cerr<<"Taking over voice 0"<<std::endl;
            m_voices[0].voice->off(64);
            m_voices[0].key = data[1];
            m_voices[0].voice->on(data[1], data[2]);
          }
        }
        
        // pitch bend
        if (status == 0xE0) {
          int value = ((data[2] << 7) | data[1]) - 0x2000;
          for (unsigned i = 0; i < m_voices.size(); ++i)
            m_voices[i].voice->bend(value);
        }
        
        // different controllers
        if (status == 0xB0) {
          
          // all notes off
          if (data[1] == 0x7B) {
            std::cerr<<"ALL NOTES OFF"<<std::endl;
            for (unsigned i = 0; i < m_voices.size(); ++i) {
              if (m_voices[i].on) {
                m_voices[i].on = false;
                m_voices[i].key = NO_KEY;
                m_voices[i].voice->off(64);
              }
            }
          }

          // all sound off
          if (data[1] == 0x78) {
            std::cerr<<"ALL SOUND OFF"<<std::endl;
            for (unsigned i = 0; i < m_voices.size(); ++i) {
              if (m_voices[i].on) {
                m_voices[i].on = false;
                m_voices[i].key = NO_KEY;
                m_voices[i].voice->fast_off();
              }
            }
          }
          
        }
        
      }
        
      m_offset += size;
    }
  }
}


template <typename V> 
std::vector<typename VoiceHandler<V>::VoiceInfo>& 
VoiceHandler<V>::get_voices() {
  return m_voices;
}


#endif
