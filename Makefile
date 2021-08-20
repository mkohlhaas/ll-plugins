PACKAGE_NAME = ll-plugins
PACKAGE_VERSION = $(shell ./VERSION)
PACKAGE_WEBPAGE = "https://www.nongnu.org/ll-plugins/"
PACKAGE_BUGTRACKER = "https://savannah.nongnu.org/bugs/?group=ll-plugins"
PACKAGE_VC = "https://git.savannah.nongnu.org/cgit/ll-plugins.git"

PACKAGE_DESCRIPTION = \
	ll-plugins is a collection of LV2 plugins and a simple LV2 host.

PKG_DEPS = \
	cairomm-1.0>=1.2.4 \
	gtkmm-2.4>=2.8.8 \
	jack>=0.109.0 \
	lv2-plugin>=1.0.0 \
	lv2-gui>=1.0.0 \
	paq>=1.0.0 \
	sndfile>=1.0.18 \
	samplerate>=0.1.2


# Disable all deprecation warnings from Glib, they are caused by GTK and GDK,
# not us.
IGNORE_DEPRECATIONS = -DGLIB_DISABLE_DEPRECATION_WARNINGS


DOCS = AUTHORS COPYING INSTALL ChangeLog README


ARCHIVES = \
	libkeyboard.a \
	libvuwidget.a

PROGRAMS = elven

LV2_BUNDLES = \
	arpeggiator.lv2 \
	control2midi.lv2 \
	klaviatur.lv2 \
	klaviatur_gtk.lv2 \
	math-constants.lv2 \
	math-functions.lv2 \
	peakmeter.lv2 \
	peakmeter_gtk.lv2 \
	rudolf556.lv2 \
	rudolf556_gtk.lv2 \
	sineshaper.lv2 \
	sineshaper_gtk.lv2


# Archives with useful code bits

libkeyboard_a_SOURCES = keyboard.hpp keyboard.cpp
libkeyboard_a_CFLAGS = `pkg-config --cflags gtkmm-2.4` $(IGNORE_DEPRECATIONS)
libkeyboard_a_SOURCEDIR = libraries/widgets

libvuwidget_a_SOURCES = vuwidget.hpp vuwidget.cpp
libvuwidget_a_CFLAGS = `pkg-config --cflags gtkmm-2.4` $(IGNORE_DEPRECATIONS)
libvuwidget_a_SOURCEDIR = libraries/widgets


# Executable programs

elven_SOURCES = \
	debug.hpp \
	lv2guihost.hpp lv2guihost.cpp \
	lv2host.hpp lv2host.cpp \
	main.cpp \
	midiutils.hpp
elven_CFLAGS = `pkg-config --cflags jack gtkmm-2.4 sigc++-2.0 lv2-plugin lv2-gui paq` -Ilibraries/components -DVERSION=\"$(PACKAGE_VERSION)\" $(IGNORE_DEPRECATIONS)
elven_LDFLAGS = `pkg-config --libs jack gtkmm-2.4 sigc++-2.0 paq` -lpthread -ldl
elven_SOURCEDIR = programs/elven


# The plugins

PLUGINARCHIVES = `pkg-config --libs lv2-plugin`
PLUGINCFLAGS = `pkg-config --cflags lv2-plugin`
ADVANCEDARCHIVES = $(PLUGINARCHIVES)
ADVANCEDCFLAGS = $(PLUGINCFLAGS)

# Control2MIDI
control2midi_lv2_MODULES = control2midi.so
control2midi_lv2_DATA = manifest.ttl control2midi.ttl
control2midi_lv2_PEGFILES = control2midi.peg
control2midi_lv2_SOURCEDIR = plugins/control2midi
control2midi_so_SOURCES = control2midi.cpp
control2midi_so_CFLAGS = $(PLUGINCFLAGS)
control2midi_so_LDFLAGS = $(PLUGINARCHIVES)

