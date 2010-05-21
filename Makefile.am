
if WANT_PYTHON
PYTHON_SUBDIR = python
else
PYTHON_SUBDIR =
endif

DIST_SUBDIRS = libgupnp-igd python tests doc

SUBDIRS = libgupnp-igd $(PYTHON_SUBDIR) tests doc

ACLOCAL_AMFLAGS = -I m4

pkgconfig_DATA= gupnp-igd-1.0.pc
pkgconfigdir = $(libdir)/pkgconfig

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing config.h.in

dist-hook:
	chmod u+w ${distdir}/ChangeLog
	if test -d .git; then git log >${distdir}/ChangeLog; fi
