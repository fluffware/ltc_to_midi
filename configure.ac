AC_INIT([ltc_to_midi], [0.1.0])
AM_INIT_AUTOMAKE([foreign])
AC_PREREQ([2.69])

AC_CONFIG_SRCDIR(src/ltc_to_midi.c)
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC

dnl Check for return type of signal handler
AC_TYPE_SIGNAL

AM_PATH_GLIB_2_0(,,,gobject)
AM_PATH_ALSA
AC_CONFIG_FILES([Makefile src/Makefile ])
AC_OUTPUT

