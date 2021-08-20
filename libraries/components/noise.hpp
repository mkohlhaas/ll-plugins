/****************************************************************************
    
    noise.hpp - a noise generator
    
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

#ifndef NOISE_HPP
#define NOISE_HPP


namespace {


  class Noise {
  public:
    Noise(unsigned int seed = 0);
    float run1();
  protected:
    unsigned int m_seed;
  };
  
  
  Noise::Noise(unsigned int seed) 
    : m_seed(seed) {

  }
  
  float Noise::run1() {
    m_seed = (m_seed * 196314165) + 907633515;
    return m_seed / float(INT_MAX) - 1;
  }


}

#endif
