AM_CPPFLAGS= -I. @GLIB_CFLAGS@
bin_PROGRAMS = ltc_to_midi
ltc_to_midi_SOURCES = ltc_to_midi.c soundio.c soundio_common.c soundio_alsa.c audio_sink.c audio_source.c audio_link.c LTC_reader.c timecode_source.c userbits_source.c mtc_sender.c gmarshal.c
ltc_to_midi_LDADD =  @GLIB_LIBS@

GLIB_GENMARSHAL=glib-genmarshal
gmarshal.h: gmarshal.h

gmarshal.h: gmarshal.list
	$(GLIB_GENMARSHAL) --header --prefix=sync $< >$@

gmarshal.c: gmarshal.list
	$(GLIB_GENMARSHAL) --body --prefix=sync $< >$@

NO_DIST=gmarshal.c

EXTRA_DIST=gmarshal.list

BUILT_SOURCES= gmarshal.h gmarshal.c
