## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = gmrun

gmrun_SOURCES = \
	gtkcompletionline.cc gtkcompletionline.h \
	history.cc history.h \
	main.cc main.h

gmrun_LDFLAGS = -s

gmrun_LDADD = @GTK_LIBS@

