/****************************************************************************
    
    randomsineoscillator.hpp - a bandlimited oscillator for use in DSSI plugins
    
    Copyright (C) 2005-2007  Lars Luthman <mail@larsluthman.net>
    
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

/** Based on the Sine plugin in the CMT package by Richard Furse */

#ifndef RANDOMSINEOSCILLATOR_HPP
#define RANDOMSINEOSCILLATOR_HPP

#include <cmath>
#include <cstdlib>

#include <ladspa.h>


using namespace std;


class RandomSineOscillator {
public:
  
  inline RandomSineOscillator(uint32_t frame_rate);
  
  inline void reset();
  inline LADSPA_Data run(LADSPA_Data frequency);
  
protected:
  
  inline void set_frequency(LADSPA_Data frequency);
  
  double m_phase;
  double m_phase_step;
  LADSPA_Data m_cached_frequency;
  const LADSPA_Data m_limit_frequency;
  LADSPA_Data m_inv_rate;
  unsigned char m_harmonic;
  
};


RandomSineOscillator::RandomSineOscillator(uint32_t frame_rate)
  : m_phase(0), m_phase_step(0), m_cached_frequency(0), 
    m_limit_frequency(LADSPA_Data(frame_rate * 0.5)),
    m_inv_rate(1.0 / frame_rate), m_harmonic(1) {

}


void RandomSineOscillator::reset() {
  m_phase = 0;
}


LADSPA_Data RandomSineOscillator::run(LADSPA_Data frequency) {
  LADSPA_Data result = sin(m_harmonic * m_phase);
  set_frequency(frequency);
  m_phase += m_phase_step;
  if (m_phase >= 2 * M_PI) {
    m_harmonic = (rand() % 2) + 1;
    m_phase -= 2 * M_PI;
  }
  return result;
}


void RandomSineOscillator::set_frequency(LADSPA_Data frequency) {
  if (frequency != m_cached_frequency) {
    if (frequency >= 0 && frequency < m_limit_frequency) 
      m_phase_step = frequency * 2 * M_PI * m_inv_rate;
    else 
      m_phase_step = 0;
    m_cached_frequency = frequency;
  }
}


#endif
