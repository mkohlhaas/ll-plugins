/****************************************************************************
    
    ringbuffer.hpp - a ringbuffer designed to be used in shared memory
    
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

#ifndef RINGBUFFER_HPP
#define RINGBUFFER_HPP

#include <cstring>


template <class T, unsigned S> class Ringbuffer {
public:

  Ringbuffer();

  inline int read(T* dest, unsigned size = 1);
  inline int write(T* src, unsigned size = 1);
  inline int write_zeros(unsigned size);
  inline int available() const;
  inline int available_contiguous() const;
  inline int get_read_pos() const;
  inline int get_write_pos() const;
  inline const T* get_read_ptr() const;
  inline T* get_write_ptr();
  /** Not threadsafe! */
  inline void clear();
  
private:
  
  volatile int m_read_pos;
  volatile int m_write_pos;
  
  char volatile m_data[S * sizeof(T)];

};


template <class T, unsigned S> 
Ringbuffer<T, S>::Ringbuffer()
  : m_read_pos(0),
    m_write_pos(0) {
  
}


template <class T, unsigned S> 
int Ringbuffer<T, S>::read(T* dest, unsigned size) {
  
  unsigned n = 0;
  T* data = (T*)m_data;
  
  if (size == 0)
    return 0;
  
  if (m_read_pos > m_write_pos) {
    n = S - m_read_pos;
    n = (n > size ? size : n);
    if (dest)
      std::memcpy(dest, data + m_read_pos, n * sizeof(T));
    m_read_pos = (m_read_pos + n) % S;
  }
  
  if (m_read_pos < m_write_pos && n < size) {
    unsigned m = m_write_pos - m_read_pos;
    m = (m > size - n ? size - n : m);
    if (dest)
      std::memcpy(dest + n, data + m_read_pos, m * sizeof(T));
    m_read_pos = (m_read_pos + m) % S;
    n += m;
  }
  
  /*  fprintf(stderr, "RB %d atoms read from the ringbuffer\n"
      "\tread_pos = %d write_pos = %d max_pos = %d\n",
      n, rb->read_pos, rb->write_pos, rb->max_pos); */

  /*if (n < size)
    fprintf(stderr, "RB: read collision %d\n", col++);*/

  return n;

}


template <class T, unsigned S>
int Ringbuffer<T, S>::write(T* src, unsigned size) {
  unsigned n = 0;
  T* data = (T*)(m_data);

  if (size == 0)
    return 0;

  if (m_write_pos >= m_read_pos) {
    n = S - m_write_pos;
    if (m_read_pos == 0)
      --n;
    n = (n > size ? size : n);
    std::memcpy(data + m_write_pos, src, n * sizeof(T));
    m_write_pos = (m_write_pos + n) % S;
  }
  
  if (m_write_pos + 1 < m_read_pos && n < size) {
    unsigned m = m_read_pos - m_write_pos - 1;
    m = (m > size - n ? size - n : m);
    std::memcpy(data + m_write_pos, src + n, m * sizeof(T));
    m_write_pos = (m_write_pos + m) % S;
    n += m;
  }

  /*fprintf(stderr, "RB %d atoms written to the ringbuffer\n"
    "\tread_pos = %d write_pos = %d max_pos = %d\n",
    n, rb->read_pos, rb->write_pos, rb->max_pos);*/
  
  /* if (n < size)
     fprintf(stderr, "RB: write collision %d\n", col++); */
  
  return n;

}


template <class T, unsigned S>
int Ringbuffer<T, S>::write_zeros(unsigned size) {
  unsigned n = 0;
  T* data = (T*)(m_data);

  if (size == 0)
    return 0;

  if (m_write_pos >= m_read_pos) {
    n = S - m_write_pos;
    if (m_read_pos == 0)
      --n;
    n = (n > size ? size : n);
    std::memset(data + m_write_pos, 0, n * sizeof(T));
    m_write_pos = (m_write_pos + n) % S;
  }
  
  if (m_write_pos + 1 < m_read_pos && n < size) {
    unsigned m = m_read_pos - m_write_pos - 1;
    m = (m > size - n ? size - n : m);
    std::memset(data + m_write_pos, 0, m * sizeof(T));
    m_write_pos = (m_write_pos + m) % S;
    n += m;
  }

  /*fprintf(stderr, "RB %d atoms written to the ringbuffer\n"
    "\tread_pos = %d write_pos = %d max_pos = %d\n",
    n, rb->read_pos, rb->write_pos, rb->max_pos);*/
  
  /* if (n < size)
     fprintf(stderr, "RB: write collision %d\n", col++); */
  
  return n;

}


template <class T, unsigned S>
int Ringbuffer<T, S>::available() const {
  if (m_read_pos <= m_write_pos)
    return m_write_pos - m_read_pos;
  else
    return S - m_read_pos + m_write_pos;
}


template <class T, unsigned S>
int Ringbuffer<T, S>::available_contiguous() const {
  if (m_read_pos <= m_write_pos)
    return m_write_pos - m_read_pos;
  else
    return S - m_read_pos;
}


template <class T, unsigned S>
int Ringbuffer<T, S>::get_read_pos() const {
  return m_read_pos;
}
  

template <class T, unsigned S>
int Ringbuffer<T, S>::get_write_pos() const {
  return m_write_pos;
}


template <class T, unsigned S>
const T* Ringbuffer<T, S>::get_read_ptr() const {
  T* data = (T*)m_data;
  return data + m_read_pos;
}


template <class T, unsigned S>
T* Ringbuffer<T, S>::get_write_ptr() {
  T* data = (T*)m_data;
  return data + m_write_pos;
}


template <class T, unsigned S>
void Ringbuffer<T, S>::clear() {
  m_read_pos = m_write_pos = 0;
}


#endif


