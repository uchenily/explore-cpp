project('learn_arrow', 'cpp',
  version: '0.0.1',
  default_options: [
    'buildtype=debug',
    'default_library=shared',
    'cpp_std=c++20',
    'warning_level=3'],
  meson_version: '>=1.2.0')

compile_args = []

includes = include_directories('.')

# $ cd /usr/lib/pkgconfig/
# $ ls arrow*
# arrow-acero.pc    arrow-csv.pc      arrow-filesystem.pc  arrow-flight-sql.pc  arrow-orc.pc  arrow-substrait.pc
# arrow-compute.pc  arrow-dataset.pc  arrow-flight.pc      arrow-json.pc        arrow.pc      arrow-tensorflow.pc
arrow_dep = dependency('arrow')
arrowdataset_dep = dependency('arrow-dataset')
dependencies = []
dependencies += arrow_dep
dependencies += arrowdataset_dep

#subdir('tests')
#subdir('benchmark')

all_tests_sources = [
  '00_basic_data_strcuture.cpp',
  '01_file_io.cpp',
  '02_arrow_compute.cpp',
  '03_arrow_dataset.cpp',
]

foreach source: all_tests_sources
  target_name = source.replace('.cpp', '')
  exe = executable(target_name, source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies)

  test(target_name, exe)
endforeach

subdir('examples')
subdir('async_task')
