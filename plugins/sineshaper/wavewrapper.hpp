/****************************************************************************
    
    wavewrapper.hpp - a sine waveshaper for use in DSSI plugins
    
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

/** Based on Steve Harris's Sinus Wavewrapper plugin */

#ifndef WAVEWRAPPER_HPP
#define WAVEWRAPPER_HPP

#include <cmath>


using namespace std;


class WaveWrapper {
public:
  
  inline float run(float input, float wrap, 
                   float shift);
  
protected:

};


float WaveWrapper::run(float input, float wrap, 
                       float shift) {
  float coef = wrap * M_PI;
  return sin(input * coef + shift);
}


#endif
