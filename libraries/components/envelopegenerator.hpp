/****************************************************************************
    
    envelope.hpp - an envelope generator for use in DSSI plugins
    
    Copyright (C) 2005-2007  Lars Luthman <mail@larsluthman.net>
    
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

#ifndef ENVELOPEGENERATOR_HPP
#define ENVELOPEGENERATOR_HPP

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <pthread.h>
#include <ladspa.h>


using namespace std;


struct EnvelopeSegment {
  double l, y;
  int type;
  double vel_sens, mod_sens;
  int vel_mod, mod_mod;
};
  

struct EnvelopeData {
  
  inline EnvelopeData() : loop_start(-1), loop_end(-1) {
    EnvelopeSegment seg1 = { 0.1, 0.0, 2, 0.0, 0.0, 0, 0 };
    EnvelopeSegment seg2 = { 0.3, 1.0, 1, 0.0, 0.0, 0, 0 };
    EnvelopeSegment seg3 = { 0.4, 0.6, 0, 0.0, 0.0, 0, 0 };
    EnvelopeSegment seg4 = { 0.2, 0.6, 3, 0.0, 0.0, 0, 0 };
    segments.push_back(seg1);
    segments.push_back(seg2);
    segments.push_back(seg3);
    segments.push_back(seg4);
    loop_start = 2;
    loop_end = 2;
  }

  inline string get_string() {
    ostringstream oss;
    oss<<loop_start<<" "<<loop_end<<" "<<segments.size();
    for (unsigned i = 0; i < segments.size(); ++i) {
      oss<<" "<<segments[i].y<<" "<<segments[i].l<<" "<<int(segments[i].type)
       <<" "<<segments[i].vel_sens<<" "<<segments[i].mod_sens<<" e";
    }
    return oss.str();
  }
  
  inline void set_string(const string& str) {
    istringstream iss(str);
    iss>>loop_start>>loop_end;
    int n;
    iss>>n;
    segments.clear();
    for (int i = 0; i < n; ++i) {
      EnvelopeSegment seg;
      iss>>seg.y>>seg.l>>seg.type>>seg.vel_sens>>seg.mod_sens;
      char e;
      iss>>e;
      segments.push_back(seg);
      if (seg.l <= 0.0001)
	seg.l = 0.0001;
    }
  }
  
  int loop_start;
  int loop_end;
  vector<EnvelopeSegment> segments;
};


static inline double env_linear(double phase) {
  return phase;
}


static inline double env_early(double phase) {
  return (1 - pow(1 - phase, 3.0));
}


static inline double env_late(double phase) {
  return pow(phase, 3);
}


static inline double env_sigmoid(double phase) {
  return (1 - cos(3.14159 * phase)) / 2;
}


class EnvelopeGenerator {
public:

  inline EnvelopeGenerator(uint32_t frame_rate);
  
  inline void on(uint32_t now);
  inline void off(uint32_t now);
  inline void set_string(const string& str);
  
  inline LADSPA_Data run(uint32_t now, 
			 LADSPA_Data speed, LADSPA_Data modulation);

private:

  LADSPA_Data m_inv_rate;
  int m_stage;
  LADSPA_Data m_phase;
  uint32_t m_last_t;
  LADSPA_Data m_start_v;
  uint32_t m_start_t;
  LADSPA_Data m_last_value;
  
  EnvelopeData d;
  pthread_mutex_t m_mutex;
};


EnvelopeGenerator::EnvelopeGenerator(uint32_t frame_rate) {
  m_inv_rate = 1.0f / frame_rate;
  m_last_value = 0.0f;
  m_stage = -1;
  pthread_mutex_init(&m_mutex, 0);
}


void EnvelopeGenerator::on(uint32_t now) {
  m_stage = 0;
  m_phase = 0;
  m_start_t = now;
  m_start_v = m_last_value;
}


void EnvelopeGenerator::off(uint32_t now) {
  if (d.loop_end != -1 && m_stage <= d.loop_end) {
    m_start_t = now;
    m_start_v = m_last_value;
    m_stage = d.loop_end + 1;
    m_phase = 0;
    if (m_stage >= (int)d.segments.size())
      m_stage = -1;
  }
}


void EnvelopeGenerator::set_string(const string& str) {
  // XXX DANGEROUS! Should make this threadsafe
  //m_stage = -1;
  //m_last_value = d.segments[0].y;
  //d.set_string(str);
  pthread_mutex_lock(&m_mutex);
  d.set_string(str);
  if (m_stage >= int(d.segments.size())) {
    m_stage = -1;
    m_last_value = d.segments[0].y;
  }
  pthread_mutex_unlock(&m_mutex);
  
}


LADSPA_Data EnvelopeGenerator::run(uint32_t now, LADSPA_Data speed,
                                   LADSPA_Data modulation) {
  // try to get the lock before we touch the data
  if (pthread_mutex_trylock(&m_mutex))
    return m_last_value;
  
  // if we're not in an envelope, just return the start value
  LADSPA_Data result = d.segments[0].y;

  // m_stage >= 0 means that we're in the envelope
  if (m_stage >= 0) {
    m_phase += (now - m_last_t) * m_inv_rate / 
      (d.segments[m_stage].l * (d.segments[m_stage].vel_sens * speed + 1));
    int next = (m_stage + 1) % d.segments.size();
    LADSPA_Data scale = d.segments[next].y * 
      (d.segments[next].mod_sens * modulation + 1) - m_start_v;
    
    // linear
    if (d.segments[m_stage].type == 0) {
      result = m_start_v + scale * m_phase;
    }
    
    // early monomial
    else if (d.segments[m_stage].type == 1) {
      result = m_start_v + scale * (1 - pow(1 - m_phase, 3.0f));
    }
    
    // late monomial
    else if (d.segments[m_stage].type == 2) {
      result = m_start_v + scale * pow(m_phase, 3);
    }
    
    // sigmoid
    else {
      result = m_start_v + scale * (1 - cos(3.14159 * m_phase)) / 2;
    }
    
    // should we move to next stage?
    if (m_phase >= 1.0f) {
      if (m_stage == d.loop_end)
	m_stage = d.loop_start;
      else
	++m_stage;
      if (m_stage >= (int)d.segments.size())
	m_stage = -1;
      m_start_t = now;
      m_start_v = result;
      m_phase = 0;
    }
  }
  
  pthread_mutex_unlock(&m_mutex);
  
  m_last_t = now;
  m_last_value = result;
  return result;
}


#endif
