function(qlog_configure_static_analyzers target_name enable_clang_tidy)
  if(NOT enable_clang_tidy)
    return()
  endif()
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)

  if (CLANG_TIDY_EXE)
    set(CLANG_TIDY_COMMAND
      "${CLANG_TIDY_EXE}"
      "--extra-arg-before=--driver-mode=g++"
      "--extra-arg=-Wno-unknown-warning-option"
    )
    set_target_properties(${target_name} PROPERTIES
      CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
    message(STATUS "clang-tidy enabled for ${target_name}")
  endif()
endfunction()
