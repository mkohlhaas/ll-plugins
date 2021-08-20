/****************************************************************************
    
    lv2host.hpp - Simple LV2 plugin loader for Elven
    
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

#ifndef LV2HOST_HPP
#define LV2HOST_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <pthread.h>
#include <semaphore.h>
#include <dlfcn.h>

#include <sigc++/slot.h>
#include <sigc++/signal.h>

#include <lv2.h>
#include <lv2_event.h>
#include <lv2_uri_map.h>
#include <lv2_saverestore.h>
#include <lv2_contexts.h>
#include <query.hpp>
#include "ringbuffer.hpp"


enum PortDirection {
  InputPort,
  OutputPort,
  NoDirection
};


enum PortType {
  AudioType,
  ControlType,
  MidiType,
  NoType
};


enum PortContext {
  AudioContext,
  MessageContext,
  NoContext
};


/** A struct that holds information about a port in a LV2 plugin. */
struct LV2Port {
  void* buffer;
  std::string symbol;
  PortDirection direction;
  PortType type;
  PortContext context;
  float default_value;
  float min_value;
  float max_value;
  float value;
  float old_value;
  bool notify;
};


struct LV2Preset {
  LV2Preset()
    : files(0),
      elven_override(false) {
  }
  ~LV2Preset() {
    if (files) {
      LV2SR_File** iter = files;
      while (*iter) {
	free((*iter)->name);
	free((*iter)->path);
	free(*iter);
	++iter;
      }
    }
  }
  std::string name;
  std::map<uint32_t, float> values;
  LV2SR_File** files;
  bool elven_override;
};


/** A class that loads a single LV2 plugin. */
class LV2Host {
public:
  
  LV2Host(const std::string& uri, unsigned long frame_rate);
  ~LV2Host();
  
  /** Returns true if the plugin was loaded OK. */
  bool is_valid() const;
  
  /** Returns the URI for the loaded plugin. */
  const std::string& get_plugin_uri() const;
  
  /** Returns a vector of objects containing information about the plugin's 
      ports. */
  const std::vector<LV2Port>& get_ports() const;

  /** Returns a vector of objects containing information about the plugin's 
      ports. */
  std::vector<LV2Port>& get_ports();
  
  /** Returns the index of the default MIDI port, or -1 if there is no default
      MIDI port. */
  long get_default_midi_port() const;
  
  /** Return the MIDI controller mappings. */
  const std::vector<int>& get_midi_map() const;
  
  /** Return the path to the SVG icon file. */
  const std::string& get_icon_path() const;
  
  /** Return the path to the GUI plugin module. */
  const std::string& get_gui_path() const;
  
  /** Return the URI for the GUI plugin. */
  const std::string& get_gui_uri() const;
  
  /** Return the path to the plugin bundle. */
  const std::string& get_bundle_dir() const;
  
  /** Return the name of the plugin. */
  const std::string& get_name() const;
  
  /** Returns all found presets. */
  const std::map<unsigned, LV2Preset>& get_presets() const;
  
  /** Activate the plugin. The plugin must be activated before you call the
      run() function. */
  void activate();
  
  /** Tell the plugin to produce @c nframes frames of signal. */
  void run(unsigned long nframes);
  
  /** Deactivate the plugin. */
  void deactivate();
  
  /** Set a control port value. */
  void set_control(uint32_t index, float value);
  
  /** Set the plugin program. */
  void set_program(unsigned char program);
  
  /** Save the current state as a program. */
  void save_program(unsigned char program, const char* name);
  
  /** Run the blocking message context. */
  void message_run();
  
  /** Queue an event. */
  void queue_event(uint32_t port, uint16_t type, 
		   uint32_t size, const uint8_t* midi);
  
  /** Queue an entire event buffer. */
  void queue_events(uint32_t port, const LV2_Event_Buffer* buffer);
  
  /** Save the current state of the plugin. */
  bool save(const std::string& directory, LV2SR_File*** files);
  
  /** Restore the plugin to a previously saved state. */
  bool restore(const LV2SR_File** files);
  
  /** List all available plugins. */
  static void list_plugins();
  
  /** Run some checks and fire signals in the main thread. */
  void run_main();
  
  sigc::signal<void, uint32_t, uint32_t, uint32_t, const void*> 
  signal_port_event;
  
  sigc::signal<void, unsigned char> signal_program_changed;
  
  sigc::signal<void, unsigned char, const char*> signal_program_added;
  
protected:
  
  static std::vector<std::string> get_search_dirs();
  
  typedef sigc::slot<bool, const std::string&> scan_callback_t;
  
  struct Event {
    Event(uint32_t p, uint16_t t, uint32_t s, const uint8_t* d) 
      : port(p), type(t), event_size(s), data(0), written(false) {
      data = new uint8_t[event_size];
      std::memcpy(data, d, event_size);
    }
    ~Event() {
      delete [] data;
    }
    uint32_t port;
    uint16_t type;
    uint32_t event_size;
    uint8_t* data;
    bool written;
  };
  
  static bool scan_manifests(const std::vector<std::string>& search_dirs, 
                             scan_callback_t callback);
                      
  bool match_uri(const std::string& bundle);

  bool match_partial_uri(const std::string& bundle);
  
  bool parse_ports(PAQ::RDFData& data, const std::string& parent, 
		   const std::string& predicate, PortContext context);
  
  bool load_plugin();
  
  bool add_preset(const LV2Preset& preset, int program = -1);
  
  void merge_presets();
  
  void load_presets_from_uri(const std::string& fileuri, bool user);
  
  static bool print_uri(const std::string& bundle);
  
  static uint32_t uri_to_id(LV2_URI_Map_Callback_Data callback_data,
			    const char* umap, const char* uri);
  
  static uint32_t event_ref(LV2_Event_Callback_Data, LV2_Event*);

  static uint32_t event_unref(LV2_Event_Callback_Data, LV2_Event*);
  
  static void request_run(void* host_handle, const char* context_uri);
  
  static bool create_user_data_bundle();
  
  static std::string uri_to_preset_filename(const std::string& uri);
  
  template <typename T> T get_symbol(const std::string& name) {
    union {
      void* s;
      T t;
    } u;
    u.s = dlsym(m_libhandle, name.c_str());
    return u.t;
  }

  
  std::string m_uri;
  std::string m_bundle;
  std::vector<std::string> m_rdffiles;
  std::string m_binary;
  uint32_t m_rate;
  
  void* m_libhandle;
  LV2_Handle m_handle;
  const LV2_Descriptor* m_desc;
  const LV2SR_Descriptor* m_sr_desc;
  const LV2_Blocking_Context* m_msg_desc;
  
  LV2_URI_Map_Feature m_urimap_host_desc;
  LV2_Event_Feature m_event_host_desc;
  LV2_Context_Feature m_context_host_desc;
  
  std::vector<LV2Port> m_ports;
  size_t m_ports_used;
  bool m_ports_updated;
  std::vector<int> m_midimap;
  long m_default_midi_port;
  std::string m_iconpath;
  std::string m_plugingui;
  std::string m_guiuri;
  std::string m_bundledir;
  std::string m_name;
  
  unsigned long m_program;
  bool m_program_is_valid;
  bool m_new_program;
  
  std::vector<Event*> m_midi_events;
  
  // big lock
  pthread_mutex_t m_mutex;
  
  // output notification semaphore
  sem_t m_notification_sem;
  
  std::map<unsigned, LV2Preset> m_presets;
  std::vector<LV2Preset> m_tmp_presets;
  
  static std::string m_user_data_bundle;
  unsigned m_next_free_preset;
};


#endif
