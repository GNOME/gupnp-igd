AC_PREREQ(2.53)
AC_INIT(gupnp-igd, 0.1.2, http://www.gupnp.org/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(libgupnp-igd/gupnp-simple-igd.h)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL
AC_FUNC_MMAP

PKG_CHECK_MODULES(LIBGUPNP, glib-2.0 >= 2.14 gupnp-1.0 >= 0.12.3 gthread-2.0)

# glib-genmarshal
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GTK_DOC_CHECK([1.0])

PKG_CHECK_MODULES(GTest, glib-2.0 >= 2.16, [gtest=yes], [gtest=no])

AM_CONDITIONAL(HAVE_GTEST, [test x$gtest = xyes])

AC_OUTPUT([
Makefile
libgupnp-igd/Makefile
tests/Makefile
tests/gtest/Makefile
tests/examples/Makefile
doc/Makefile
doc/version.xml
gupnp-igd-1.0.pc
gupnp-igd-1.0-uninstalled.pc
])
