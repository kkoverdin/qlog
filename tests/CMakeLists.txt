include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
  SYSTEM # ignore GoogleTest warnings
)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(qlog_tests
  test_serializer.cpp
)

target_link_libraries(qlog_tests
  PRIVATE
    qlog
    qlog_warnings
    GTest::gtest_main
)

qlog_configure_static_analyzers(qlog_tests ${QLOG_ENABLE_CLANG_TIDY})

include(GoogleTest)
gtest_discover_tests(qlog_tests)
