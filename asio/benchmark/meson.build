cpp_benchmarks_sources = [
  'benchmark_st.cpp',
]

foreach source: cpp_benchmarks_sources
  executable(source.replace('.cpp', ''), source,
    include_directories: includes,
    cpp_args: compile_args,
    dependencies: dependencies,
    # buildtype not work, see https://github.com/mesonbuild/meson/issues/5920
    override_options: {'buildtype': 'release', 'optimization': '3', 'b_ndebug': 'true'})
endforeach
