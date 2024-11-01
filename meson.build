project(
    'pkgconf-mangle',
    'cpp',
    meson_version : '>=1.0',
    version : '1.0',
    default_options: [
        'cpp_std=c++17',
        'default_library=static',
    ]
)
executable(meson.project_name(), 'main.cc', link_args: ['-static'])
