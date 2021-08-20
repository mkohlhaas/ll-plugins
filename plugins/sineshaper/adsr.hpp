/****************************************************************************
    
    adsr.hpp - a simple ADSR envelope generator for use in DSSI plugins
    
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

#ifndef ADSR_HPP
#define ADSR_HPP

#include <stdint.h>


/** A simple ADSR envelope generator. */
class ADSR {
public:
  
  inline ADSR(uint32_t frame_rate);
  
  /** Turn the envelope on (when a key is pressed). */
  inline void on(uint32_t now);
  
  /** Turn the envelope off (when a key is released). */
  inline void off(uint32_t now);
  
  /** Turn the envelope off without a long release time. Useful if you want
      to stop all sound, but still want a short release to avoid clicks. */
  inline void fast_off(uint32_t now);
  
  /** Compute one sample. */
  inline float run(uint32_t now, float level, float att,
                   float dec, float sus, float rel);

protected:

  enum State {
    Off,
    Attack,
    Decay,
    Sustain,
    Release,
    FastRelease
  } m_state;
  uint32_t m_segment_start_time;
  float m_segment_start_value;
  float m_inv_rate;
  float m_last_value;
  
};


ADSR::ADSR(uint32_t frame_rate) 
  : m_state(Off), m_inv_rate(1.0f / frame_rate) {
  
}


void ADSR::on(uint32_t now) {
  m_state = Attack;
  m_segment_start_time = now;
  m_segment_start_value = m_last_value;
}


void ADSR::off(uint32_t now) {
  if (m_state != Off && m_state != Release) {
    m_state = Release;
    m_segment_start_time = now;
    m_segment_start_value = m_last_value;
  }
}
 

void ADSR::fast_off(uint32_t now) {
  if (m_state != Off && m_state != FastRelease) {
    m_state = FastRelease;
    m_segment_start_time = now;
    m_segment_start_value = m_last_value;
  }
}
 

float ADSR::run(uint32_t now, float level, float att, 
                float dec, float sus, float rel) {
  float tmp = 0;

  switch (m_state) {
    
  case Off:
    m_last_value = 0.0f;
    break;
    
  case Attack:
    if (att > 0)
      tmp = (now - m_segment_start_time) * m_inv_rate / att;
    if (att <= 0 || tmp > 1) {
      m_state = Decay;
      m_segment_start_time = now;
    }
    m_last_value = m_segment_start_value + (level- m_segment_start_value) * tmp;
    if (att > 0)
      break;
    
  case Decay:
    if (dec > 0)
      tmp = (now - m_segment_start_time) * m_inv_rate / dec;
    if (dec <= 0 || tmp > 1) {
      m_state = Sustain;
    }
    m_last_value = level * (1 - (1 - sus) * tmp);
    if (dec > 0)
      break;
    
  case Sustain:
    m_last_value = sus * level;
    break;
    
  case Release:
    if (rel > 0)
      tmp = (now - m_segment_start_time) * m_inv_rate / rel;
    if (rel <= 0 || tmp > 1) {
      m_state = Off;
    }
    m_last_value = m_segment_start_value * (1 - tmp);
    break;
    
  case FastRelease:
    tmp = (now - m_segment_start_time) * m_inv_rate / 0.001;
    if (tmp > 1) {
      m_state = Off;
    }
    m_last_value = m_segment_start_value * (1 - tmp);
    break;

  } 
  
  return m_last_value;
}


#endif
