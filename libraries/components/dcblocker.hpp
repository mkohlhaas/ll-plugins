/****************************************************************************
    
    dcblocker.hpp - a simple DC blocker for use in DSSI plugins
    
    Copyright (C) 2005-2007 Lars Luthman <mail@larsluthman.net>
    
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

#ifndef DCBLOCKER_HPP
#define DCBLOCKER_HPP


class DCBlocker {
public:
  DCBlocker(uint32_t framerate) : m_x1(0.0), m_y1(0.0), m_R(0.995) {

  }
  
  inline float run(float input) {
    float result = ((1 + m_R) / 2) * (input - m_x1) + m_R * m_y1;
    m_x1 = input;
    m_y1 = result;
    return result;
  }
  
private:
  uint32_t m_framerate;
  float m_x1;
  float m_y1;
  float m_R;
};


#endif
