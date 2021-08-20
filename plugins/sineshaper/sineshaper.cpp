/****************************************************************************
    
    sineshaper.cpp - A DSSI synth plugin
    
    Copyright (C) 2006-2008 Lars Luthman <mail@larsluthman.net>
    
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

#include <cmath>
#include <iostream>

#include <lv2_event_helpers.h>

#include "sineshaper.hpp"
#include "sineshaperports.hpp"


using namespace std;
using namespace LV2;


SineShaper::SineShaper(double frame_rate) 
  : Plugin<SineShaper, URIMap<true>, EventRef<true> >(SINESHAPER_PORT_COUNT),
    m_vibrato_lfo(frame_rate),
    m_tremolo_lfo(frame_rate),
    m_shaper_lfo(frame_rate),
    m_osc(frame_rate),
    m_subosc(frame_rate),
    m_adsr(frame_rate),
    m_freq_slide(frame_rate),
    m_vel_slide(frame_rate),
    m_delay(frame_rate, 3),
    m_osc_mix_slide(frame_rate),
    m_trem_depth_slide(frame_rate),
    m_shp_env_slide(frame_rate),
    m_shp_total_slide(frame_rate),
    m_shp_split_slide(frame_rate),
    m_shp_shift_slide(frame_rate),
    m_lfo_depth_slide(frame_rate),
    m_att_slide(frame_rate),
    m_dec_slide(frame_rate),
    m_sus_slide(frame_rate),
    m_rel_slide(frame_rate),
    m_amp_env_slide(frame_rate),
    m_drive_slide(frame_rate),
    m_gain_slide(frame_rate),
    m_delay_fb_slide(frame_rate),
    m_delay_mix_slide(frame_rate),
    m_blocker(frame_rate),
    m_frame_rate(frame_rate),
    m_last_frame(0),
    m_velocity(0.5f),
    m_pitch(440.0f),
    m_active_key(Key::NoKey),
    m_pitchbend(1.0f),
    m_midi_type(uri_to_id(LV2_EVENT_URI, 
			  "http://lv2plug.in/ns/ext/midi#MidiEvent")) {
  
}


void SineShaper::handle_midi(const uint8_t* event_data) {

  static const float twelfth_root_of_2 = pow(2, 1/12.0);

  // note on
  if ((event_data[0] & 0xF0) == 0x90) {
    
    // update variables
    const unsigned char& key = event_data[1];
    m_pitch = m_table[key];
    m_velocity = event_data[2] / 128.0f;
    
    // if this key is already held, remove it from the key stack
    if (m_keys[key].held) {
      if (key == m_active_key)
	m_active_key = m_keys[key].below;
      if (m_keys[key].below != Key::NoKey)
	m_keys[m_keys[key].below].above = m_keys[key].above;
      if (m_keys[key].above != Key::NoKey)
	m_keys[m_keys[key].above].below = m_keys[key].below;
    }
    
    // if this is not a tied note, trigger the envelope and reset velocity
    // and frequency by setting their slide times to 0
    if (m_active_key == Key::NoKey || !m_tie_overlapping) {
      m_adsr.on(m_last_frame);
      m_vel_slide.reset();
      if (*p(PRT_ON) <= 0)
	m_freq_slide.reset();
    }
    
    // add this key at the top of the key stack
    m_keys[key].held = true;
    m_keys[key].vel = m_velocity;
    m_keys[key].below = m_active_key;
    m_keys[key].above = Key::NoKey;
    if (m_active_key != Key::NoKey)
      m_keys[m_active_key].above = key;
    
    m_active_key = key;
  }
  
  // note off
  else if ((event_data[0] & 0xF0) == 0x80) {
    
    const unsigned char& key = event_data[1];
    
    // if we are not tieing notes, clear the note stack
    if (!m_tie_overlapping) {
      m_active_key = Key::NoKey;
      for (unsigned i = 0; i < 128; ++i) {
	m_keys[i].held = false;
	m_keys[i].above = Key::NoKey;
	m_keys[i].below = Key::NoKey;
      }
    }
    
    // if we are, just remove this note from the note stack
    else if (m_keys[key].held) {
      m_keys[key].held = false;
      if (key == m_active_key)
	m_active_key = m_keys[key].below;
      if (m_keys[key].below != Key::NoKey)
	m_keys[m_keys[key].below].above = m_keys[key].above;
      if (m_keys[key].above != Key::NoKey)
	m_keys[m_keys[key].above].below = m_keys[key].below;
    }
    
    // turn the envelope off if this was the last note, otherwise just
    // update the pitch and velocity
    if (m_active_key == Key::NoKey) 
      m_adsr.off(m_last_frame);
    else {
      m_velocity = m_keys[m_active_key].vel;
      m_pitch = m_table[m_active_key];
    }
  }
  
  // all notes off
  else if ((event_data[0] & 0xF0) == 0xB0 && event_data[1] == 123) {
    m_active_key = Key::NoKey;
    for (unsigned i = 0; i < 128; ++i) {
      m_keys[i].held = false;
      m_keys[i].above = Key::NoKey;
      m_keys[i].below = Key::NoKey;
    }
    m_adsr.off(m_last_frame);
  }
  
  // pitchbend
  else if ((event_data[0] & 0xF0) == 0xE0) {
    int value = -8192 + ((event_data[2] << 7) + event_data[1]);
    m_pitchbend = pow(twelfth_root_of_2, value / 4096.0f);
  }
  
  // all sound off
  else if ((event_data[0] & 0xF0) == 0xB0 && event_data[1] == 120) {
    // XXX This should really turn off the delay as well
    m_active_key = Key::NoKey;
    for (unsigned i = 0; i < 128; ++i) {
      m_keys[i].held = false;
      m_keys[i].above = Key::NoKey;
      m_keys[i].below = Key::NoKey;
    }
    m_adsr.fast_off(m_last_frame);
  }
  
}


void SineShaper::render_audio(uint32_t from, uint32_t to) {

  float freq;
  float vel;
  static const unsigned long param_slide_time = 60;

  // get all the port values and buffers
  float tune = *p(TUN) * pow(2.0f, *p(OCT));
  float subtune = *p(SUB_TUN) * pow(2.0f, *p(SUB_OCT));
  float& osc_mix = *p(OSC_MIX);

  float& porta_on = *p(PRT_ON);
  float& porta_time = *p(PRT_TIM);
  float& vib_freq = *p(VIB_FRQ);
  float& vib_depth = *p(VIB_DPT);
  float& trem_freq = *p(TRM_FRQ);
  float& trem_depth = *p(TRM_DPT);
  
  float& shp_env = *p(SHP_ENV);
  float& shp_total = *p(SHP_TOT);
  float& shp_split = *p(SHP_SPL);
  float shp_shift = *p(SHP_SHF) * M_PI / 2;
  float& lfo_freq = *p(LFO_FRQ);
  float& lfo_depth = *p(LFO_DPT);
  
  float& att = *p(ATT);
  float& dec = *p(DEC);
  float& sus = *p(SUS);
  float& rel = *p(REL);

  float& amp_env = *p(AMP_ENV);
  float& drive = *p(DRIVE);
  float& gain = *p(GAIN);

  float& delay_time = *p(DEL_TIM);
  float& delay_feedback = *p(DEL_FB);
  float& delay_mix = *p(DEL_MIX);
  
  m_tie_overlapping = (*p(PRT_TIE) > 0.5);

  unsigned long freq_slide_time = (unsigned long)(porta_time * m_frame_rate);
  unsigned long vel_slide_time = (unsigned long)(porta_time * m_frame_rate);
  
  float* output = p(OUT);
  
  for (uint32_t i = from; i < to; ++i) {
    
    // don't allow fast changes for controls that can produce clicks
    vel = m_vel_slide.run(m_velocity, vel_slide_time);
    float osc_mix2 = m_osc_mix_slide.run(osc_mix, param_slide_time);
    float trem_depth2 = m_trem_depth_slide.run(trem_depth, 
					       param_slide_time);
    float att2 = m_att_slide.run(att, param_slide_time);
    float dec2 = m_dec_slide.run(dec, param_slide_time);
    float sus2 = m_sus_slide.run(sus, param_slide_time);
    float rel2 = m_rel_slide.run(rel, param_slide_time);
    float shp_env2 = m_shp_env_slide.run(shp_env, param_slide_time);
    float shp_total2 = m_shp_total_slide.run(shp_total, param_slide_time);
    float shp_split2 = m_shp_split_slide.run(shp_split, param_slide_time);
    float shp_shift2 = m_shp_shift_slide.run(shp_shift, param_slide_time);
    float lfo_depth2 = m_lfo_depth_slide.run(lfo_depth, param_slide_time);
    float drive2 = m_drive_slide.run(drive, param_slide_time);
    float gain2 = m_gain_slide.run(gain, param_slide_time);
    float amp_env2 = m_amp_env_slide.run(amp_env, param_slide_time);
    float delay_feedback2 = m_delay_fb_slide.run(delay_feedback, 
						 param_slide_time);
    float delay_mix2 = m_delay_mix_slide.run(delay_mix, param_slide_time);
    
    // portamento
    freq = m_pitchbend * m_freq_slide.run(m_pitch, freq_slide_time);
    
    // add vibrato to the fundamental frequency
    freq += freq * vib_depth * m_vibrato_lfo.run(vib_freq);
    
    // run the envelope
    float env = m_adsr.run(m_last_frame, vel, att2, dec2, sus2, rel2);
    
    // run the oscillators
    float osc1 = m_osc.run(tune * freq);
    float osc2 = m_subosc.run(tune * subtune * freq);
    float osc = ((1 - osc_mix2) * osc1 + osc_mix2 * osc2);
    
    // compute the shape amounts for the two shapers
    shp_total2 *= (1.0f + lfo_depth2 * m_shaper_lfo.run(lfo_freq));
    float shp2 = shp_total2 * shp_split2;
    float shp1 = shp_total2 - shp2;
    
    // run the shapers (and apply the envelope to the shape amount 
    // for shaper 1)
    output[i] = m_shaper.run(m_shaper.run(osc, shp1 * (shp_env2 * env + 
						       (1 - shp_env2)), 0),
			     shp2, shp_shift2);
    
    // apply the envelope to the amplitude
    output[i] = output[i] * env * amp_env2 + output[i] * (1 - amp_env2);
    
    // apply tremolo
    output[i] *= (1.0f +  trem_depth2 * m_tremolo_lfo.run(trem_freq));
    
    // apply gain and overdrive
    output[i] = gain2 * (drive2 * atan((1 + 10 * drive2) * output[i]) + 
			 (1 - drive2) * output[i]);
    
    // run delay
    output[i] = m_delay.run(output[i], delay_time, delay_feedback2) * 
      delay_mix2 + output[i] * (1 - delay_mix2);
    
    // and DC blocker (does this actually help?)
    output[i] = m_blocker.run(output[i]);
    
    ++m_last_frame;
  }
}


void SineShaper::run(uint32_t sample_count) {
  
  LV2_Event_Iterator iter;
  lv2_event_begin(&iter, p<LV2_Event_Buffer>(MIDI));
  uint8_t* event_data;
  uint32_t samples_done = 0;
  
  while (samples_done < sample_count) {
    uint32_t to = sample_count;
    LV2_Event* ev = 0;
    if (lv2_event_is_valid(&iter)) {
      ev = lv2_event_get(&iter, &event_data);
      to = ev->frames;
      lv2_event_increment(&iter);
    }
    
    if (to > samples_done) {
      render_audio(samples_done, to);
      samples_done = to;
    }
    
    if (ev) {
      if (ev->type == 0)
	event_unref(ev);
      else if (ev->type == m_midi_type) {
	handle_midi(event_data);
      }
    }
  }

}


static unsigned _ = SineShaper::register_class("http://ll-plugins.nongnu.org/lv2/sineshaper#0");
