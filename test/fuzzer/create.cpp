#define BOOST_TEST_MODULE fuzzer.create
#define BOOST_TEST_DYN_LINK
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Python/PythonFuzzer.hpp>
#include "config.h"
#ifdef HAS_CXX_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <boost/scope_exit.hpp>
#include <create_file.hpp>
#include <move_to_program_location.hpp>

// to test both fork server mode and non fork server mode, we specify forksrv via an argument
static void FuzzerCreate(bool forksrv) {
  // cd $(dirname $0)
  MoveToProgramLocation();

  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );
#ifdef HAS_CXX_STD_FILESYSTEM
  namespace fs = std::filesystem;
#else
  namespace fs = boost::filesystem;
#endif
  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  auto input_path = input_dir / "0";
  create_file( input_path.string(), "Hello, World!" );
  auto fuzzer = PythonFuzzer(
    { "../put_binaries/command_wrapper", "/bin/cat" },
    input_dir.native(), output_dir.native(),
    1000, 10000,
    forksrv,
    true, true // need_afl_cov, need_bb_cov
  );
  fuzzer.SuppressLog();
}

BOOST_AUTO_TEST_CASE(FuzzerCreateNonForkMode) {
  // 出力を入れないと、複数テストケースある場合、切れ目がどこかが分からず、どっちが失敗しているか分からない事に気づいた
  std::cout << "[*] FuzzerCreateNonForkMode started\n";
  FuzzerCreate(false);
  std::cout << "[*] FuzzerCreateNonForkMode ended\n";
}

BOOST_AUTO_TEST_CASE(FuzzerCreateForkMode) {
  std::cout << "[*] FuzzerCreateForkMode started\n";
  FuzzerCreate(true);
  std::cout << "[*] FuzzerCreateForkMode ended\n";
}

