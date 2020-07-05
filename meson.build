project('doocs-xdr-rpc', 'c',
    version : '1.0',
    default_options : [
        #'c_std=c',
        'warning_level=0',
        'buildtype=release',
    ],
    meson_version : '>=0.49'
)

if host_machine.system() != 'windows'
    error('This is a Windows-only specific port, use tirpc on other systems')
endif

inc = include_directories('include')
    subdir('src' / 'core')

minirpc_dep = declare_dependency(
    dependencies : deps,
    link_with : librpc,
)

minirpc_static_dep = declare_dependency(
    dependencies : deps,
    link_with : librpc_static,
)

# vi:ts=4:sw=4:sts=4:et:syn=conf
