/****************************************************************************
    
    rudolf556.cpp - Simple drum machine plugin
    
    Copyright (C) 2008 Lars Luthman <mail@larsluthman.net>
    
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

#include <cstring>
#include <iostream>

#include <samplerate.h>
#include <sndfile.h>

#include "lv2synth.hpp"
#include "rudolf556.peg"


using namespace std;


/** The Voice class for the LV2::Synth template. There is one single 
    voice class that plays different drum sounds depending on the constructor
    parameters. It could be done with inheritance and virtual functions as 
    well, but that would add vtable lookups which prevent function call 
    inlining. */
class Voice : public LV2::Voice {
public:
  
  /** The different drum types. */
  enum VoiceType {
    Bass = 0,
    Snare = 1,
    Hihat = 2
  };
  
  /** The constructor initialises the voice to play a certain drum sound. 
      @param type          The type of drum sound, different types use slightly 
                           different algorithms
      @param number        The voice number, so the voice knows which ports to 
                           get its control parameters from
      @param quickfade_max The length, in samples, of the quickfade - the 
                           fadeout time of a voice when it is told to start 
                           playing a new note
      @param buf1          The first sample buffer (there can be three of them,
                           but all drum types don't use all three)
      @param len1          The length, in samples, of @c buf1.
  */
  Voice(VoiceType type, int number, unsigned quickfade_max,
	float* buf1, unsigned len1, float* buf2, unsigned len2, 
	float* buf3 = 0, unsigned len3 = 0)
    : m_key(LV2::INVALID_KEY),
      m_frame(0),
      m_last_sample(0.0),
      m_quickfade(0),
      m_quickfade_max(quickfade_max),
      m_type(type),
      m_number(number - 1),
      m_buf1(buf1),
      m_buf2(buf2),
      m_buf3(buf3),
      m_len1(len1),
      m_len2(len2),
      m_len3(len3) {
    
  }
  
  /** LV2::Synth calls this when it receives a Note On event for a key
      mapped to this voice. */
  void on(unsigned char key, unsigned char velocity) {
    
    m_key = key;

    // if we are already playing a note we need to fade it out really quick
    // before starting this one to avoid clicks
    // don't do it for hihats, they are noisy enough to cover the clicks
    if (m_type != Hihat && m_key != LV2::INVALID_KEY) {
      m_quickfade = m_quickfade_max;
    }
    
    // reset the frame position
    m_frame = 0;
    
    // copy the current parameters (we keep them constant for the duration 
    // of this drum hit)
    m_l = *p(1 + 6 * m_type + 3 * m_number + 0);
    m_l = m_l < 0 ? 0 : m_l;
    m_l = m_l > 1 ? 1 : m_l;
    m_h = *p(1 + 6 * m_type + 3 * m_number + 1);
    m_h = m_h < 0 ? 0 : m_h;
    m_h = m_h > 1 ? 1 : m_h;
    m_v = *p(1 + 6 * m_type + 3 * m_number + 2);
    m_v = m_v < 0 ? 0 : m_v;
    m_v = m_v > 1 ? 1 : m_v;
    
    // set the decay onset and total length depending on the length parameter
    if (m_type == Bass) {
      float e = (pow(100, 0.8 * m_l) - 1) / 99;
      m_decay_start = (0.02 + 0.98 * e) * m_len1;
      m_end = m_decay_start + (0.02 + 0.98 * e) * m_len1;
      m_end = m_end < m_len1 ? m_end : m_len1;
    }
    else if (m_type == Snare) {
      float e = (pow(50, m_l) - 1) / 49;
      m_decay_start = (0.02 + 0.98 * e) * m_len1;
      m_end = m_decay_start + (0.02 + 0.98 * e) * m_len1;
      m_end = m_end < m_len1 ? m_end : m_len1;
    }
    else {
      float e = (pow(100, 0.8 * m_l) - 1) / 99;
      m_decay_start = (0.02 + 0.98 * e) * m_len1;
      m_end = m_decay_start + (0.02 + 0.98 * e) * m_len1;
      m_end = m_end < m_len1 ? m_end : m_len1;
    }
  }
  
  /** The LV2::Synth template requires this, but it isn't really needed
      since we write our own Rudolf556::find_free_voice() that doesn't care
      if a voice is already playing or not. */
  unsigned char get_key() const {
    return m_key;
  }
  
