/****************************************************************************
    
    envelope.hpp - An envelope generator
    
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

#ifndef ENVELOPE_HPP
#define ENVELOPE_HPP

#include <sstream>
#include <string>
#include <vector>

#include <pthread.h>
#include <stdint.h>


namespace {


class Envelope {
public:
  
  Envelope(uint32_t rate);
  ~Envelope();

  float run(float a, float d, float s, float r);
  
  void on();
  void off();
  void fast_off();
  
  bool set_string(const std::string& str);
  
protected:
  
  enum StageType {
    Constant,
    Attack,
    Decay,
    Release,
    NumTypes
  };
  
  enum CurveType {
    Linear,
    Exponential,
    Exponential2,
    NumCurves
  };
  
  struct Stage {
    Stage(float _start, float _p, float _ss, StageType _type, CurveType _curve)
      : start(_start), p(_p), ss(_ss), type(_type), curve(_curve) { }
    Stage() { }
    float start;
    float p;
    float ss;
    StageType type;
    CurveType curve;
  };
  
  std::vector<Stage> m_stages;
  
  float m_value;
  int m_stage;
  float m_phase;
  uint32_t m_rate;
  int m_loop_start;
  int m_loop_end;
  float m_stage_start;
  pthread_mutex_t m_mutex;
};


Envelope::Envelope(uint32_t rate) 
  : m_value(0),
    m_stage(-1),
    m_phase(0),
    m_rate(rate),
    m_loop_start(-1),
    m_loop_end(-1),
    m_stage_start(0) {

  m_stages.push_back(Stage(0, 1, 0, Attack, Linear));
  m_stages.push_back(Stage(1, 1, 0, Decay, Exponential));
  m_stages.push_back(Stage(0.5, 1, 1, Constant, Linear));
  m_stages.push_back(Stage(0.5, 1, 1, Release, Exponential));
  
  m_loop_start = 2;
  m_loop_end = 3;
  
  pthread_mutex_init(&m_mutex, 0);
}


Envelope::~Envelope() {
  pthread_mutex_destroy(&m_mutex);
}


float Envelope::run(float a, float d, float s, float r) {
  
  if (pthread_mutex_trylock(&m_mutex))
    return m_value;
  
  switch (m_stage) {

  case -2:
    m_value -= 1.0 / (0.003 * m_rate);
    if (m_value <= 0) {
      m_value = 0;
      m_stage = -1;
    }
    pthread_mutex_unlock(&m_mutex);
    return m_value;
    
  case -1:
    pthread_mutex_unlock(&m_mutex);
    return m_value;
    
  default: {
    float phi = 1.0 / (m_rate);
    if (m_stages[m_stage].type == Attack)
      phi /= a;
    else if (m_stages[m_stage].type == Decay)
      phi /= d;
    else if (m_stages[m_stage].type == Release)
      phi /= r;
    m_phase += phi;
    float a = m_stage_start;
    float b = m_stages[(m_stage + 1) % m_stages.size()].start;
    m_value = a + (b - a) * m_phase;
    if (m_phase >= 1) {
      ++m_stage;
      m_phase = 0;
      if (m_stage == m_loop_end)
        m_stage = m_loop_start;
      if (m_stage >= m_stages.size())
        m_stage = -1;
      m_value = b;
      m_stage_start = m_value;
    }
    pthread_mutex_unlock(&m_mutex);
    return m_value;
  }
    
  }
  
}
 
 
void Envelope::on() {
  m_stage = 0;
  m_phase = 0;
  m_stage_start = m_value;
}


void Envelope::off() {
  if (m_loop_end != -1) {
    m_stage = m_loop_end;
    m_stage_start = m_value;
    if (m_loop_end == m_stages.size())
      m_stage = -2;
  }
}


void Envelope::fast_off() {
  m_stage = -2;
  m_phase = 0;
}

  
bool Envelope::set_string(const std::string& str) {
  std::istringstream iss(str);
  int loop_start, loop_end;
  iss>>loop_start>>loop_end;
  std::vector<Stage> new_segments;
  while (iss.good()) {
    Stage s;
    iss>>s.start>>s.p>>s.ss;
    std::string tmp;
    iss>>tmp;
    if (tmp == "c")
      s.type = Constant;
    else if (tmp == "a")
      s.type = Attack;
    else if (tmp == "d")
      s.type = Decay;
    else if (tmp == "r")
      s.type = Release;
    iss>>tmp;
    if (tmp == "l")
      s.curve = Linear;
    else if (tmp == "e")
      s.curve = Exponential;
    else if (tmp == "e2")
      s.curve = Exponential2;
    new_segments.push_back(s);
  }
  
  if (new_segments.size() == 0 || new_segments[0].start != 0 || 
      (loop_start != -1 && loop_start >= new_segments.size()) ||
      (loop_end != -1 && loop_end > new_segments.size()))
    return false;
  
  m_stages = new_segments;
  m_loop_start = loop_start;
  m_loop_end = loop_end;
  
  return true;
}


}


#endif
