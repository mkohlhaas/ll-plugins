/****************************************************************************
    
    slide.hpp - a slew limiter for use in DSSI plugins
    
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

#ifndef SLIDE_HPP
#define SLIDE_HPP


class Slide {
public:
  
  inline Slide(uint32_t frame_rate);
  
  inline float run(float input, uint32_t slide_time);
  
protected:
  
  uint32_t m_last_change;
  float m_from;
  float m_to;
  float m_last_output;
};


Slide::Slide(uint32_t) : m_from(0), m_to(0), m_last_output(0) {

}


float Slide::run(float input, uint32_t slide_time) {
  if (input != m_to) {
    m_from = m_last_output;
    m_to = input;
  }
  
  float result;
  
  if (slide_time == 0) 
    result = input;
  else {
    float increment = (m_to - m_from) / slide_time;
    bool rising = (m_to > m_from);
    result = m_last_output + increment;
    if ((result > m_to && rising) || (result < m_to && !rising))
      result = m_to;
  }
  
  m_last_output = result;
  return result;
}


#endif
