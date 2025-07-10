project(
    'pkgconf-mangle',
    'cpp',
    meson_version : '>=1.0',
    version : '1.0',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
        'buildtype=debugoptimized',
        'default_library=static',
        'b_lto=true',
    ]
)

cxx = meson.get_compiler('cpp')
extra_cpp_args = cxx.get_supported_arguments([
  '-ftrivial-auto-var-init=pattern', '-Wtrivial-auto-var-init',
  '-Wconversion',
])

executable(meson.project_name(), 'main.cc', cpp_args: extra_cpp_args, link_args: ['-static'])
