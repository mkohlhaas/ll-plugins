/****************************************************************************
    
    sineshaper.hpp - A LV2 synth plugin
    
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef SINESHAPER_HPP
#define SINESHAPER_HPP

#include "lv2plugin.hpp"
#include "sineoscillator.hpp"
#include "adsr.hpp"
#include "frequencytable.hpp"
#include "wavewrapper.hpp"
#include "slide.hpp"
#include "dcblocker.hpp"
#include "delay.hpp"


/** This is the class that contains all the code and data for the Sineshaper
    synth plugin. */
class SineShaper : public LV2::Plugin<SineShaper, 
				      LV2::URIMap<true>, LV2::EventRef<true> > {
public:
  
  SineShaper(double frame_rate);
  
  void run(uint32_t sample_count);
  
protected:
  
  void handle_midi(const uint8_t* event_data);

  void render_audio(uint32_t from, uint32_t to);
  
  SineOscillator m_vibrato_lfo;
  SineOscillator m_tremolo_lfo;
  SineOscillator m_shaper_lfo;
  SineOscillator m_osc;
  SineOscillator m_subosc;
  ADSR m_adsr;
  FrequencyTable m_table;
  WaveWrapper m_shaper;
  Slide m_freq_slide;
  Slide m_vel_slide;
  Delay m_delay;
  Slide m_osc_mix_slide;
  Slide m_trem_depth_slide;
  Slide m_shp_env_slide;
  Slide m_shp_total_slide;
  Slide m_shp_split_slide;
  Slide m_shp_shift_slide;
  Slide m_lfo_depth_slide;
  Slide m_att_slide;
  Slide m_dec_slide;
  Slide m_sus_slide;
  Slide m_rel_slide;
  Slide m_amp_env_slide;
  Slide m_drive_slide;
  Slide m_gain_slide;
  Slide m_delay_fb_slide;
  Slide m_delay_mix_slide;
  DCBlocker m_blocker;

  bool m_tie_overlapping;
  
  unsigned long m_frame_rate;
  unsigned long m_last_frame;
  
  float m_velocity;
  float m_pitch;
  
  struct Key {
    static const unsigned char NoKey = 255;
    Key() : above(NoKey), below(NoKey), vel(0), held(false) { }
    unsigned char above;
    unsigned char below;
    float vel;
    bool held;
  } m_keys[128];
  unsigned char m_active_key;
  
  float m_pitchbend;
  
  uint32_t m_midi_type;
  
};


#endif
