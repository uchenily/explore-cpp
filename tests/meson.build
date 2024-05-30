all_tests_sources = [
  'test_debug.cpp',
  'test_print.cpp',
  'test_timer.cpp',
  'test_async_timer.cpp',
]

foreach source: all_tests_sources
  target_name = source.replace('.cpp', '')
  exe = executable(target_name, source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies)

  test_name = target_name.replace('test_', '')
  test(test_name, exe)
endforeach
