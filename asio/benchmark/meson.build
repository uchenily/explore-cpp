cpp_benchmarks_sources = [
  'benchmark_st.cpp',
  'benchmark_mt.cpp',
]

liburing_dep = dependency('liburing')

foreach source: cpp_benchmarks_sources
  executable(source.replace('.cpp', ''), source,
    include_directories: includes,
    cpp_args: compile_args + ['-DASIO_HAS_IO_URING=ON', '-DASIO_DISABLE_EPOLL=ON'], # -DASIO_ENABLE_HANDLER_TRACKING
    dependencies: dependencies + liburing_dep,
    # buildtype not work, see https://github.com/mesonbuild/meson/issues/5920
    override_options: {'buildtype': 'release', 'optimization': '3', 'b_ndebug': 'true'})
endforeach
