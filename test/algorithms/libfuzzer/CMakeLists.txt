add_executable( test-algorithms-libfuzzer-dictionary dictionary.cpp )
target_link_libraries(
  test-algorithms-libfuzzer-dictionary
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-algorithms-libfuzzer-dictionary
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-algorithms-libfuzzer-dictionary
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-algorithms-libfuzzer-dictionary
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "algorithms.libfuzzer.dictionary" COMMAND test-algorithms-libfuzzer-dictionary )
