project('learn_oatpp', 'cpp',
  version: '0.0.1',
  default_options: [
    'buildtype=debug',
    'default_library=shared',
    'cpp_std=c++20',
    'warning_level=3'],
  meson_version: '>=1.2.0')

compile_args = []

includes = include_directories('.')

oatpp_dep = dependency('oatpp')
dependencies = []
dependencies += oatpp_dep

all_tests_sources = [
  '00_start.cpp',
  '01_request_handler.cpp',
  '02_json_response.cpp',
]

foreach source: all_tests_sources
  target_name = source.replace('.cpp', '')
  exe = executable(target_name, source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies)

  test(target_name, exe)
endforeach

subdir('03_well_structed_project')
