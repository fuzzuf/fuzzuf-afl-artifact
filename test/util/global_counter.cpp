
#define BOOST_TEST_MODULE util.global_counter
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <Utils/Common.hpp>

BOOST_AUTO_TEST_CASE(UtilGlobalCounter) {
  auto x = Util::GlobalCounter();
  auto y = Util::GlobalCounter();
  BOOST_CHECK( x != y );
}