project('gupnp-igd', 'c', version: '0.2.5')

gnome = import('gnome')
pkg = import('pkgconfig')

dependencies = [
    dependency('glib-2.0', version: '>= 2.26', required: true),
    dependency('gobject-2.0', version: '>= 2.26', required: true),
    dependency('gupnp-1.2', version : '>= 1.1.0'),
    dependency('gssdp-1.2', version : '>= 1.1.0'),
    dependency('gthread-2.0', required: true)
]

subdir('libgupnp-igd')
subdir('tests')
if get_option('gtk_doc')
    subdir('doc')
endif
