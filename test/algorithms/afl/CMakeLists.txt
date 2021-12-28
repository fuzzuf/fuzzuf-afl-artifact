add_executable( test-algorithms-afl-dictionary dictionary.cpp )
target_link_libraries(
  test-algorithms-afl-dictionary
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-algorithms-afl-dictionary
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-algorithms-afl-dictionary
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-algorithms-afl-dictionary
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "algorithms.afl.dictionary" COMMAND test-algorithms-afl-dictionary )

add_executable( test-afl-loop loop.cpp )
target_link_libraries(
  test-afl-loop
  test-common
  fuzzuf
  ${FUZZUF_LIBRARIES}
  Boost::unit_test_framework
)
target_include_directories(
  test-afl-loop
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-afl-loop
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-afl-loop
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
if( ENABLE_HEAVY_TEST )
add_test( NAME "afl.loop" COMMAND test-afl-loop )
endif()

if( BEHAVE_DETERMINISTIC )
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/AFL/
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/AFL/
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
    ${CMAKE_CURRENT_BINARY_DIR}/run.sh
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/diff.sh
    ${CMAKE_CURRENT_BINARY_DIR}/diff.sh
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/original-afl.patch
    ${CMAKE_CURRENT_BINARY_DIR}/original-afl.patch
    COPYONLY
)
endif()