  /** The main audio generating function. */
  void render(uint32_t from, uint32_t to) {
    
    // if we're not playing a note, just return
    if (m_key == LV2::INVALID_KEY)
      return;
    
    float*& output = p(r_output_mix);
    
    // if we are doing a quickfade from last note, finish that first
    // it might be nicer to do a BLEP here, but a linear ramp seems to work OK
    if (m_quickfade) {
      uint32_t n = from + m_quickfade;
      n = n > to ? to : n;
      for ( ; from < n; ++from, --m_quickfade)
	output[from] += (m_last_sample * m_quickfade) / m_quickfade_max;
    }
    if (from >= to)
      return;
    
    // different drum types use slightly different algorithms
    switch (m_type) {
      
    case Bass: {
      
      /* The bass drum uses three samples of different "hardness" and 
	 mixes two of them depending on whether the hardness parameter is 
	 smaller or larger than 0.5. It also uses a fadeout with onset and 
	 length depending on the current length parameter.
      */
      
      // choose the right pair of sample buffers to mix
      float h;
      float* buf1;
      float* buf2;
      if (m_h < 0.5) {
	h = 2 * m_h;
	buf1 = m_buf1;
	buf2 = m_buf2;
      }
      else {
	h = 2 * m_h - 1;
	buf1 = m_buf2;
	buf2 = m_buf3;
      }
      
      // write the audio to the output port
      if (m_frame < m_decay_start) {
	uint32_t n = from + m_decay_start - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  m_last_sample = 
	    0.6 * m_v * ((1-h) * buf1[m_frame] + h * buf2[m_frame]);
	  output[from] += m_last_sample;
	}
      }
      if (m_frame < m_end) {
	uint32_t n = from + m_end - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  float s = 
	    1 - (m_frame - m_decay_start) / float(m_end - m_decay_start);
	  m_last_sample = 
	    s * 0.6 * m_v * ((1-h) * buf1[m_frame] + h * buf2[m_frame]);
	  output[from] += m_last_sample;
	}
      }
      
      break;
    }
      
    case Snare: {
      
      /* The snare drum uses two samples, one for the "drum" part (pretty
	 much a sped up bass drum) and one for the "snare" part (a noise burst).
	 The volume of the snare depends on the hardness parameter. It has
	 a fadeout similar to the bass drum.
      */

      if (m_frame < m_decay_start) {
	uint32_t n = from + m_decay_start - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  m_last_sample = 
	    0.6 * m_v * (m_buf1[m_frame] + m_h * 0.2 * m_buf2[m_frame]);
	  output[from] += m_last_sample;
	}
      }
      if (m_frame < m_end) {
	uint32_t n = from + m_end - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  float s = 
	    1 - (m_frame - m_decay_start) / float(m_end - m_decay_start);
	  m_last_sample = 
	    s * 0.6 * m_v * (m_buf1[m_frame] + m_h * 0.2 * m_buf2[m_frame]);
	  output[from] += m_last_sample;
	}
      }

      break;
    }
      
    case Hihat: {
      
      /* The hihat uses two samples, one "base" which is a long hihat hit and
	 one "hit" which is a much shorter version of the same sound. The
	 volume of the hit depends on the hardness parameter. It has a fadeout
	 similar to the bass drum.
      */

      if (m_frame < m_decay_start) {
	uint32_t n = from + m_decay_start - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  float v = m_buf1[m_frame] + m_h * m_buf2[m_frame];
	  v = (fabs(v + 1) - fabs(v - 1)) * 0.5;
	  m_last_sample = 0.3 * m_v * v;
	  output[from] += m_last_sample;
	}
      }
      if (m_frame < m_end) {
	uint32_t n = from + m_end - m_frame;
	n = n > to ? to : n;
	for ( ; from < n; ++from, ++m_frame) {
	  float v = m_buf1[m_frame] + m_h * m_buf2[m_frame];
	  v = (fabs(v + 1) - fabs(v - 1)) * 0.5;
	  float s = 
	    1 - (m_frame - m_decay_start) / float(m_end - m_decay_start);
	  m_last_sample = s * 0.3 * m_v * v;
	  output[from] += m_last_sample;
	}
      }

      break;
    }
      
    }

    // if we're done with this note, reset
    if (m_frame >= m_end) {
      m_frame = 0;
      m_key = LV2::INVALID_KEY;
    }
    
  }
  

protected:
  
  unsigned char m_key;
  unsigned m_frame;
  unsigned m_decay_start;
  unsigned m_end;
  float m_last_sample;
  unsigned m_quickfade;
  unsigned m_quickfade_max;
  
  VoiceType m_type;
  int m_number;
  float* m_buf1;
  float* m_buf2;
  float* m_buf3;
  unsigned m_len1;
  unsigned m_len2;
  unsigned m_len3;
  float m_l;
  float m_h;
  float m_v;
  
};


/** The main plugin class. This handles sample loading and voice stealing. */
class Rudolf556 : public LV2::Synth<Voice, Rudolf556> {
public:

