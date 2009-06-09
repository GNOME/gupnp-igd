# current:revision:age
# library source changed -> increment REVISION
# interfaces added/removed/changed -> increment CURRENT, REVISION = 0
# interfaces added -> increment AGE
# interfaces removed -> AGE = 0

LTVERSION = 2:1:0

AM_CFLAGS = $(LIBGUPNP_CFLAGS) -I$(top_srcdir)

libgupnp_igd_incdir = $(includedir)/gupnp-igd-1.0/libgupnp-igd

lib_LTLIBRARIES = libgupnp-igd-1.0.la

libgupnp_igd_inc_HEADERS = \
	gupnp-simple-igd.h \
	gupnp-simple-igd-thread.h

libgupnp_igd_1_0_la_SOURCES = \
	gupnp-simple-igd.c \
	gupnp-simple-igd-thread.c \
	gupnp-simple-igd-marshal.c


noinst_HEADERS = gupnp-simple-igd-marshal.h


BUILT_SOURCES = \
	gupnp-simple-igd-marshal.c \
	gupnp-simple-igd-marshal.h

gupnp-simple-igd-marshal.h: gupnp-simple-igd-marshal.list Makefile
	glib-genmarshal --header --prefix=_gupnp_simple_igd_marshal $(srcdir)/$< > $@.tmp
	mv $@.tmp $@

gupnp-simple-igd-marshal.c: gupnp-simple-igd-marshal.list Makefile
	echo "#include \"glib-object.h\"" >> $@.tmp
	echo "#include \"gupnp-simple-igd-marshal.h\"" >> $@.tmp
	glib-genmarshal --body --prefix=_gupnp_simple_igd_marshal $(srcdir)/$< >> $@.tmp
	mv $@.tmp $@

libgupnp_igd_1_0_la_LDFLAGS = -version-info $(LTVERSION) \
	$(GUPNP_LDFLAGS)

libgupnp_igd_1_0_la_LIBADD = \
	$(LIBGUPNP_LIBS)

EXTRA_DIST = gupnp-simple-igd-marshal.list

CLEANFILES = $(BUILT_SOURCES)
DISTCLEANFILES = $(BUILT_SOURCES)
MAINTAINERCLEANFILES = Makefile.in $(BUILT_SOURCES)

