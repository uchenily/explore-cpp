project('learn_asio', 'cpp',
  version: '0.0.1',
  default_options: [
    'buildtype=debug',
    'default_library=shared',
    'cpp_std=c++20',
    'warning_level=3'],
  meson_version: '>=1.2.0')

compile_args = []

includes = include_directories('learn_asio')

asio_dep = dependency('asio')
dependencies = []
dependencies += asio_dep

subdir('tests')
subdir('benchmark')
