#define BOOST_TEST_MODULE afl.loop
#define BOOST_TEST_DYN_LINK
#include <random>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Algorithms/AFL/AFLFuzzer.hpp>
#include <Utils/Filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <move_to_program_location.hpp>

// to test both fork server mode and non fork server mode, we specify forksrv via an argument
static void AFLLoop(bool forksrv) {
  // cd $(dirname $0)
  MoveToProgramLocation();

  // Create root directory
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;
  BOOST_CHECK( raw_dirname != nullptr );

  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    // Executed on this test exit
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END
  
  // Create input file
  auto put_dir = fs::path( "../../put_binaries/libjpeg" );
  auto input_dir = put_dir / "seeds";
  auto output_dir = root_dir / "output";
  
  // Create fuzzer instance
  auto fuzzer = AFLFuzzer(
    { "../../put_binaries/libjpeg/libjpeg_turbo_fuzzer", "@@" },
    input_dir.native(), output_dir.native(),
    AFLOption::EXEC_TIMEOUT, AFLOption::MEM_LIMIT,
    forksrv
  );

  for (int i=0; i<1; i++) {
      std::cout << "the " << i << "-th iteration starts" << std::endl;
      fuzzer.OneLoop();
  }
}

BOOST_AUTO_TEST_CASE(AFLLoopNonForkMode) {
  // 出力を入れないと、複数テストケースある場合、切れ目がどこかが分からず、どっちが失敗しているか分からない事に気づいた
  std::cout << "[*] AFLLoopNonForkMode started\n";
  AFLLoop(false);
  std::cout << "[*] AFLLoopNonForkMode ended\n";
}

BOOST_AUTO_TEST_CASE(AFLLoopForkMode) {
  std::cout << "[*] AFLLoopForkMode started\n";
  AFLLoop(true);
  std::cout << "[*] AFLLoopForkMode ended\n";
}