  /** This constructor loads all sample files. If it can't get them all
      the plugin will not be instantiated. */
  Rudolf556(double rate) 
    : LV2::Synth<Voice, Rudolf556>(r_n_ports, r_midi_input),
      m_rate(rate) {
    
    // load all samples
    long m_bass_h00_frames, m_bass_h05_frames, m_bass_h10_frames,
      m_snare_bonk_frames, m_snare_noise_frames, 
      m_hihat_base_frames, m_hihat_hit_frames;
    m_bass_h00 = get_sample_data("bass_h00.flac", m_bass_h00_frames);
    m_bass_h05 = get_sample_data("bass_h05.flac", m_bass_h05_frames);
    m_bass_h10 = get_sample_data("bass_h10.flac", m_bass_h10_frames);
    m_snare_bonk = get_sample_data("snare_bonk.flac", m_snare_bonk_frames);
    m_snare_noise = get_sample_data("snare_noise.flac", m_snare_noise_frames);
    m_hihat_base = get_sample_data("hihat_base.flac", m_hihat_base_frames);
    m_hihat_hit = get_sample_data("hihat_hit.flac", m_hihat_hit_frames);
    if (!(m_bass_h00 && m_bass_h05 && m_bass_h10 && 
	  m_snare_bonk && m_snare_noise && m_hihat_base && m_hihat_hit)) {
      set_ok(false);
      return;
    }
    
    // add two voices of each type
    add_voices(new Voice(Voice::Bass, 1, 64,
			 m_bass_h00, m_bass_h00_frames,
			 m_bass_h05, m_bass_h05_frames,
			 m_bass_h10, m_bass_h10_frames),
	       new Voice(Voice::Bass, 2, 64,
			 m_bass_h00, m_bass_h00_frames,
			 m_bass_h05, m_bass_h05_frames,
			 m_bass_h10, m_bass_h10_frames),
	       new Voice(Voice::Snare, 1, 64,
			 m_snare_bonk, m_snare_bonk_frames,
			 m_snare_noise, m_snare_noise_frames),
	       new Voice(Voice::Snare, 2, 64,
			 m_snare_bonk, m_snare_bonk_frames,
			 m_snare_noise, m_snare_noise_frames),
	       new Voice(Voice::Hihat, 1, 64,
			 m_hihat_base, m_hihat_base_frames,
			 m_hihat_hit, m_hihat_hit_frames),
	       new Voice(Voice::Hihat, 2, 64,
			 m_hihat_base, m_hihat_base_frames,
			 m_hihat_hit, m_hihat_hit_frames));
    
    add_audio_outputs(r_output_mix);
  }
  
  
  /** Delete all the samples here so we don't leak memory. */
  ~Rudolf556() {
    delete [] m_bass_h00;
    delete [] m_bass_h05;
    delete [] m_bass_h10;
    delete [] m_snare_bonk;
    delete [] m_snare_noise;
    delete [] m_hihat_base;
    delete [] m_hihat_hit;
  }
  
  
  /** We override find_free_voice() so we can map keys directly to voices -
      we don't care if they are free or not. */
  unsigned find_free_voice(unsigned char key, unsigned char velocity) {
    static unsigned NO = 1024;
    static unsigned voice_map[] = { 0, NO, 1, NO, 2, 3, NO, 4, NO, 5, NO, NO };
    return voice_map[key % 12];
  }
  
protected:
  
  /** Load a sound from a file and resample it to the current rate. */
  float* get_sample_data(const std::string& filename, long& frames) {
    
    frames = 0;
    
    // open file
    SF_INFO info;
    info.format = 0;
    string path = (string(bundle_path()) + filename);
    SNDFILE* snd = sf_open(path.c_str(), SFM_READ, &info);
    if (!snd) {
      cerr << "sf_open(\"" << path << "\") failed." << endl;
      cerr << "Maybe libsndfile is built without FLAC support." << endl;
      return 0;
    }
    
    // read data
    float* data = new float[info.frames];
    sf_read_float(snd, data, info.frames);
    sf_close(snd);
    
    // if no resampling is needed, return the data
    if (abs(m_rate - info.samplerate) / m_rate < 0.0001) {
      frames = info.frames;
      return data;
    }
    
    // resample to plugin rate
    SRC_DATA src;
    src.src_ratio = m_rate / info.samplerate;
    src.data_in = data;
    src.output_frames = info.frames * src.src_ratio + 1;
    src.data_out = new float[src.output_frames];
    src.data_out[src.output_frames - 1] = 0;
    src.input_frames = info.frames;
    if (src_simple(&src, SRC_SINC_BEST_QUALITY, 1)) {
      delete [] data;
      delete [] src.data_out;
      return 0;
    }
    delete [] data;
    frames = src.output_frames;
    return src.data_out;
  }
  
  // the sample data
  float* m_bass_h00;
  float* m_bass_h05;
  float* m_bass_h10;
  float* m_snare_bonk;
  float* m_snare_noise;
  float* m_hihat_base;
  float* m_hihat_hit;
  
  double m_rate;
  
};


static unsigned _ = Rudolf556::register_class(r_uri);
