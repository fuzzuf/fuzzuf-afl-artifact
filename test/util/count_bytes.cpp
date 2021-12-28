#define BOOST_TEST_MODULE util.count_bytes
#define BOOST_TEST_DYN_LINK
#include <array>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Utils/Common.hpp>
#include "random_data.hpp"
BOOST_AUTO_TEST_CASE(UtilCountBytes) {
  BOOST_CHECK_EQUAL( ( Util::CountBytes( random_data1.data(), random_data1.size() ) ), 65281 );
  BOOST_CHECK_EQUAL( ( Util::CountBytes( random_data2.data(), random_data2.size() ) ), 479 );
  BOOST_CHECK_EQUAL( ( Util::CountBytes( random_data3.data(), random_data3.size() ) ), 112 );
}

