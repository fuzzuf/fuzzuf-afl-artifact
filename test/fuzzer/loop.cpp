// fuzzer_handle.fuzzing_loop に改名
#define BOOST_TEST_MODULE fuzzer.loop
#define BOOST_TEST_DYN_LINK
#include <random>
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

// Fuzzerインスタンスを生成し、一定回数mutation & PUTのexecuteを行っても少なくともクラッシュやUAFを引き起こさないことを確認するテスト

// to test both fork server mode and non fork server mode, we specify forksrv via an argument
static void FuzzerLoop(bool forksrv) {
  // cd $(dirname $0)
  MoveToProgramLocation();

  // Create root directory
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
    // Executed on this test exit
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END
  
  // Create input file
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  BOOST_CHECK_EQUAL( fs::create_directory( input_dir ), true );
  auto input_path = input_dir / "0";
  create_file( input_path.string(), "Hello, World!" );
  
  // Create fuzzer instance
  auto fuzzer = PythonFuzzer(
    { "../put_binaries/command_wrapper", "/bin/cat" },
    input_dir.native(), output_dir.native(),
    1000, 10000,
    forksrv,
    true, true // need_afl_cov, need_bb_cov
  );

  // Configurate fuzzer
  fuzzer.SuppressLog();
  
  std::mt19937 rand;
  std::uniform_int_distribution<> len_dist( 0, 2 );
  for( int i = 0; i != 100; ++i ) {
    auto seeds = fuzzer.GetSeedIDs();
    // ExecInputSet should never be null
    BOOST_CHECK(seeds.size() > 0);
    // just select one seed roughtly 
    fuzzer.SelectSeed(seeds[0]);

    auto len = 1 << (len_dist( rand ));
    std::uniform_int_distribution<> pos_dist( 0, 12 * 8 - len );
    fuzzer.FlipBit(pos_dist(rand), len);
  }
}

BOOST_AUTO_TEST_CASE(FuzzerLoopNonForkMode) {
  // 出力を入れないと、複数テストケースある場合、切れ目がどこかが分からず、どっちが失敗しているか分からない事に気づいた
  std::cout << "[*] FuzzerLoopNonForkMode started\n";
  FuzzerLoop(false);
  std::cout << "[*] FuzzerLoopNonForkMode ended\n";
}

BOOST_AUTO_TEST_CASE(FuzzerLoopForkMode) {
  std::cout << "[*] FuzzerLoopForkMode started\n";
  FuzzerLoop(true);
  std::cout << "[*] FuzzerLoopForkMode ended\n";
}
