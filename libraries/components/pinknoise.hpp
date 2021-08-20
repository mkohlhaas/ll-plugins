/****************************************************************************
    
    pinknoise.hpp - A pink noise generator
    
    Uses an algorithm designed by Larry Trammell, see
    http://home.earthlink.net/~ltrammell/tech/newpink.htm

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA

****************************************************************************/

#ifndef PINKNOISE_HPP
#define PINKNOISE_HPP

#include "noise.hpp"


namespace {


  class PinkNoise {
  public:
    PinkNoise(unsigned int seed = 0);
    float run1();
  protected:
    float m_pA[5];
    float m_pSUM[5];
    float m_contrib[5];
    Noise m_noise;
  };

  
  PinkNoise::PinkNoise(unsigned int seed = 0) 
    : m_noise(seed) {
    m_pA[0] = 3.8024;
    m_pA[1] = 2.9694;
    m_pA[2] = 2.5970;
    m_pA[3] = 3.0870;
    m_pA[4] = 3.4006;
    m_pSUM[0] = 0.00198;
    m_pSUM[1] = 0.01478;
    m_pSUM[2] = 0.06378;
    m_pSUM[3] = 0.23378;
    m_pSUM[4] = 0.91578;
    for (int i = 0; i < 5; ++i)
      m_contrib[i] = m_noise.run1() * m_pA[i];
  }
  
  
  float PinkNoise::run1() {
    float ur1, ur2;
    ur1 = (m_noise.run1() + 1) / 2;
    for (int i = 0; i < 5; ++i) {
      if (ur1 <= pSUM(i)) {
	ur2 = m_noise.run1();
	m_contrib[stage] = ur2 * m_pA[istage];
	break;
      }
    }
    return m_contrib[0] + m_contrib[1] + m_contrib[2] + 
      m_contrib[3] + m_contrib[4];
  }
  
  
}


#endif