# Klaviatur
klaviatur_lv2_MODULES = klaviatur.so
klaviatur_lv2_DATA = manifest.ttl klaviatur.ttl
klaviatur_lv2_PEGFILES = klaviatur.peg
klaviatur_lv2_SOURCEDIR = plugins/klaviatur
klaviatur_so_SOURCES = klaviatur.cpp
klaviatur_so_CFLAGS = $(PLUGINCFLAGS) -Ilibraries/components
klaviatur_so_LDFLAGS = $(PLUGINARCHIVES)

# Klaviatur GUI
klaviatur_gtk_lv2_MODULES = klaviatur_gtk.so
klaviatur_gtk_lv2_MANIFEST = gui_manifest.ttl
klaviatur_gtk_lv2_SOURCEDIR = plugins/klaviatur
klaviatur_gtk_so_SOURCES = klaviatur_gtk.cpp
klaviatur_gtk_so_CFLAGS = `pkg-config --cflags lv2-gui` -Ilibraries/widgets $(IGNORE_DEPRECATIONS)
klaviatur_gtk_so_LDFLAGS = `pkg-config --libs lv2-gui` -z nodelete
klaviatur_gtk_so_ARCHIVES = libraries/widgets/libkeyboard.a

# Arpeggiator
arpeggiator_lv2_MODULES = arpeggiator.so
arpeggiator_so_SOURCES = arpeggiator.cpp
arpeggiator_so_CFLAGS = $(PLUGINCFLAGS)
arpeggiator_so_LDFLAGS = $(PLUGINARCHIVES)
arpeggiator_lv2_DATA = manifest.ttl arpeggiator.ttl
arpeggiator_lv2_SOURCEDIR = plugins/arpeggiator

# Peak meter
peakmeter_lv2_MODULES = peakmeter.so
peakmeter_so_SOURCES = peakmeter.cpp
peakmeter_lv2_PEGFILES = peakmeter.peg
peakmeter_so_CFLAGS = $(PLUGINCFLAGS)
peakmeter_so_LDFLAGS = $(PLUGINARCHIVES)
peakmeter_lv2_DATA = manifest.ttl peakmeter.ttl icon.svg
peakmeter_lv2_SOURCEDIR = plugins/peakmeter

# Peak meter GUI
peakmeter_gtk_lv2_MODULES = peakmeter_gtk.so
peakmeter_gtk_lv2_SOURCEDIR = plugins/peakmeter
peakmeter_gtk_lv2_MANIFEST = gui_manifest.ttl
peakmeter_gtk_so_SOURCES = peakmeter_gtk.cpp
peakmeter_gtk_so_CFLAGS = `pkg-config --cflags gtkmm-2.4 lv2-gui` -Ilibraries/widgets $(IGNORE_DEPRECATIONS)
peakmeter_gtk_so_LDFLAGS = `pkg-config --libs gtkmm-2.4 lv2-gui` 
peakmeter_gtk_so_ARCHIVES = libraries/widgets/libvuwidget.a

# Math constants
math-constants_lv2_MODULES = math-constants.so
math-constants_so_SOURCES = math-constants.cpp
math-constants_so_CFLAGS = `pkg-config --cflags lv2-plugin`
math-constants_so_LDFLAGS = `pkg-config --libs lv2-plugin`
math-constants_lv2_DATA = \
	manifest.ttl math-constants.ttl \
	e.svg pi.svg pi_2.svg pi_4.svg sqrt2.svg \
	1_pi.svg 1_sqrt2.svg 2_pi.svg 2_sqrtpi.svg
math-constants_lv2_SOURCEDIR = plugins/math-constants

# Math functions
math-functions_lv2_MODULES = math-functions.so
math-functions_so_SOURCES = math-functions.cpp
math-functions_so_CFLAGS = `pkg-config --cflags lv2-plugin`
math-functions_so_LDFLAGS = `pkg-config --libs lv2-plugin`
math-functions_lv2_DATA = manifest.ttl math-functions.ttl
math-functions_lv2_SOURCEDIR = plugins/math-functions

# Sineshaper
sineshaper_lv2_MODULES = sineshaper.so
sineshaper_lv2_DATA = manifest.ttl sineshaper.ttl presets.ttl icon.svg
sineshaper_lv2_SOURCEDIR = plugins/sineshaper
sineshaper_lv2_PEGFILES = sineshaper.peg
sineshaper_so_SOURCES = \
	adsr.hpp \
	delay.hpp \
	frequencytable.hpp \
	midiiterator.hpp \
	sineoscillator.hpp \
	sineshaper.hpp sineshaper.cpp \
	sineshaperports.hpp \
	slide.hpp \
	wavewrapper.hpp
