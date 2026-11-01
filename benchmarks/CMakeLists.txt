include(FetchContent)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.10.0
  SYSTEM
)

set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

add_executable(qlog_benchmarks
  # some sources
  dummy.cpp
)

target_link_libraries(qlog_benchmarks
  PRIVATE
    qlog
    qlog_warnings
    benchmark::benchmark_main
)

qlog_configure_static_analyzers(qlog_benchmarks ${QLOG_ENABLE_CLANG_TIDY})
