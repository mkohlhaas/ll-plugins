/****************************************************************************
    
    allpass.hpp - a simple allpass filter
    
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

#ifndef HOUSEHOLDERFDN_HPP
#define HOUSEHOLDERFDN_HPP

#include <stdint.h>

#include "delayline.hpp"
#include "filter.hpp"


namespace {
  

  class HouseholderFDN {
  public:
    
    HouseholderFDN(uint32_t rate);
    
    void run(float* left, float* right, uint32_t nframes, 
	     float time, float mix, float damp);
    
  protected:
    
    uint32_t m_rate;
    DelayLine m_d1;
    DelayLine m_d2;
    DelayLine m_d3;
    DelayLine m_d4;
    DelayLine m_d5;
    DelayLine m_d6;
    DelayLine m_d7;
    DelayLine m_d8;
    Filter m_f1;
    Filter m_f2;
    Filter m_f3;
    Filter m_f4;
    Filter m_f5;
    Filter m_f6;
    Filter m_f7;
    Filter m_f8;
        
  };
  
  
  HouseholderFDN::HouseholderFDN(uint32_t rate)
    : m_rate(rate),
      m_d1(2473),
      m_d2(2767),
      m_d3(3217),
      m_d4(3557),
      m_d5(3907),
      m_d6(4127),
      m_d7(2143),
      m_d8(1933),
      m_f1(rate),
      m_f2(rate),
      m_f3(rate),
      m_f4(rate),
      m_f5(rate),
      m_f6(rate),
      m_f7(rate),
      m_f8(rate) {
    
  }
  
  
  void HouseholderFDN::run(float* left, float* right, uint32_t nframes, 
			   float time, float mix, float damp) {
    
    damp = damp < 0 ? 0 : damp;
    damp = damp > m_rate / 2.0 ? m_rate / 2.0 : damp;
    float f = time;
    f = f < 0 ? 0 : f;
    f = f > 1 ? 1 : f;
    
    for (uint32_t i = 0; i < nframes; ++i) {
      
      // read from delay lines and compute output
      float u1 = m_d1.read();
      float u2 = m_d2.read();
      float u3 = m_d3.read();
      float u4 = m_d4.read();
      float u5 = m_d5.read();
      float u6 = m_d6.read();
      float u7 = m_d7.read();
      float u8 = m_d8.read();
      float output_l = u1 + u3 + u5 + u7;
      float output_r = u2 + u4 + u6 + u8;
      
      // set filter coefficients
      m_f1.set_cutoff(damp);
      m_f2.set_cutoff(damp);
      m_f3.set_cutoff(damp);
      m_f4.set_cutoff(damp);
      m_f5.set_cutoff(damp);
      m_f6.set_cutoff(damp);
      m_f7.set_cutoff(damp);
      m_f8.set_cutoff(damp);
      
      // write to delay lines
      float ul = left[i] - output_l * 2.0 / 4;
      float ur = right[i] - output_r * 2.0 / 4;
      m_d1.write(f * m_f1.run1(ul + u1));
      m_d2.write(f * m_f2.run1(ur + u2));
      m_d3.write(f * m_f3.run1(ul + u3));
      m_d4.write(f * m_f4.run1(ur + u4));
      m_d5.write(f * m_f5.run1(ul + u5));
      m_d6.write(f * m_f6.run1(ur + u6));
      m_d7.write(f * m_f7.run1(ul + u7));
      m_d8.write(f * m_f8.run1(ur + u8));
    
      left[i] = (1 - mix) * left[i] + mix * output_l;
      right[i] = (1 - mix) * right[i] + mix * output_r;
      
    }
    
  }
  

}


#endif
