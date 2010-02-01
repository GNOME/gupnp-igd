PYDEFS=`pkg-config --variable=defsdir pygobject-2.0`
[ -z "$PYDEFS" && PYDEFS=`pkg-config --variable=defsdir pygtk-2.0`]

AM_CPPFLAGS =						\
	-I.						\
	-I$(top_srcdir)					\
	-DDATADIR=\""$(datadir)"\"			\
	$(PYGUPNP_IGD_CFLAGS)				\
	$(PYTHON_INCLUDES)				\
	$(LIBGUPNP_CFLAGS) 				\
	$(WARN_CFLAGS)

BUILT_SOURCES =						\
	pygupnp-igd.c

pygupnpigddir = $(pyexecdir)/gupnp
pygupnpigd_LTLIBRARIES = igd.la
pygupnpigd_PYTHON = __init__.py

igd_la_SOURCES =					\
	pygupnp-igd-module.c

nodist_igd_la_SOURCES = pygupnp-igd.c

igd_la_LIBADD =						\
	$(PYGUPNP_IGD_LIBS)				\
	$(top_builddir)/libgupnp-igd/libgupnp-igd-1.0.la

igd_la_LDFLAGS =					\
	`python-config --libs`				\
	-module -avoid-version

pygupnp-igd.c: $(srcdir)/pygupnp-igd.override $(srcdir)/pygupnp-igd.defs
	$(PYCODEGEN)						\
		--prefix igd					\
		--override $(srcdir)/pygupnp-igd.override	\
		$(srcdir)/pygupnp-igd.defs > $@

EXTRA_DIST =						\
	pygupnp-igd.defs				\
	pygupnp-igd-filters.defs  			\
	pygupnp-igd.override				\
	rebuild-defs.sh

CLEANFILES = $(BUILT_SOURCES)
