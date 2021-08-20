/****************************************************************************
    
    sineoscillator.hpp - a bandlimited sine oscillator for use in DSSI plugins
    
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

/** Based on the Sine plugin in the CMT package by Richard Furse */

#ifndef SINEOSCILLATOR_HPP
#define SINEOSCILLATOR_HPP

#include <cmath>

#define SINE_TABLE_BITS 14
#define SINE_TABLE_SHIFT (8 * sizeof(uint32_t) - SINE_TABLE_BITS)


using namespace std;


/** A simple bandlimited sine oscillator. */
class SineOscillator {
public:
  
  inline SineOscillator(uint32_t frame_rate);
  
  /** Reset the phase to 0. */
  inline void reset();
  
  /** Compute one sample. */
  inline float run(float frequency);
  
protected:
  
  /** Change the oscillator frequency. */
  inline void set_frequency(float frequency);
  
  /** Fill the sine table (only done once, the table is a static member). */
  static inline void initialise_table();
  
  uint32_t m_phase;
  uint32_t m_phase_step;
  float m_cached_frequency;
  const float m_limit_frequency;
  float m_phase_step_scalar;

  static float m_sine[1 << SINE_TABLE_BITS];
  static float m_phase_step_base;
  static bool m_initialised;
};


float SineOscillator::m_sine[1 << SINE_TABLE_BITS];
float SineOscillator::m_phase_step_base;
bool SineOscillator::m_initialised(false);


SineOscillator::SineOscillator(uint32_t frame_rate)
  : m_phase(0), m_phase_step(0), m_cached_frequency(0), 
    m_limit_frequency(float(frame_rate * 0.5)) {

  if (!m_initialised)
    initialise_table();
  
  m_phase_step_scalar = float(m_phase_step_base / frame_rate);
}


void SineOscillator::reset() {
  m_phase = 0;
}


float SineOscillator::run(float frequency) {
  float result = m_sine[m_phase >> SINE_TABLE_SHIFT];
  set_frequency(frequency);
  m_phase += m_phase_step;
  return result;
}


void SineOscillator::set_frequency(float frequency) {
  if (frequency != m_cached_frequency) {
    if (frequency >= 0 && frequency < m_limit_frequency) 
      m_phase_step = (uint32_t)(m_phase_step_scalar * frequency);
    else 
      m_phase_step = 0;
    m_cached_frequency = frequency;
  }
}


void SineOscillator::initialise_table() {
  uint32_t lTableSize = 1 << SINE_TABLE_BITS;
  double dShift = (double(M_PI) * 2) / lTableSize;
  for (uint32_t lIndex = 0; lIndex < lTableSize; lIndex++)
    m_sine[lIndex] = float(sin(dShift * lIndex));
  m_phase_step_base = (float)pow(2.0f, int(sizeof(uint32_t) * 8));
}


#endif
