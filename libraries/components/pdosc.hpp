/****************************************************************************
    
    pdosc.hpp - Simple phase distortion oscillator
    
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

#ifndef PDOSC_HPP
#define PDOSC_HPP

#include <cmath>


class PDOsc {
public:
  
  inline PDOsc(uint32_t rate);
  inline void reset();
  inline float run(float freq, int shape, float amount);

protected:
  
  float m_phase;
  float m_inv_rate;
  
};


PDOsc::PDOsc(uint32_t rate)
  : m_phase(0.25),
    m_inv_rate(1.0 / rate) {
  
}
  
  
void PDOsc::reset() {
  m_phase = 0.25;
}

  
float PDOsc::run(float freq, int shape, float amount) {
  
  m_phase = fmodf(m_phase + freq * m_inv_rate, 1.0);
  float dphase;
  
  switch (shape) {
  case 0:
    if (m_phase < 0.5 * (1 - amount))
      dphase = m_phase / (1 - amount);
    else
      dphase = 0.5 + (m_phase - 0.5 * (1 - amount)) / (1 + amount);
    break;
    
  case 1:
    if (m_phase < 0.5 * (1 - amount))
      dphase = m_phase / (1 - amount);
    else if (m_phase < 0.5)
      dphase = 0.5;
    else if (m_phase < 0.5 + 0.5 * (1 - amount))
      dphase = 0.5 + (m_phase - 0.5) / (1 - amount);
    else
      dphase = 1.0;
    break;
    
  case 2:
    if (m_phase < amount)
      dphase = 0.25;
    else
      dphase = 0.25 + (m_phase - amount) / (1 - amount);
    break;
    
  case 3:
    if (m_phase < amount)
      dphase = 0.25;
    else
      dphase = 0.25 + 0.5 * (m_phase - amount) / (1 - amount);
    break;

  case 4:
    if (m_phase < amount)
      dphase = 0.25;
    else
      dphase = 0.25 + 2 * (m_phase - amount) / (1 - amount);
    break;

  case 5:
    if (m_phase < amount)
      dphase = 0.25;
    else
      dphase = 0.25 + 3 * (m_phase - amount) / (1 - amount);
    break;

  }
  
  return cos(dphase * 2 * M_PI);
}


#endif
