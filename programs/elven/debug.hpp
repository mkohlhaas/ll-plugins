/****************************************************************************
    
    debug.hpp - Debugging utilities
    
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

#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <pthread.h>


/** This structs holds information about the current debug state, such
    as the message level and thread message prefixes. */
struct DebugInfo {
  
  /** Returns a reference to the message level. */
  static inline int& level() {
    static int dlevel = 0;
    return dlevel;
  }
  
  /** Returns a reference to the process-wide message prefix. */
  static inline std::string& prefix() {
    static std::string dprefix;
    return dprefix;
  }
  
  /** Returns a reference to the thread message prefix map. */
  static inline std::map<pthread_t, std::string>& thread_prefix() {
    static std::map<pthread_t, std::string> dthread_prefix;
    return dthread_prefix;
  }
  
};

  
#ifdef NDEBUG


#define DBG0(A) std::cerr<<A<<std::endl
#define DBG1(A) do { } while (false)
#define DBG2(A) do { } while (false)
#define DBG3(A) do { } while (false)
#define DBG4(A) do { } while (false)


#else


namespace {
  
  inline std::ostream& debug_print(const char* file, int line, int level) {
    if (level <= 0)
      std::cerr<<"\033[31;1m";
    else if (level == 1)
      std::cerr<<"\033[33;1m";
    else if (level == 2)
      std::cerr<<"\033[32;1m";
    else
      std::cerr<<"\033[1m";
    return std::cerr<<'['<<DebugInfo::prefix()
                    <<DebugInfo::thread_prefix()[pthread_self()]
                    <<std::setw(16)<<std::setfill(' ')
                    <<file<<':'<<std::setw(3)<<std::setfill('0')
                    <<line<<"] "<<"\033[0m";
  }

}

#define DBG0(A) do { if (DebugInfo::level() >= 0) debug_print(__FILE__, __LINE__, 0)<<A<<std::endl; } while (false)
#define DBG1(A) do { if (DebugInfo::level() >= 1) debug_print(__FILE__, __LINE__, 1)<<A<<std::endl; } while (false)
#define DBG2(A) do { if (DebugInfo::level() >= 2) debug_print(__FILE__, __LINE__, 2)<<A<<std::endl; } while (false)
#define DBG3(A) do { if (DebugInfo::level() >= 3) debug_print(__FILE__, __LINE__, 3)<<A<<std::endl; } while (false)
#define DBG4(A) do { if (DebugInfo::level() >= 4) debug_print(__FILE__, __LINE__, 4)<<A<<std::endl; } while (false)


#endif


#endif
