#define BOOST_TEST_MODULE util.which
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <Utils/Which.hpp>
#include <Utils/Filesystem.hpp>

// whichが見つけてきたteeのパスにコマンドが存在することを確認する
BOOST_AUTO_TEST_CASE(Which) {
  BOOST_CHECK( fs::exists( fuzzuf::utils::which( fs::path( "tee" ) ) ) );
}


