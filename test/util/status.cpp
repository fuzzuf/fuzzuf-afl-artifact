#define BOOST_TEST_MODULE util.status
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <Utils/Status.hpp>

enum class different_order_t {
  DISCONNECTED,
  CONFLICT,
  BAD_REQUEST,
  OK,
  UNKNOWN
};

enum class partial_t {
  UNKNOWN,
  OK
};

// status_tからstatus_tへのstrictなキャストが出来る事を確認する
BOOST_AUTO_TEST_CASE(Strict) {
  BOOST_CHECK_EQUAL( fuzzuf::status_cast< fuzzuf::status_t >( fuzzuf::status_t::OK ), fuzzuf::status_t::OK );
}

// status_tから並び順の異なるdifferent_order_tにキャストしても名前通りの変換が行われる事を確認する
// different_order_tからstatus_tへの変換も同様に行える事を確認する
BOOST_AUTO_TEST_CASE(DifferentOrder) {
  BOOST_CHECK( ( ( fuzzuf::status_cast< different_order_t, false >( fuzzuf::status_t::OK ) ) == different_order_t::OK ) );
  BOOST_CHECK_EQUAL( ( fuzzuf::status_cast< fuzzuf::status_t >( different_order_t::BAD_REQUEST ) ), fuzzuf::status_t::BAD_REQUEST );
}

// status_tから要素の足りないpartial_tにキャストしても非strictモードなら正しく変換できる事を確認する
// partial_tからstatus_tへの変換はstrictモードで行える事を確認する
BOOST_AUTO_TEST_CASE(Partial) {
  BOOST_CHECK( ( ( fuzzuf::status_cast< partial_t, false >( fuzzuf::status_t::OK ) ) == partial_t::OK ) );
  BOOST_CHECK( ( ( fuzzuf::status_cast< partial_t, false >( fuzzuf::status_t::BAD_REQUEST ) ) == partial_t::UNKNOWN ) );
  BOOST_CHECK_EQUAL( ( fuzzuf::status_cast< fuzzuf::status_t >( partial_t::OK ) ), fuzzuf::status_t::OK );
}

// status_tから文字列に変換できる事を確認する
BOOST_AUTO_TEST_CASE(ToString) {
  BOOST_CHECK_EQUAL( fuzzuf::status_cast< std::string >( fuzzuf::status_t::OK ), std::string( "OK" ) );
}

// 文字列からstatus_tに変換できる事を確認する
// 文字列の場合strictモードでも存在しない値がUNKNOWNになる事を確認する
BOOST_AUTO_TEST_CASE(FromString) {
  BOOST_CHECK_EQUAL( fuzzuf::status_cast< fuzzuf::status_t >( "OK" ), fuzzuf::status_t::OK );
  BOOST_CHECK_EQUAL( fuzzuf::status_cast< fuzzuf::status_t >( "HOGE" ), fuzzuf::status_t::UNKNOWN );
}

