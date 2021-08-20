/****************************************************************************
    
    programmanager.hpp - Program manager class
    
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

#ifndef PROGRAMMANAGER_HPP
#define PROGRAMMANAGER_HPP

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "lv2-instrument.h"


/** Simple program manager for programs that only set port values. */
class ProgramManager {
public:
  
  /** Create a program manager that reads the programs from the given
      filename. */
  inline ProgramManager(const std::string& filename = "") {
    load_programs(filename);
  }
  
  
  /** Load programs from a file. */
  void load_programs(const std::string& filename) {
    std::ifstream ifs(filename.c_str());
    while (ifs.good()) {

      // read one line
      char line_c[1025];
      ifs.getline(line_c, 1024);
      std::string line(line_c);
      int i = line.find('\t');
      if (i == -1)
        break;
      
      // get the program number
      unsigned preset_number = atoi(line.substr(0, i).c_str());
      line = line.substr(i + 1);
      i = line.find('\t');
      if (i == -1)
        break;
      
      // get the program name
      std::string preset_name = line.substr(0, i);
      
      // get the port values
      std::istringstream iss(line.substr(i + 1));
      Program p;
      for (int k = 0; !iss.eof(); ++k) {
        double d;
        iss>>d;
        p.values[k] = d;
      }
      p.name = preset_name;
      p.number = preset_number;
      m_programs.push_back(p);
      
      std::cerr<<"Loaded program "<<p.number<<", \""<<p.name<<"\""<<std::endl;
    }
  }
  
  
  /** Apply a program to the given port buffer vector. */
  inline void apply_program(std::vector<void*>& ports, uint32_t program) {
    size_t index = find_program(program);
    if (index >= m_programs.size())
      return;
    std::map<uint32_t, float>::const_iterator iter;
    const std::map<uint32_t, float>& values = m_programs[index].values;
    for (iter = values.begin(); iter != values.end(); ++iter) {
      if (iter->first < ports.size())
        *static_cast<float*>(ports[iter->first]) = iter->second;
    }
  }
  
  
  /** Add a new program. */
  inline void add_program(uint32_t program, const std::string& name,
                          const std::map<uint32_t, float>& values) {
    size_t index = find_program(program);
    if (index >= m_programs.size()) {
      Program p;
      m_programs.push_back(p);
      index = m_programs.size() - 1;
      m_programs[index].number = program;
    }
    m_programs[index].name = name;
    m_programs[index].values = values;
  }
  
  
  /** Remove an existing program. */
  inline void remove_program(uint32_t program) {
    size_t index = find_program(program);
    if (index < m_programs.size())
      m_programs.erase(m_programs.begin() + index);
  }
  
  
  /** Return @c true if there is a program with the given index,
      and if @c desc is not 0, set its number and name to the programs
      number and name. */
  inline bool get_program_at_index(size_t index, LV2_ProgramDescriptor* desc) {
    if (index < m_programs.size()) {
      if (desc) {
        desc->number = m_programs[index].number;
        desc->name = m_programs[index].name.c_str();
      }
      return true;
    }
    return false;
  }
  
  
  /** Get the port values for a program. */
  const std::map<uint32_t, float>* get_values(uint32_t program) {
    size_t index = find_program(program);
    if (index < m_programs.size()) {
      return &m_programs[index].values;
    }
    return 0;
  }
  
  
protected:
  
  
  inline size_t find_program(uint32_t number) {
    if (m_programs.empty())
      return 0;
    size_t a = 0;
    size_t b = m_programs.size() - 1;
    while (b != a) {
      size_t c = (a + b) / 2;
      if (m_programs[c].number < number)
        a = c + 1;
      else if (m_programs[c].number > number)
        b = c - 1;
      else
        return c;
    } 
    if (m_programs[a].number == number)
      return a;
    return m_programs.size();
  }
  
  
  struct Program {
    std::string name;
    uint32_t number;
    std::map<uint32_t, float> values;
  };
  
  
  std::vector<Program> m_programs;
  
};


#endif
