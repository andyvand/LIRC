ACLOCAL_AMFLAGS             = -I m4

AM_LDFLAGS                  = -module -avoid-version -L../lib -llirc_driver
AM_CFLAGS                   =
AM_CPPFLAGS                 = -I../lib -I@kerneldir@

DISTCLEANFILES              = pluginlist.am

plugindir                   =  $(pkglibdir)/plugins
EXTRA_DIST                  = pluginlist.am make-pluginlist.sh
plugin_LTLIBRARIES          =

if BUILD_CARACA
plugin_LTLIBRARIES         += hw_caraca.la
hw_caraca_la_SOURCES        = hw_caraca.c hw_caraca.h
hw_caraca_la_LDFLAGS        = $(AM_LDFLAGS) -lcaraca_client
endif

if BUILD_USB
plugin_LTLIBRARIES         += hw_atilibusb.la
hw_atilibusb_la_SOURCES     = hw_atilibusb.c
hw_atilibusb_la_LDFLAGS     = $(AM_LDFLAGS) @usb_libs@
hw_atilibusb_la_CFLAGS      = $(AM_CFLAGS) $(LIBUSB_CFLAGS)

plugin_LTLIBRARIES         += hw_awlibusb.la
hw_awlibusb_la_SOURCES      = hw_awlibusb.c
hw_awlibusb_la_LDFLAGS      = $(AM_LDFLAGS) @usb_libs@
hw_awlibusb_la_CFLAGS       = $(AM_CFLAGS) $(LIBUSB_CFLAGS)

plugin_LTLIBRARIES         += hw_dfclibusb.la
hw_dfclibusb_la_SOURCES     = hw_dfclibusb.c
hw_dfclibusb_la_LDFLAGS     = $(AM_LDFLAGS) @usb_libs@
hw_dfclibusb_la_CFLAGS      = $(AM_CFLAGS) $(LIBUSB_CFLAGS)

plugin_LTLIBRARIES         += hw_commandir.la
hw_commandir_la_SOURCES     = hw_commandir.c hw_commandir.h
hw_commandir_la_LDFLAGS     = $(AM_LDFLAGS) @usb_libs@
hw_commandir_la_CFLAGS      = $(AM_CFLAGS) $(LIBUSB_CFLAGS)

plugin_LTLIBRARIES         += hw_srm7500libusb.la
hw_srm7500libusb_la_SOURCES = hw_srm7500libusb.c hw_srm7500libusb.h
hw_srm7500libusb_la_LDFLAGS = $(AM_LDFLAGS) @usb_libs@
hw_srm7500libusb_la_CFLAGS  = $(AM_CFLAGS) $(LIBUSB_CFLAGS)
endif

if BUILD_FTDI
plugin_LTLIBRARIES         += hw_ftdi.la
hw_ftdi_la_SOURCES          = hw_ftdi.c
hw_ftdi_la_LDFLAGS          = $(AM_LDFLAGS) -lftdi
endif

if BUILD_IGUANAIR
plugin_LTLIBRARIES         += hw_iguanaIR.la
hw_iguanaIR_la_SOURCES      = hw_iguanaIR.c
hw_iguanaIR_la_LDFLAGS      = $(AM_LDFLAGS) -liguanaIR
endif

if BUILD_LIBIRMAN
plugin_LTLIBRARIES         += hw_irman.la
hw_irman_la_SOURCES         = hw_irman.c
hw_irman_la_LDFLAGS         = $(AM_LDFLAGS) -lirman
endif

if BUILD_LIBIRMAN_SW
plugin_LTLIBRARIES         += hw_irman_sw.la
hw_irman_sw_la_SOURCES      = hw_irman.c hw_irman.h
hw_irman_sw_la_LDFLAGS      = $(AM_LDFLAGS) -lirman_sw
endif

if BUILD_LIBPORTAUDIO
plugin_LTLIBRARIES         += hw_audio.la
hw_audio_la_SOURCES         = hw_audio.c
hw_audio_la_LDFLAGS         = $(AM_LDFLAGS) -lportaudio \
                              $(portaudio_lib_other)  @forkpty@
endif

if BUILD_LIBALSA
plugin_LTLIBRARIES         += hw_audio_alsa.la
hw_audio_alsa_la_SOURCES    = hw_audio_alsa.c
hw_audio_alsa_la_LDFLAGS    = $(AM_LDFLAGS) -lasound
endif

if BUILD_ALSA_SB_RC
plugin_LTLIBRARIES         += hw_alsa_usb.la
hw_alsa_usb_la_SOURCES      = hw_alsa_usb.c
hw_alsa_usb_la_LDFLAGS      = $(AM_LDFLAGS) -lasound
endif

if BUILD_I2CUSER
plugin_LTLIBRARIES         += hw_i2cuser.la
hw_i2cuser_la_SOURCES       = hw_i2cuser.c
endif

if BUILD_DSP
plugin_LTLIBRARIES         += hw_dsp.la
hw_dsp_la_SOURCES           = hw_dsp.c
endif

plugin_LTLIBRARIES         += hw_atwf83.la
hw_atwf83_la_SOURCES        = hw_atwf83.c
hw_atwf83_la_LDFLAGS        = $(AM_LDFLAGS) -lpthread

plugin_LTLIBRARIES           += hw_livedrive_midi.la
hw_livedrive_midi_la_SOURCES  = hw_livedrive_midi.c hw_livedrive_common.h \
                                hw_livedrive_common.c

plugin_LTLIBRARIES           += hw_livedrive_seq.la
hw_livedrive_seq_la_SOURCES   = hw_livedrive_seq.c hw_livedrive_common.h \
                                hw_livedrive_common.c

plugin_LTLIBRARIES           += hw_uirt2.la
hw_uirt2_la_SOURCES           = hw_uirt2.c hw_uirt2_common.c hw_uirt2_common.h

plugin_LTLIBRARIES           += hw_uirt2_raw.la
hw_uirt2_raw_la_SOURCES       = hw_uirt2_raw.c hw_uirt2_common.c hw_uirt2_common.h

dist-hook:
	$(srcdir)/make-pluginlist.sh > pluginlist.am


include pluginlist.am
