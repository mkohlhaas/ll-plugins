/****************************************************************************
    
    mooglpf.hpp - A filter that emulates a Moog lowpass filter
    
    Based on (well, actually blatantly stolen from)  Fons Adriaensen's
    LADSPA plugin Mvclpf-4 in the mcp-plugins package. Thank you, Fons!

    Copyright (C) 2003 Fons Adriaensen
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

#ifndef MOOGLPF_HPP
#define MOOGLPF_HPP

#include <cmath>


class MoogLPF {
public:
  
  
  MoogLPF::MoogLPF(uint32_t rate) 
    : m_rate(rate) {
    
  }
  

  void MoogLPF::reset() {
    _c1 = _c2 = _c3 = _c4 = _c5 = _w = _r = 0;
  }

  
  void MoogLPF::run(uint32_t samples, float* input, float* output,
                    float* frequency, float* exp_fm, float* resonance,
                    float input_gain, float freq_ctrl, float exp_fm_gain,
                    float res_ctrl, float res_gain, float poles, 
                    float output_gain) {
    int   k, op;
    float *p2, *p3, *p4;
    float c1, c2, c3, c4, c5;
    float g0, g1, r, dr, w, dw, x, t, d, y;

    p2 = frequency - 1;
    p3 = exp_fm - 1;
    p4 = resonance - 1;
    g0 = exp2ap(0.1661 * input_gain) / 2;
    g1 = exp2ap(0.1661 * output_gain) * 2;
    op = (int)(std::floor(poles + 0.5));

    c1 = _c1 + 1e-6;
    c2 = _c2;
    c3 = _c3;
    c4 = _c4;
    c5 = _c5;
    w = _w; 
    r = _r;

    do {
      k = (samples > 24) ? 16 : samples;
      p2 += k;
      p3 += k;
      p4 += k;
      samples -= k;
      
      t = exp2ap(exp_fm_gain * *p3 + freq_ctrl + *p2 + 9.70) / m_rate;
      if (t < 0.75)
        t *= 1.005 - t * (0.624 - t * (0.65 - t * 0.54));
      else {
        t *= 0.6748;
        if (t > 0.82) t = 0.82;
      }
      dw = (t - w) / k;
      
      t = res_gain * *p4 + res_ctrl;
      if (t > 1) t = 1;
      if (t < 0) t = 0;
      dr = (t - r) / k;  
      
      while (k--) {
        w += dw;                        
        r += dr;
  
        x = *input * g0 - (4.3 - 0.2 * w) * r * c5 + 1e-10; 
        //            x = tanh (x);
        x /= sqrt (1 + x * x);
        d = w * (x  - c1) / (1 + c1 * c1);           
        x = c1 + 0.77 * d;
        c1 = x + 0.23 * d;        
        d = w * (x  - c2) / (1 + c2 * c2);            
        x = c2 + 0.77 * d;
        c2 = x + 0.23 * d;        
        d = w * (x  - c3) / (1 + c3 * c3);            
        x = c3 + 0.77 * d;
        c3 = x + 0.23 * d;        
        d = w * (x  - c4);
        x = c4 + 0.77 * d;
        c4 = x + 0.23 * d;        
        c5 += 0.85 * (c4 - c5);
  
        x = y = *input++ * g0 -(4.3 - 0.2 * w) * r * c5;
        //            x = tanh (x);
        x /= sqrt (1 + x * x);
        d = w * (x  - c1) / (1 + c1 * c1);            
        x = c1 + 0.77 * d;
        c1 = x + 0.23 * d;        
        d = w * (x  - c2) / (1 + c2 * c2);            
        x = c2 + 0.77 * d;
        c2 = x + 0.23 * d;        
        d = w * (x  - c3) / (1 + c3 * c3);            
        x = c3 + 0.77 * d;
        c3 = x + 0.23 * d;        
        d = w * (x  - c4);
        x = c4 + 0.77 * d;
        c4 = x + 0.23 * d;        
        c5 += 0.85 * (c4 - c5);
  
        switch (op) {
        case 1: y = c1; break;
        case 2: y = c2; break;
        case 3: y = c3; break;
        case 4: y = c4; break;
        }
  
        *output++  = g1 * y;
      }
    } while (samples);

    _c1 = c1;
    _c2 = c2;
    _c3 = c3;
    _c4 = c4;
    _c5 = c5;
    _w = w;
    _r = r;
  }

  
protected:

  static float exp2ap(float x) {
    int i;
    i = (int)(floor (x));
    x -= i;
    return ldexp(1 + x * (0.6930 + x * (0.2416 + x * (0.0517 + x * 0.0137))),i);
  }

  
  float _c1, _c2, _c3, _c4, _c5, _w, _r;
  uint32_t m_rate;
  
};


#endif
