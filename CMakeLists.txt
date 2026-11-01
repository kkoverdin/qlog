cmake_minimum_required(VERSION 3.25)
project(qlog LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(QLOG_BUILD_TESTS "Build unit tests" ON)
option(QLOG_BUILD_BENCHMARKS "Build benchmarks" ON)
option(QLOG_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(QLOG_ENABLE_CLANG_TIDY "Enable static analyzers" ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/StaticAnalyzers.cmake)

# warnings
add_library(qlog_warnings INTERFACE)
qlog_set_compiler_warnings(qlog_warnings ${QLOG_WARNINGS_AS_ERRORS})

add_library(qlog STATIC
  src/dummy.cpp
)

target_include_directories(qlog
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(qlog
  PRIVATE
    qlog_warnings
)

qlog_configure_static_analyzers(qlog ${QLOG_ENABLE_CLANG_TIDY})

if(QLOG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(QLOG_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
