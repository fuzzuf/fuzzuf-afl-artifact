#define BOOST_TEST_MODULE util.count_bits
#define BOOST_TEST_DYN_LINK
#include <array>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Utils/Common.hpp>
#include "random_data.hpp"
BOOST_AUTO_TEST_CASE(UtilCountBits) {
  BOOST_CHECK_EQUAL( ( Util::CountBits( random_data1.data(), random_data1.size() ) ), 262309 );
  BOOST_CHECK_EQUAL( ( Util::CountBits( random_data2.data(), random_data2.size() ) ), 1892 );
  BOOST_CHECK_EQUAL( ( Util::CountBits( random_data3.data(), random_data3.size() ) ), 448 );
}

