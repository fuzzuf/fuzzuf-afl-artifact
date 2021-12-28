#define BOOST_TEST_MODULE util.range
#define BOOST_TEST_DYN_LINK
#include <vector>
#include <boost/test/unit_test.hpp>
#include <Utils/SharedRange.hpp>

// shared_rangeしたvectorのiteratorがrandom_access_iteratorの要件を満たす事を確認
BOOST_AUTO_TEST_CASE(SharedRange) {
  using namespace fuzzuf::utils::range::adaptor;
  std::shared_ptr< std::vector< int > > p( new std::vector< int >{ 1, 2, 3, 4, 5 } );
  auto sr1 = p|shared;
  auto sr2 = p|shared;
  auto sr3 = sr2;
  BOOST_CHECK( ( sr1.begin().get() == p->begin() ) );
  BOOST_CHECK( ( sr2.begin().get() == p->begin() ) );
  BOOST_CHECK( ( sr1.end().get() == p->end() ) );
  BOOST_CHECK( ( sr2.end().get() == p->end() ) );
  BOOST_CHECK_EQUAL( sr1.size(), p->size() );
  BOOST_CHECK_EQUAL( sr2.size(), p->size() );
  BOOST_CHECK_EQUAL( sr1.empty(), p->empty() );
  BOOST_CHECK_EQUAL( sr2.empty(), p->empty() );
  p.reset();
  BOOST_CHECK( ( sr1.begin() == sr2.begin() ) );
  BOOST_CHECK( ( sr1.end() == sr2.end() ) );
  BOOST_CHECK( ( ++sr1.begin() == ++sr2.begin() ) );
  BOOST_CHECK( ( --sr1.end() == --sr2.end() ) );
  BOOST_CHECK( ( *sr1.begin() == *sr2.begin() ) );
  BOOST_CHECK( ( sr1.begin()[ 2 ] == sr2.begin()[ 2 ] ) );
  BOOST_CHECK( ( sr1.begin() == sr3.begin() ) );
  BOOST_CHECK( ( sr1.end() == sr3.end() ) );
}

