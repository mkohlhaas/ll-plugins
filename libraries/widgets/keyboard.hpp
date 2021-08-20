/****************************************************************************
    
    keyboard.hpp - Simple MIDI keyboard widget
    
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

#ifndef KEYBOARD_HPP
#define KEYBOARD_HPP

#include <map>
#include <vector>

#include <gtkmm/drawingarea.h>
#include <sigc++/signal.h>


/** A keyboard widget that can be played with the mouse or the keyboard.
    The user can click a key on the widget or press a key on the keyboard
    (if the CAN_FOCUS widget flag is set) to "play" a key. When a key in 
    the widget is pressed down the signal returned by signal_key_on() is 
    emitted with the MIDI key number as parameter, and when it is released
    the signal returned by signal_key_off() is emitted. You can also turn
    keys on and off in your program by using the key_on() and key_off() 
    functions, and request the state of a key with get_key_state(). */
class Keyboard : public Gtk::DrawingArea {
public:
  
  /** This enumerates different methods for handling mouse events. */
  enum ClickBehaviour {

    /** This means that the key state is toggled when the mouse button is
        pressed, and nothing happens when the mouse button is released. */
    CLICK_TOGGLE,
    
    /** This means that the key state is set to @c on when the mouse button
        is pressed and to @c off when the mouse button is released. */
    CLICK_ON

  };
  
  /** Constructs a new Keyboard widget.
      @param octaves The number of octaves. The number of keys will be 12 *
                     @c octaves + 1 (i.e. for 1 octave you will get two C keys)
      @param octave_offset The initial octave offset - the number of octaves
                           that the leftmost key is transposed.
      @param keywidth The width in pixels of the white keys
      @param bkeywidth The width in pixels of the black keys
      @param keyheight The height in pixels of the white keys
      @param bkeyheight The height in pixels of the black keys */
  Keyboard(unsigned int octaves = 3, unsigned int octave_offset = 3,
	   unsigned int keywidth = 21, unsigned int bkeywidth = 14, 
	   unsigned int keyheight = 80, unsigned int bkeyheight = 50, 
	   ClickBehaviour cb = CLICK_ON);
  
  /** Turn a key on. If the key is already on nothing happens. */
  void key_on(unsigned char key);

  /** Turn a key off. If the key is already off nothing happens. */
  void key_off(unsigned char key);

  /** Return @c true if the key is on and @c false if it is off. */
  bool get_key_state(unsigned char key) const;
  
  /** This signal is emitted when a key changes state from off to on. The MIDI
      number of the key is given by the parameter. */
  sigc::signal<void, unsigned char>& signal_key_on();

  /** This signal is emitted when a key changes state from on to off. The MIDI
      number of the key is given by the parameter. */
  sigc::signal<void, unsigned char>& signal_key_off();

protected:
  
  /// @name Gtkmm event handlers
  // @{
  virtual bool on_key_press_event(GdkEventKey* event);
  virtual bool on_key_release_event(GdkEventKey* event);
  virtual bool on_button_press_event(GdkEventButton* event);
  virtual bool on_button_release_event(GdkEventButton* event);
  virtual bool on_motion_notify_event(GdkEventMotion* event);
  virtual bool on_scroll_event(GdkEventScroll* event);
  virtual void on_realize();
  virtual bool on_expose_event(GdkEventExpose* event);
  // @}
  
  /** Draw a white key with MIDI number @c k with the left upper corner 
      at pixel coordinate (@c x, 0). If @c on is @c true the key will be drawn
      as if it is pressed. */
  void draw_white_key(unsigned char k, int x, bool on);

  /** Draw a white key with MIDI number @c k with the upper center 
      at pixel coordinate (@c x, 0). If @c on is @c true the key will be drawn
      as if it is pressed. */
  void draw_black_key(int x, bool on);

  /** Return the MIDI number for the key that is at pixel (@c x, @c y). If
      @c only_white_keys is @c true black keys will be ignored, and if @c clamp
      is @c true pixels outside the keyboard will be assumed to be on the 
      nearest edge. If @c clamp is @c false and the pixel is outside the
      keyboard 255 will be returned. */
  unsigned char pixel_to_key(int x, int y, bool only_white_keys = false,
                             bool clamp = false) const;

  /** Return the bounding rectangle for the given key. */
  void key_to_rect(unsigned char k, int& x, int& y, 
                   int& width, int& height) const;

  /** Returns @c true if the given key is a black key. */
  bool is_black(unsigned char k) const;
  
  Glib::RefPtr<Gdk::GC> m_gc;
  Glib::RefPtr<Gdk::Window> m_win;
  Gdk::Color m_white, m_black, m_grey1, m_grey2, m_green1, m_green2;
  
  sigc::signal<void, unsigned char> m_signal_key_on;
  sigc::signal<void, unsigned char> m_signal_key_off;
  
  std::vector<bool> m_keys_on;
  std::map<int, unsigned char> m_keymap;
  
  const unsigned int m_octaves;
  const unsigned int m_keywidth;
  const unsigned int m_bkeywidth;
  const unsigned int m_keyheight;
  const unsigned int m_bkeyheight;
  
  double m_text_size;
  
  unsigned int m_octave_offset;
  
  unsigned char m_moused_key;
  ClickBehaviour m_click_behaviour;
  bool m_motion_turns_on;
};


#endif
