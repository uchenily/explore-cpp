all_tests_sources = [
  'example.cpp',
  'compute_and_write_csv_example.cpp',
  'acero_register_example.cpp',
  'dataset_example.cpp',
]

foreach source: all_tests_sources
  target_name = source.replace('.cpp', '')
  exe = executable(target_name, source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies)

  test(target_name, exe)
endforeach
