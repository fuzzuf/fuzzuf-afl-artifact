#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <boost/scope_exit.hpp>

#include "config.h"
#include "Utils/Filesystem.hpp"
#include "Python/PythonFuzzer.hpp"
#include "ExecInput/ExecInput.hpp"

int main(int argc, char **argv) {
  srand(time(NULL));
  std::string root_dir_template( "/tmp/fuzzuf_test.XXXXXX" );
  const auto raw_dirname = mkdtemp( root_dir_template.data() );
  if( !raw_dirname ) throw -1;

  auto root_dir = fs::path( raw_dirname );
  BOOST_SCOPE_EXIT( &root_dir ) {
    fs::remove_all( root_dir );
  } BOOST_SCOPE_EXIT_END
  auto input_dir = root_dir / "input";
  auto output_dir = root_dir / "output";
  fs::create_directory( input_dir );

  std::vector<std::string> argvv;
  if (argc < 3 || (strcmp(argv[1], "0") != 0 && strcmp(argv[1], "1") != 0)) {
    printf("[ usage ] %s (0, 1: non-fork-server-mode, fork-server-mode) <argv>\n", argv[0]);
    printf("ex) %s 1 /Bench/freetype/ftfuzzer @@\n", argv[0]);
    return 1;
  }
  for (int i = 2; i < argc; i++) {
    argvv.emplace_back(argv[i]);
  } 
  auto fuzzer = PythonFuzzer(
    argvv, input_dir.native(), output_dir.native(),
    100, 10000,
    argv[1][0] == '1', // forksrv
    true, true // need_afl_cov, need_bb_cov
  );
  fuzzer.SuppressLog();
  std::vector<u8> my_buf;
  auto LEN = 10000;
  for (int i = 0; i < LEN; i++) {
    my_buf.push_back(rand() % 256);
  }
  for (int i = 0; i < 20000; i++) {
    if (i % 10000 == 0 && i) {
      printf("finished %d add_seed\n", i);
    }
    auto id = fuzzer.AddSeed(LEN, my_buf);
    if (id != ExecInput::INVALID_INPUT_ID) {
      fuzzer.RemoveSeed(id);
    }
  }
}
