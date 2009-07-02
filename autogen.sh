#! /bin/sh
gtkdocize || exit 1
autoreconf -v --install || exit 1
glib-gettextize --force --copy || exit 1
ACLOCAL_AMFLAGS = -I m4
./configure --enable-gtk-doc "$@"
