marshal = gnome.genmarshal(
    'gupnp-simple-igd-marshal',
    sources : 'gupnp-simple-igd-marshal.list',
    prefix : '_gupnp_simple_igd_marshal'
)

headers = files(
    'gupnp-simple-igd.h',
    'gupnp-simple-igd-thread.h'
)

install_headers(headers, subdir: 'gupnp-igd-1.0/libgupnp-igd')

sources = files(
    'gupnp-enum-types.c',
    'gupnp-simple-igd.c',
    'gupnp-simple-igd-thread.c'
)

libgupnp_igd = library(
    'gupnp-igd-1.0',
    sources + marshal,
    version: '4.2.0',
    include_directories: include_directories('..'),
    dependencies : dependencies,
    install: true)

gupnp_igd = declare_dependency(
    link_with : libgupnp_igd,
    include_directories : include_directories('..'),
    dependencies: dependencies
)

pkg.generate(
    libraries : libgupnp_igd,
    name : 'gupnp-igd-1.0',
    subdirs : 'gupnp-igd-1.0',
    filebase : 'gupnp-igd-1.0',
    description: 'GUPnP Simple IGD library',
    requires : 'gupnp-1.2'
)


if get_option('introspection')
    gnome.generate_gir(
        libgupnp_igd,
        sources : headers + sources,
        namespace : 'GUPnPIgd',
        nsversion : '1.0',
        symbol_prefix: ['gupnp'],
        identifier_prefix : 'GUPnP',
        install: true,
        export_packages : 'gupnp-1.0',
        includes : ['GObject-2.0']
    )
endif
