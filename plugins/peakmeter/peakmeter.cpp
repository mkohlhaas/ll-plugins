/****************************************************************************
    
    peakmeter.cpp - simple audio meter plugin
    
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

#include <cmath>

#include "lv2plugin.hpp"
#include "peakmeter.peg"


/** This template class implements a plugin that takes an audio input signal
    and outputs a decaying peak value in a control port. It is a template
    so we don't have to write the same code twice for mono and stereo meters.
*/
template <unsigned C>
class PeakMeter : public LV2::Plugin< PeakMeter<C> > {
public:
  
  // we need import p() into our namespace since this is a template class
  using LV2::Plugin< PeakMeter<C> >::p;
  
  /** The constructor initialises the peak values to 0. */
  PeakMeter(double rate) : 
    LV2::Plugin< PeakMeter<C> >(2 * C),
    m_dy(1.0 / (1.0 * rate)),
    m_min(1.0 / 256),
    m_decay(exp(log(m_min) / (1 * rate))) {
    for (unsigned i = 0; i < C; ++i)
      m_values[i] = 0.0;
  }
  
  /** Read audio input, write control output. */
  void run(uint32_t nframes) {
    for (unsigned c = 0; c < C; ++c) {
      for (uint32_t i = 0; i < nframes; ++i) {
	float f = std::abs(p(2 * c)[i]);
	m_values[c] = f > m_values[c] ? f : m_values[c];
      }
      *p(2 * c + 1) = m_values[c] > m_min ? m_values[c] : 0;
      if (m_values[c] > m_min)
	m_values[c] *= pow(m_decay, nframes);
      else
	m_values[c] = 0.0;
    }
  }
  
protected:
  
  float m_values[C];
  float m_dy;
  float m_min;
  float m_decay;
  
};


// here we instantiate one mono plugin and one stereo plugin
static unsigned _1 = PeakMeter<1>::register_class(v_uri);
static unsigned _2 = PeakMeter<2>::register_class(v2_uri);
