simple_test = executable(
    'gupnp-simple-igd',
    files('gupnp-simple-igd.c'),
    dependencies : gupnp_igd,
)

test(
    'gupnp-simple-igd',
    simple_test,
    is_parallel: false,
    env: 'XML_PATH=@0@'.format(meson.current_source_dir())
)
