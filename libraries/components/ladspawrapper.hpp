/****************************************************************************
    
    ladspawrapper.hpp - A class that loads a LADSPA plugin
    
    Copyright (C) 2006-2007  Lars Luthman <mail@larsluthman.net>
    
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

#ifndef LADSPAWRAPPER_HPP
#define LADSPAWRAPPER_HPP

#include <iostream>

using namespace std;

#include <string>
#include <vector>
#include <dlfcn.h>

#include <ladspa.h>


class LADSPAWrapper {
public:
  
  LADSPAWrapper(const std::string& filename, 
                unsigned long uid, unsigned long rate);
  ~LADSPAWrapper();
  
  void connect_port(unsigned long port, LADSPA_Data* data_location);
  void activate();
  void run(unsigned long sample_count);
  void run_adding(unsigned long sample_count);
  void set_run_adding_gain(LADSPA_Data gain);
  void deactivate();
  
  void set_control(unsigned long port, LADSPA_Data value);
  
protected:
  
  const LADSPA_Descriptor* m_desc;
  void* m_handle;
  LADSPA_Handle m_instance;
  std::vector<LADSPA_Data> m_controls;
};


LADSPAWrapper::LADSPAWrapper(const std::string& filename,
			     unsigned long uid, unsigned long rate)
  : m_desc(0),
    m_handle(0),
    m_instance(0) {
  
  m_handle = dlopen(filename.c_str(), RTLD_NOW);
  assert(m_handle);
  typedef const LADSPA_Descriptor* (*ld_fun)(unsigned long);
  ld_fun get_desc = (ld_fun)dlsym(m_handle, "ladspa_descriptor");
  for (unsigned i = 0; true; ++i) {
    const LADSPA_Descriptor* desc = get_desc(i);
    if (!desc)
      break;
    if (desc->UniqueID == uid) {
      m_desc = desc;
      break;
    }
  }
  assert(m_desc);
  
  cerr<<"Loaded "<<filename<<":"<<m_desc->Label<<endl
      <<'\''<<m_desc->Name<<"' by "<<m_desc->Maker<<endl;
  
  m_instance = m_desc->instantiate(m_desc, rate);
  assert(m_instance);
  cerr<<"Instantiated one plugin"<<endl;
  
  m_controls.resize(m_desc->PortCount);
  for (unsigned i = 0; i < m_desc->PortCount; ++i)
    connect_port(i, &m_controls[i]);
}


LADSPAWrapper::~LADSPAWrapper() {
  if (m_desc->cleanup)
    m_desc->cleanup(m_instance);
  dlclose(m_handle);
}


void LADSPAWrapper::connect_port(unsigned long port, 
				 LADSPA_Data* data_location) {
  if (m_desc->connect_port)
    m_desc->connect_port(m_instance, port, data_location);
}


void LADSPAWrapper::activate() {
  if (m_desc->activate)
    m_desc->activate(m_instance);
}


void LADSPAWrapper::run(unsigned long sample_count) {
  if (m_desc->run)
    m_desc->run(m_instance, sample_count);
}


void LADSPAWrapper::run_adding(unsigned long sample_count) {
  if (m_desc->run_adding)
    m_desc->run_adding(m_instance, sample_count);
}


void LADSPAWrapper::set_run_adding_gain(LADSPA_Data gain) {
  if (m_desc->set_run_adding_gain)
    m_desc->set_run_adding_gain(m_instance, gain);
}


void LADSPAWrapper::deactivate() {
  if (m_desc->deactivate)
    m_desc->deactivate(m_instance);
}


void LADSPAWrapper::set_control(unsigned long port, LADSPA_Data value) {
  m_controls[port] = value;
}


#endif
