add_executable( ok ok.cpp )
set_target_properties( ok PROPERTIES COMPILE_FLAGS "" )
add_executable( fail fail.cpp )
set_target_properties( fail PROPERTIES COMPILE_FLAGS "" )
add_executable( never_exit never_exit.cpp )
set_target_properties( never_exit PROPERTIES COMPILE_FLAGS "" )
add_executable( abort abort.cpp )
set_target_properties( abort PROPERTIES COMPILE_FLAGS "" )
add_executable( segmentation_fault segmentation_fault.cpp )
set_target_properties( segmentation_fault PROPERTIES COMPILE_FLAGS "" )
add_executable( illegal_instruction illegal_instruction.cpp )
set_target_properties( illegal_instruction PROPERTIES COMPILE_FLAGS "" )

subdirs( non_fork_server_mode )

add_executable( test-pintool-run pintool_run.cpp )
target_link_libraries(
  test-pintool-run
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-pintool-run
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-pintool-run
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-pintool-run
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "pintool_executor.pintool_context.run" COMMAND test-pintool-run )
subdirs( intel_pin )