sineshaper_so_CFLAGS = `pkg-config --cflags lv2-plugin` -Ilibraries/components
sineshaper_so_LDFLAGS = `pkg-config --libs lv2-plugin`

# Sineshaper GUI
sineshaper_gtk_lv2_MODULES = sineshaper_gtk.so
sineshaper_gtk_lv2_MANIFEST = gui_manifest.ttl
sineshaper_gtk_lv2_DATA = dial.png sineshaper.png icon.svg
sineshaper_gtk_lv2_SOURCEDIR = plugins/sineshaper
sineshaper_gtk_so_SOURCES = \
	sineshaper_gtk.cpp \
	sineshaperwidget.cpp sineshaperwidget.hpp \
	skindial_gtkmm.cpp skindial_gtkmm.hpp
sineshaper_gtk_so_CFLAGS = `pkg-config --cflags lv2-gui` $(IGNORE_DEPRECATIONS)
sineshaperwidget_cpp_CFLAGS = -DVERSION=\"$(PACKAGE_VERSION)\"
sineshaper_gtk_so_LDFLAGS = `pkg-config --libs lv2-gui` 
sineshaper_gtk_so_SOURCEDIR = plugins/sineshaper

# Rudolf 556
rudolf556_lv2_MODULES = rudolf556.so
rudolf556_lv2_DATA = \
	manifest.ttl rudolf556.ttl \
	icon.svg \
	bass_h00.flac bass_h05.flac bass_h10.flac \
	snare_bonk.flac snare_noise.flac \
	hihat_base.flac hihat_hit.flac
rudolf556_lv2_SOURCEDIR = plugins/rudolf556
rudolf556_lv2_PEGFILES = rudolf556.peg
rudolf556_so_SOURCES = rudolf556.cpp
rudolf556_so_CFLAGS = `pkg-config --cflags lv2-plugin sndfile samplerate`
rudolf556_so_LDFLAGS = `pkg-config --libs lv2-plugin sndfile samplerate`

# Rudolf 556 GUI
rudolf556_gtk_lv2_MODULES = rudolf556_gtk.so
rudolf556_gtk_lv2_MANIFEST = gui_manifest.ttl
rudolf556_gtk_lv2_DATA = rudolf556.png
rudolf556_gtk_lv2_SOURCEDIR = plugins/rudolf556
rudolf556_gtk_so_SOURCES = \
	rudolf556_gtk.cpp \
	rudolf556widget.cpp rudolf556widget.hpp
rudolf556_gtk_so_CFLAGS = `pkg-config --cflags lv2-gui` $(IGNORE_DEPRECATIONS)
rudolf556_gtk_so_LDFLAGS = `pkg-config --libs lv2-gui` 



# The shared headers need to go in the distribution too
EXTRA_DIST = \
	libraries/components/adsr.hpp \
	libraries/components/chebyshevshaper.hpp \
	libraries/components/dcblocker.hpp \
	libraries/components/delay.hpp \
	libraries/components/delayline.hpp \
	libraries/components/envelopegenerator.hpp \
	libraries/components/filter.hpp \
	libraries/components/frequencytable.hpp \
	libraries/components/householderfdn.hpp \
	libraries/components/ladspawrapper.hpp \
	libraries/components/markov.hpp \
	libraries/components/monophonicmidinote.hpp \
	libraries/components/monostep.hpp \
	libraries/components/mooglpf.hpp \
	libraries/components/pdosc.hpp \
	libraries/components/polyphonicmidinote.hpp \
	libraries/components/programmanager.hpp \
	libraries/components/randomsineoscillator.hpp \
	libraries/components/ringbuffer.hpp \
	libraries/components/sineoscillator.hpp \
	libraries/components/slide.hpp \
	libraries/components/voicehandler.hpp \
	libraries/components/wavewrapper.hpp


# Do the magic
include Makefile.template
