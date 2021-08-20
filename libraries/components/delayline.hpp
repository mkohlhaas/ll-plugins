/****************************************************************************
    
    delayline.hpp - a simple delay line
    
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

#ifndef DELAYLINE_HPP
#define DELAYLINE_HPP

#include <cstring>


namespace {


  class DelayLine {
  public:
    
    DelayLine(unsigned int delay);
    ~DelayLine();
    
    void write(float input);
    float read();

  protected:
    
    unsigned int m_size;
    float* m_line;
    unsigned int m_pos;

  };
  
  
  DelayLine::DelayLine(unsigned int delay) 
    : m_size(delay),
      m_line(new float[m_size]),
      m_pos(0) {
    std::memset(m_line, 0, sizeof(float) * m_size);
  }
  
  
  DelayLine::~DelayLine() {
    delete [] m_line;
  }
  
  
  void DelayLine::write(float input) {
    m_line[m_pos] = input;
    ++m_pos;
    m_pos %= m_size;
  }
  
  
  float DelayLine::read() {
    return m_line[m_pos];
  }
  
}


#endif
