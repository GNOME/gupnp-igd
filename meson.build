project('gupnp-igd', 'c', version: '1.6.0')

gnome = import('gnome')
pkg = import('pkgconfig')

glib_req = '>= 2.70'
dependencies = [
    dependency('glib-2.0', version: glib_req, required: true),
    dependency('gobject-2.0', version: glib_req, required: true),
    dependency('gupnp-1.6', version : '>= 1.6.0'),
    dependency('gssdp-1.6', version : '>= 1.6.0'),
    dependency('gthread-2.0', required: true)
]

glib_req_minmax_str = glib_req.split().get(1).underscorify()
add_project_arguments(
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_' + glib_req_minmax_str,
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_' + glib_req_minmax_str,
  language: 'c')


subdir('libgupnp-igd')
subdir('tests')
if get_option('gtk_doc')
    subdir('doc')
endif
