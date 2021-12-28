###
add_executable( test-cli-parser parser.cpp )
target_link_libraries(
  test-cli-parser
  fuzzuf-cli-lib
  Boost::unit_test_framework
)
target_include_directories(
  test-cli-parser
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/test/common
)
add_test( NAME "cli.parser" COMMAND test-cli-parser )