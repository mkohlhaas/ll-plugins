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

#ifndef ALLPASS_HPP
#define ALLPASS_HPP

#include <cstring>
#include <stdint.h>


namespace {


  class Allpass {
  public:
    
    Allpass(uint32_t rate, unsigned int delay);
    ~Allpass();
    
    void run(float* input, float* output, 
	     uint32_t nframes, unsigned int delay, float g);
    float run1(float input, unsigned int delay, float g);
    
  protected:
    
    float* m_data;
    unsigned int m_linesize;
    unsigned int m_counter;
    
  };
  
  
  Allpass::Allpass(uint32_t rate, unsigned int delay) 
    : m_data(new float[delay]),
      m_linesize(delay),
      m_counter(0) {
    
    std::memset(m_data, 0, m_linesize * sizeof(float));
    
  }
  
  
  Allpass::~Allpass() {
    delete [] m_data;
  }
  
  
  void Allpass::run(float* input, float* output, 
		    uint32_t nframes, unsigned delay, float g) {
    for (uint32_t i = 0; i < nframes; ++i, ++m_counter) {
      m_counter %= m_linesize;
      float o = m_data[m_counter] - g * input[i];
      m_data[(m_counter + delay) % m_linesize] = input[i] + g * o;
      output[i] = o;
    }
  }

  float Allpass::run1(float input, unsigned delay, float g) {
    float output = m_data[m_counter] - g * input;
    m_data[(m_counter + delay) % m_linesize] = input + g * output;
    ++m_counter;
    m_counter %= m_linesize;
    return output;
  }

  
}


#endif
