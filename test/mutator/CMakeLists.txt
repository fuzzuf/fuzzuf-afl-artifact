add_executable( test-mutator mutator.cpp )
target_link_libraries(
        test-mutator
        test-common
        fuzzuf
        ${FUZZUF_LIBRARIES}
        Boost::unit_test_framework
)
target_include_directories(
        test-mutator
        PRIVATE
        ${FUZZUF_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/test/common
)
set_target_properties(
  test-mutator
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  test-mutator
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
add_test( NAME "mutator.mutator" COMMAND test-mutator )
