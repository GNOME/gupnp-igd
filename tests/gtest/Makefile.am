
if HAVE_GTEST

TESTS = $(check_PROGRAMS)

check_PROGRAMS = gupnp-simple-igd

gupnp_simple_igd_SOURCES = gupnp-simple-igd.c

endif

LDADD = \
	$(top_builddir)/libgupnp-igd/libgupnp-igd-1.0.la \
	$(LIBGUPNP_LIBS)

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	-I$(top_srcdir)

