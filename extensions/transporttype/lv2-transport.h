#ifndef LV2_TRANSPORT_H
#define LV2_TRANSPORT_H


/** This struct contains information about the current transport state.
    The port buffer for a LV2 plugin port that has the datatype 
    <http://ll-plugins.nongnu.org/lv2/namespace#transporttype> should be a
    pointer to an instance of this struct.
    
    A host that loads plugins with ports of this type should always set the 
    three fields 'state', 'frame', and 'valid', even if the host has no 
    concept of a transport state (or expect the plugin to set them, if it is
    an output port). In that case 'state' should always be set to 
    LV2_TransportStopped, 'frame' should always be set to 0, and 'valid'
    should always be set to 0. The host does not have to set the rest of the
    fields since the plugin should know that the fact that 'valid' is 0 means
    that there is no meaningful information about things like BPM, current bar,
    beats per bar etc.
    
    This struct is designed to make it easy to just copy values from a
    jack_position_t, especially the BBT fields. It does not contain as 
    much information as a jack_position_t, partly because LV2 plugins don't
    have the same capabilities as a JACK client and partly because I wanted
    to make this LV2 extension as simple as possible.
    
    It should be possible to use this port type for both input and output
    ports. For input ports the host should set all fields and the plugin is not 
    allowed to change them, and for output ports the plugin should set all 
    fields, but can not expect them to stay the same between run() calls,
    since the host may switch the port buffer to point to another LV2_Transport
    instance.
*/    
typedef struct {
  
  /** Transport state. This field is always assumed to be valid. 
      It should be set to a nonzero value when the transport is rolling and 0
      when it isn't. */
  int rolling;
  
  /** Frame position. This field is always assumed to be valid. */
  unsigned long frame;
  
  /** This field is nonzero when the rest of the fields below this one are
      valid and 0 when they are not valid. "Not valid" means that the 
      information in them is not meaningful and should be ignored. */
  int valid;
  
  /** The current bar, starting at 1. */
  int bar;
  
  /** The current beat within the current bar, starting at 1. */
  int beat;
  
  /** The current tick within the current beat, starting at 0. */
  int tick;
  
  /** The current frame within the current tick, starting at 0. */
  unsigned long frame_offset;
  
  /** The number of beats per bar (time signature numerator). */
  float beats_per_bar;
  
  /** The "type" of beat - 4 for quarter notes, 8 for eights etc 
      (time signature denominator). */
  float beat_type;
  
  /** Number of ticks per beat. */
  double ticks_per_beat;
  
  /** Number of beats per minute. Or more correctly, the number of beats
      that would occur each minute _if the transport was rolling_. If the
      transport is not rolling this field should not be set to 0, but rather
      to the BPM at the current song position. This is because BPM-synced
      delays and similar plugins may need to know the BPM even if the transport
      is stopped. 
  */
  double beats_per_minute;
  
} LV2_Transport;


#endif
