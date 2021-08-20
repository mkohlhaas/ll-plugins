/****************************************************************************
    
    monostep.hpp - a monophonic step sequencer
    
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

#ifndef MONOSTEP_HPP
#define MONOSTEP_HPP


class MonoStep {
public:
  MonoStep() : note(64), velocity(5), on(false), slide(false) { }
  unsigned char note;
  unsigned char velocity;
  bool on;
  bool slide;
};


#endif
