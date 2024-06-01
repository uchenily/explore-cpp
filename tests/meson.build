all_tests_sources = [
  'test_debug.cpp',
  'test_print.cpp',
  'test_timer.cpp',
  'test_async_timer.cpp',
  'test_binding_arguments.cpp',
  'test_member_function.cpp',
  'test_multithread.cpp',
  'test_tcp_server.cpp',
  'test_tcp_client.cpp',
  'test_async_tcp_server.cpp',
  'test_udp_client.cpp',
  'test_udp_server.cpp',
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
