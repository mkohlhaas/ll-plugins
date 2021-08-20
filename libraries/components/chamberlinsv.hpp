/****************************************************************************
    
    chamberlinsv.hpp - a Chamberlin state variable filter
    
    Copyright (C) 2008 Lars Luthman <mail@larsluthman.net>
    
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

#ifndef CHAMBERLINSV_HPP
#define CHAMBERLINSV_HPP

#include <cmath>
#include <stdint.h>


class ChamberlinSV {
public:
  
  ChamberlinSV(double rate, float freq, float q)
    : m_rate(rate),
      m_f(2 * std::sin(M_PI * freq / m_rate)),
      m_q(q),
      m_l(0),
      m_h(0),
      m_b(0) {

  }
  
  void set_frequency(float freq) {
    m_f = 2 * std::sin(M_PI * freq / m_rate);
  }
  
  void set_q(float q) {
    m_q = q;
  }
  
  const ChamberlinSV& run(float input) {
    m_l += m_f * m_b;
    m_h = input - m_l - m_q * m_b;
    m_b += m_f * m_h;
    return *this;
  }
  
  float low() const {
    return m_l;
  }

  float high() const {
    return m_h;
  }

  float band() const {
    return m_b;
  }

  float notch() const {
    return m_l + m_h;
  }
  
protected:

  double m_rate;
  float m_f;
  float m_q;
  float m_l;
  float m_h;
  float m_b;

};

  
#endif
