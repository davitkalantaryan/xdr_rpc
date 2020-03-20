src = files([
    #'doocs_rpc_auth_functions.c',
    'doocs_rpc_entry.c',
    'doocs_rpc_xdr_functions.c',
    'doocs_rpc_xdr_mem.c',
])

winsock_dep = meson.get_compiler('c').find_library('Ws2_32')

deps = [
    declare_dependency(
        include_directories : inc,
    ),
    winsock_dep,
]

librpc = shared_library(
    meson.project_name(),
    src,
    dependencies : deps,
    soversion : meson.project_version(),
    c_args : [ '-D_CRT_SECURE_NO_WARNINGS', '-DMINI_XDR_COMPILING_SHARED_LIB', ],
    install : true,
)

librpc_static = static_library(
    meson.project_name(),
    src,
    dependencies : deps,
    c_args : [ '-D_CRT_SECURE_NO_WARNINGS', '-DMINI_XDR_USING_STATIC_LIB_OR_OBJECTS', ],
    install : true,
)

# vi:ts=4:sw=4:sts=4:et:syn=conf
