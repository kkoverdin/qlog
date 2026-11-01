function(qlog_set_compiler_warnings target_name warnings_as_errors)
  set(QLOG_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wcast-align
    -Wconversion
    -Wfloat-conversion
    -Wsign-conversion
  )

  if (warnings_as_errors)
    list(APPEND QLOG_WARNINGS -Werror)
  endif()

  target_compile_options(${target_name} INTERFACE ${QLOG_WARNINGS})
endfunction()
