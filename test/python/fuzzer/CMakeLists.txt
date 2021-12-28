add_executable( test-pythonfuzzer-reset reset.cpp )
target_link_libraries(
  test-pythonfuzzer-reset
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-pythonfuzzer-reset
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-pythonfuzzer-reset
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-pythonfuzzer-reset
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "pythonfuzzer.reset" COMMAND test-pythonfuzzer-reset )
