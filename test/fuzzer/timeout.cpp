#define BOOST_TEST_MODULE fuzzer.timeout
#define BOOST_TEST_DYN_LINK
#include <random>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <boost/scope_exit.hpp>

#include "config.h"
#include "Utils/Filesystem.hpp"
#include "Python/PythonFuzzer.hpp"
#include "ExecInput/ExecInput.hpp"
#include "create_file.hpp"
#include "move_to_program_location.hpp"

// Fuzzerインスタンスが適切にPUTへ実行時間制限を課せているか確認

// to test both fork server mode and non fork server mode, we specify forksrv via an argument
static void FuzzerTestTimeout(bool forksrv) {
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
  // AddSeedを使うので初期シードは必要ない
  //auto input_path = input_dir / "0";
  //create_file( input_path.string(), "Hello, World!" );
  
  // Create fuzzer instance
  // sleep 2を制限時間1秒で実行
  auto fuzzer = PythonFuzzer(
    { "../put_binaries/command_wrapper", "/bin/sleep", "2" },
    input_dir.native(), output_dir.native(),
    1000, 10000,
    forksrv,
    true, true // need_afl_cov, need_bb_cov
  );

  // Configurate fuzzer
  fuzzer.SuppressLog();

  auto id = fuzzer.AddSeed(1, {1}); // どんなシードを与えようがタイムアウトして然るべき
  BOOST_CHECK_EQUAL(id, ExecInput::INVALID_INPUT_ID); // FIXME: タイムアウトとクラッシュを識別する手立てがない
}

BOOST_AUTO_TEST_CASE(FuzzerTestTimeoutNonForkMode) {
  // 出力を入れないと、複数テストケースある場合、切れ目がどこかが分からず、どっちが失敗しているか分からない事に気づいた
  std::cout << "[*] FuzzerTestTimeoutNonForkMode started\n";
  FuzzerTestTimeout(false);
  std::cout << "[*] FuzzerTestTimeoutNonForkMode ended\n";
}

BOOST_AUTO_TEST_CASE(FuzzerTestTimeoutForkMode) {
  std::cout << "[*] FuzzerTestTimeoutForkMode started\n";
  FuzzerTestTimeout(true);
  std::cout << "[*] FuzzerTestTimeoutForkMode ended\n";
}
