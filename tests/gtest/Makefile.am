
if HAVE_GTEST

TESTS_ENVIRONMENT = XML_PATH=$(srcdir) \
	LD_LIBRARY_PATH=$(top_builddir)/libgupnp-igd/.libs

TESTS = $(check_PROGRAMS)

check_PROGRAMS = gupnp-simple-igd

gupnp_simple_igd_SOURCES = gupnp-simple-igd.c

endif

LDADD = \
	$(top_builddir)/libgupnp-igd/libgupnp-igd-1.0.la \
	$(LIBGUPNP_LIBS)

AM_CFLAGS = $(LIBGUPNP_CFLAGS) \
	-I$(top_srcdir)

EXTRA_DIST = InternetGatewayDevice.xml WANIPConnection.xml WANPPPConnection.xml
