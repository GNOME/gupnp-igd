AC_PREREQ(2.53)
AC_INIT(gupnp-igd, 0.1.10.1, http://www.gupnp.org/)
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(libgupnp-igd/gupnp-simple-igd.h)
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_LIBTOOL
AC_FUNC_MMAP

PKG_CHECK_MODULES(LIBGUPNP, glib-2.0 >= 2.14 gupnp-1.0 >= 0.13.2 gthread-2.0)

# glib-genmarshal
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])

GTK_DOC_CHECK([1.0])

GOBJECT_INTROSPECTION_CHECK([0.10])
if test "x$found_introspection" = "xyes"; then
   INTROSPECTION_TYPELIBDIR=`$PKG_CONFIG --variable=typelibdir --define-variable="libdir=${libdir}" gobject-introspection-1.0`
   INTROSPECTION_GIRDIR=`$PKG_CONFIG --variable=girdir --define-variable="datadir=${datadir}" gobject-introspection-1.0`
   AC_SUBST(INTROSPECTION_TYPELIBDIR)
   AC_SUBST(INTROSPECTION_GIRDIR)
fi

PKG_CHECK_MODULES(GTest, glib-2.0 >= 2.16, [gtest=yes], [gtest=no])

AM_CONDITIONAL(HAVE_GTEST, [test x$gtest = xyes])

AC_ARG_ENABLE([python],
	AC_HELP_STRING([--disable-python], [Disable Python bindings]),
	[case "${enableval}" in
	    yes) WANT_PYTHON=yes ;;
	    no)  WANT_PYTHON=no ;;
	    *) AC_MSG_ERROR(bad value ${enableval} for --enable-python) ;;
        esac],
	WANT_PYTHON=yes)

if test "x$WANT_PYTHON" = "xyes"; then
   AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers)])
   PKG_CHECK_MODULES(PYGUPNP_IGD, [ pygobject-2.0 >= 2.12.0 ] )
   PKG_CHECK_MODULES(PYCODEGEN, [ pygobject-2.0 >= 2.16.0 ],
     PYCODEGEN=pygobject-codegen-2.0,
     PKG_CHECK_MODULES(PYCODEGEN, [ pygtk-2.0 >= 2.10 ],
       PYCODEGEN=pygtk-codegen-2.0,
       AC_MSG_ERROR([You need pygobject 2.16+ or pygtk for the codegen])))
   AM_PATH_PYTHON([], [], [AC_MSG_ERROR([You need python installed])])
fi
AC_SUBST(PYCODEGEN)
AM_CONDITIONAL(WANT_PYTHON, test "x$WANT_PYTHON" = "xyes")

AC_OUTPUT([
Makefile
libgupnp-igd/Makefile
python/Makefile
tests/Makefile
tests/gtest/Makefile
tests/examples/Makefile
doc/Makefile
doc/version.xml
gupnp-igd-1.0.pc
gupnp-igd-1.0-uninstalled.pc
])
