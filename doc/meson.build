entities = configuration_data()
entities.set('VERSION', meson.project_version())
version_xml = configure_file(input: 'version.xml.in',
               output: 'version.xml', configuration:
               entities)

gnome.gtkdoc(
    'gupnp-igd',
    main_xml : 'gupnp-igd-docs.xml',
    src_dir : [
        join_paths(meson.source_root(), 'libgupnp-igd'),
        join_paths(meson.build_root(), 'libgupnp-igd')
    ],
    dependencies : libgupnp_igd,
    scan_args : [
        '--ignore-decorators', 'G_DEPRECATED|G_GNUC_DEPRECATED,G_DEPRECATED_FOR'
    ],
    install : true
)

