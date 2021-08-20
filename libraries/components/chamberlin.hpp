/****************************************************************************
    
    chamberlin.hpp - a Chamberlin filter
    
    Copyright (C) 2007 Lars Luthman <mail@larsluthman.net>
    
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

#ifndef CHAMBERLIN_HPP
#define CHAMBERLIN_HPP

#include <cmath>
#include <stdint.h>


namespace {


  class Chamberlin {
  public:
    Chamberlin(uint32_t rate);
    void run(float* input, float* output, float cutoff, 
	     uint32_t nframes, float q,
	     float low_gain, float high_gain, float band_gain);
  protected:
    float m_invrate;
    float m_low;
    float m_band;
  };

  
  Chamberlin::Chamberlin(uint32_t rate)
    : m_invrate(1.0f / rate),
      m_low(0),
      m_band(0) {

  }


  void Chamberlin::run(float* input, float* output, uint32_t nframes,
		       float cutoff, float q,
		       float low_gain, float high_gain, float band_gain) {
    float f = 2 * std::sin(M_PI * cutoff * m_invrate);
    for (uint32_t i = 0; i < nframes; ++i) {
      m_low = m_low + f * m_band;
      float high = q * input[i] - m_low - q * m_band;
      m_band = f * high + m_band;
      output[i] = m_low * low_gain + m_high * high_gain + m_band + band_gain;
    }
  }


}


#endif
