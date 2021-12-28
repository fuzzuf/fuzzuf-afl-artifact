add_executable( test-instrument-cov-bb cov_bb.cpp )
target_link_libraries(
  test-instrument-cov-bb
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-instrument-cov-bb
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-instrument-cov-bb
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-instrument-cov-bb
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "instrument.cov.bb" COMMAND test-instrument-cov-bb )

add_executable( test-instrument-cov-afl cov_afl.cpp )
target_link_libraries(
  test-instrument-cov-afl
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-instrument-cov-afl
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-instrument-cov-afl
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-instrument-cov-afl
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "instrument.cov.afl" COMMAND test-instrument-cov-afl )
