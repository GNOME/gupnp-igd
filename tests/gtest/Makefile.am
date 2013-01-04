
TESTS_ENVIRONMENT = XML_PATH=$(srcdir) \
	G_SLICE=debug-blocks \
	LD_LIBRARY_PATH=$(top_builddir)/libgupnp-igd/.libs:$(LD_LIBRARY_PATH)

TESTS = $(check_PROGRAMS)

check_PROGRAMS = gupnp-simple-igd

gupnp_simple_igd_SOURCES = gupnp-simple-igd.c

LDADD = \
	$(top_builddir)/libgupnp-igd/libgupnp-igd-1.0.la \
	$(LIBGUPNP_LIBS)

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	-I$(top_srcdir)

EXTRA_DIST = InternetGatewayDevice.xml WANIPConnection.xml WANPPPConnection.xml
