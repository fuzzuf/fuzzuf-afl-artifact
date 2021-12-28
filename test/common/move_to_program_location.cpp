#include <cassert>
#include "config.h"
#ifdef HAS_CXX_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif
#include <boost/test/unit_test.hpp>

// shell scriptでいうところのcd $(dirname $0)するための関数。
// テストコード中で使用するとどのcwdからテストのバイナリを実行しても動作するようになるはず
void MoveToProgramLocation(void) {
    static bool has_moved = false;

    if (has_moved) return;
    has_moved = true;

    char *argv0 = boost::unit_test::framework::master_test_suite().argv[0];
    assert(argv0);

#ifdef HAS_CXX_STD_FILESYSTEM
    namespace fs = std::filesystem;
#else
    namespace fs = boost::filesystem;
#endif

    fs::current_path(fs::path(argv0).parent_path());
}
