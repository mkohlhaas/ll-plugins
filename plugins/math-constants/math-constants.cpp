/****************************************************************************
    
    math-constants.cpp - LV2 plugins producing constant output for some common
                         constants from the C math library
    
    Copyright (C) 2006-2007 Lars Luthman <mail@larsluthman.net>
    
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

#include <cmath>
#include <iostream>

#include "lv2plugin.hpp"


namespace {
  
  // This is needed because floats can't be used as template parameters,
  // so we have to use references
  float e = M_E;
  float log2e = M_LOG2E;
  float log10e = M_LOG10E;
  float ln2 = M_LN2;
  float ln10 = M_LN10;
  float pi = M_PI;
  float pi_2 = M_PI_2;
  float pi_4 = M_PI_4;
  float _1_pi = M_1_PI;
  float _2_pi = M_2_PI;
  float _2_sqrtpi = M_2_SQRTPI;
  float sqrt2 = M_SQRT2;
  float sqrt1_2 = M_SQRT1_2;
}

/** This is a template plugin class with a single output port that returns 
    a constant value. */
template <float& Output>
class Constant : public LV2::Plugin< Constant<Output> > {
public:
  Constant(double) : LV2::Plugin< Constant<Output> >(1) {
    
  }
  void run(uint32_t sample_count) {
    *static_cast<float*>(LV2::Plugin< Constant<Output> >::m_ports[0]) = Output;
  }
};


// register plugins for all the different output values
#define URIPFX "http://ll-plugins.nongnu.org/lv2/math-constant-"
static unsigned _ = (Constant<e>::register_class(URIPFX "e#0"),
		     Constant<log2e>::register_class(URIPFX "log2e#0"),
		     Constant<log10e>::register_class(URIPFX "log10e#0"),
		     Constant<ln2>::register_class(URIPFX "ln2#0"),
		     Constant<ln10>::register_class(URIPFX "ln10#0"),
		     Constant<pi>::register_class(URIPFX "pi#0"),
		     Constant<pi_2>::register_class(URIPFX "pi_2#0"),
		     Constant<pi_4>::register_class(URIPFX "pi_4#0"),
		     Constant<_1_pi>::register_class(URIPFX "1_pi#0"),
		     Constant<_2_pi>::register_class(URIPFX "2_pi#0"),
		     Constant<_2_sqrtpi>::register_class(URIPFX "2_sqrtpi#0"),
		     Constant<sqrt2>::register_class(URIPFX "sqrt2#0"),
		     Constant<sqrt1_2>::register_class(URIPFX "sqrt1_2#0"));
