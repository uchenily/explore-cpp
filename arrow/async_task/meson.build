all_tests_sources = [
  '00_future.cpp',
  '01_threadpool.cpp',
  '02_scheduler.cpp',
  '03_allfinished_callback.cpp',
  '04_taskgroup.cpp',
  '05_taskscheduler.cpp',
  '06_asyncscheduler.cpp',
]

foreach source: all_tests_sources
  target_name = source.replace('.cpp', '')
  exe = executable(target_name, source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies)

  test(target_name, exe)
endforeach
