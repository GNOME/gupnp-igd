# current:revision:age
# library source changed -> increment REVISION
# interfaces added/removed/changed -> increment CURRENT, REVISION = 0
# interfaces added -> increment AGE
# interfaces removed -> AGE = 0

LTVERSION = 6:0:2

AM_CFLAGS = $(LIBGUPNP_CFLAGS) -I$(top_srcdir)

libgupnp_igd_incdir = $(includedir)/gupnp-igd-1.0/libgupnp-igd

lib_LTLIBRARIES = libgupnp-igd-1.0.la

libgupnp_igd_inc_HEADERS = \
	gupnp-simple-igd.h \
	gupnp-simple-igd-thread.h

libgupnp_igd_1_0_la_SOURCES = \
	gupnp-simple-igd.c \
	gupnp-simple-igd-thread.c \
	gupnp-simple-igd-priv.h \
	gupnp-enum-types.c \
	gupnp-enum-types.h \
	$(BUILT_SOURCES)

BUILT_SOURCES = \
	gupnp-simple-igd-marshal.c \
	gupnp-simple-igd-marshal.h

gupnp-simple-igd-marshal.h: gupnp-simple-igd-marshal.list Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) --header --prefix=_gupnp_simple_igd_marshal $< > $@.tmp
	@mv $@.tmp $@

gupnp-simple-igd-marshal.c: gupnp-simple-igd-marshal.list Makefile
	$(AM_V_GEN)echo "#include \"glib-object.h\"" > $@.tmp && \
	echo "#include \"gupnp-simple-igd-marshal.h\"" >> $@.tmp && \
	$(GLIB_GENMARSHAL) --body --prefix=_gupnp_simple_igd_marshal $< >> $@.tmp && \
	mv $@.tmp $@

libgupnp_igd_1_0_la_LDFLAGS = -version-info $(LTVERSION) \
	-export-symbols-regex ^gupnp_ \
	$(GUPNP_LDFLAGS)

libgupnp_igd_1_0_la_LIBADD = \
	$(LIBGUPNP_LIBS)

EXTRA_DIST = gupnp-simple-igd-marshal.list


include $(INTROSPECTION_MAKEFILE)
GUPnPIgd-1.0.gir: libgupnp-igd-1.0.la
INTROSPECTION_SCANNER_ARGS = --identifier-prefix=gupnp \
	--identifier-prefix=gupnp_igd \
	--identifier-prefix=GUPnP \
	--identifier-prefix=GUPnPIgd
INTROSPECTION_GIRS = GUPnPIgd-1.0.gir
GUPnPIgd_1_0_gir_NAMESPACE = GUPnPIgd
GUPnPIgd_1_0_gir_VERSION = 1.0
GUPnPIgd_1_0_gir_LIBS = $(lib_LTLIBRARIES)
GUPnPIgd_1_0_gir_FILES = $(libgupnp_igd_1_0_la_SOURCES) $(libgupnp_igd_inc_HEADERS)
GUPnPIgd_1_0_gir_INCLUDES=GObject-2.0
GUPnPIgd_1_0_gir_PACKAGES=gupnp-1.0
GUPnPIgd_1_0_gir_CFLAGS=-I$(srcdir) -I$(top_srcdir)

if HAVE_INTROSPECTION

girdir = $(INTROSPECTION_GIRDIR)
dist_gir_DATA = $(INTROSPECTION_GIRS)
typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = GUPnPIgd-1.0.typelib

endif

CLEANFILES = $(BUILT_SOURCES) $(dist_gir_DATA) $(typelib_DATA)
DISTCLEANFILES = $(BUILT_SOURCES)
MAINTAINERCLEANFILES = Makefile.in $(BUILT_SOURCES)
