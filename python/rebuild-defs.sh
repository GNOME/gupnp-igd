#!/bin/sh

HEADERS=" \
    gupnp-simple-igd.h \
    gupnp-simple-igd-thread.h \
    gupnp-enum-types.h"

srcdir=../libgupnp-igd/

output=pygupnp-igd.defs
filter=pygupnp-igd-filters.defs
modulename=gupnp_simple_igd

cat $filter>$output

H2DEF="$(pkg-config --variable=codegendir pygobject-2.0)/h2def.py"
[ -z "${H2DEF}" ] && H2DEF="$(pkg-config --variable=codegendir pygtk-2.0)/h2def.py"
[ -z "${H2DEF}" -a -f /usr/share/pygtk/2.0/codegen/h2def.py ] && H2DEF=/usr/share/pygtk/2.0/codegen/h2def.py

for h in $HEADERS; do
    python ${H2DEF} --defsfilter=${filter} --modulename=${modulename} ${srcdir}/$h >> $output
done

sed -e "/of-object \"GUPnpSimpleIgd\"/ a \
      \  (unblock-threads t)" \
    -i ${output}
