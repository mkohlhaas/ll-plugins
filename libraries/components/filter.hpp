/****************************************************************************
    
    filter.hpp - a simple filter
    
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

#ifndef FILTER_HPP
#define FILTER_HPP

#include <cmath>
#include <stdint.h>


namespace {


  class Filter {
  public:
    
    Filter(uint32_t rate);
    
    void set_cutoff(float cutoff);
    
    float run1(float input);

  protected:
    
    uint32_t m_rate;
    float m_p;
    float m_x;

  };
  
  
  Filter::Filter(uint32_t rate)
    : m_rate(rate),
      m_p(0),
      m_x(0) {

  }
  
  
  void Filter::set_cutoff(float cutoff) {
    m_p = std::pow(1 - 2 * cutoff / m_rate, 2);
  }
  
  float Filter::run1(float input) {
    return (m_x = (1 - m_p) * input + m_p * m_x);
  }


}


#endif
