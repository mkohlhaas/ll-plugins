/****************************************************************************
    
    onepole.hpp - a one pole filter
    
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

#ifndef ONEPOLE_HPP
#define ONEPOLE_HPP

#include <stdint.h>


namespace {


  class OnePole {
  public:
    
    OnePole(uint32_t rate);
    
    void set_b(float b);
    
    float run1(float input);

  protected:
    
    float m_b;
    float m_s;

  };

  
  
  OnePole::OnePole(uint32_t rate) 
    : m_b(0),
      m_s(0) {

  }

  
  void OnePole::set_b(float b) {
    m_b = b;
  }
  
  
  float run1(float input) {
    return (m_s = (1 - b) * input + b * m_s);
  }
  
  
}


#endif
