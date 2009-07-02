PYTHON_SUBDIR = python

SUBDIRS = libgupnp-igd $(PYTHON_SUBDIR) tests doc common-modified m4

ACLOCAL_AMFLAGS = -I m4

pkgconfig_DATA= gupnp-igd-1.0.pc
pkgconfigdir = $(libdir)/pkgconfig

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Extra clean files so that maintainer-clean removes *everything*
MAINTAINERCLEANFILES = aclocal.m4 compile config.guess config.sub configure depcomp install-sh ltmain.sh Makefile.in missing config.h.in

dist-hook:
	chmod u+w ${distdir}/ChangeLog
	if test -d .svn; then svn log >${distdir}/ChangeLog; fi
