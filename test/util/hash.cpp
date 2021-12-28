#define BOOST_TEST_MODULE util.hash
#define BOOST_TEST_DYN_LINK
#include <array>
#include <iostream>
#include <boost/test/unit_test.hpp>
#include <Utils/Common.hpp>
#include "random_data.hpp"
BOOST_AUTO_TEST_CASE(UtilHash32) {
#ifdef __x86_64__
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data1.data(), random_data1.size(), 0xa5b35705 ) ), 1150639252 );
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data2.data(), random_data2.size(), 0xa5b35705 ) ), 1783865212 );
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data3.data(), random_data3.size(), 0xa5b35705 ) ), 4021282027 );
#else
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data1.data(), random_data1.size(), 0xa5b35705 ) ), 1260228967 );
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data2.data(), random_data2.size(), 0xa5b35705 ) ), 665174276 );
  BOOST_CHECK_EQUAL( ( Util::Hash32( random_data3.data(), random_data3.size(), 0xa5b35705 ) ), 2392848746 );
#endif
}

