/****************************************************************************
    
    markov.hpp - a Markov process simulator
    
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

#ifndef MARKOV_HPP
#define MARKOV_HPP

#include <cstdlib>


/** A class that simulates a discrete-time Markov process with a finite 
    state space. You can set all the transition probabilities. */
class Markov {
public:
  
  /** Create a new Markov process with @c nstates states. */
  inline Markov(unsigned int nstates);
  
  /** Destroy the Markov process. */
  inline ~Markov();
  
  /** Set the current state of the process. */
  inline void set_state(unsigned int state);
  
  /** Get the current state. */
  inline unsigned int get_state() const;
  
  /** Go to next (nondeterministic) state. */
  inline unsigned int transition();
  
  /** Set the probability of transition from one state to another. */
  inline void set_probability(unsigned int from, unsigned int to, float prob);
  
protected:
  
  float* m_prob;
  unsigned int m_nstates;
  unsigned int m_state;
  
};


Markov::Markov(unsigned int nstates)
  : m_prob(new float[nstates * nstates]),
    m_nstates(nstates),
    m_state(0) {

  for (unsigned int i = 0; i < m_nstates; ++i)
    for (unsigned int j = 0; j < m_nstates; ++j)
      m_prob[i * m_nstates + j] = 1;
}


Markov::~Markov() {
  delete [] m_prob;
}


void Markov::set_state(unsigned int state) {
  m_state = state;
}


unsigned int Markov::get_state() const {
  return m_state;
}


unsigned int Markov::transition() {
  float total = 0;
  for (unsigned int i = 0; i < m_nstates; ++i)
    total += m_prob[m_state * m_nstates + i];
  float r = total * (std::rand() / float(RAND_MAX));
  float acc = 0;
  for (unsigned int i = 0; i < m_nstates; ++i) {
    acc += m_prob[m_state * m_nstates + i];
    if (r <= acc) {
      m_state = i;
      return i;
    }
  }
  return m_nstates - 1;
}


void Markov::set_probability(unsigned int from, unsigned int to, float prob) {
  m_prob[from * m_nstates + to] = prob;
}



#endif
