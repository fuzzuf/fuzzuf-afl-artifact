#define BOOST_TEST_MODULE exec_input.set
#define BOOST_TEST_DYN_LINK
#include <set>
#include <memory>
#include <boost/test/unit_test.hpp>

#include "ExecInput/ExecInput.hpp"
#include "ExecInput/ExecInputSet.hpp"

BOOST_AUTO_TEST_CASE(ExecInputSetTest) {
  ExecInputSet input_set;

  int N = 5;
  for (int i = 0; i < N; i++) {
    input_set.CreateOnMemory((const u8*)"test", 4);
  }
  BOOST_CHECK_EQUAL(input_set.size(), N);

  auto v = input_set.get_ids();
  BOOST_CHECK_EQUAL(v.size(), N);
  std::set<u64> v_set(v.begin(), v.end());
  BOOST_CHECK_EQUAL(v_set.size(), N);

  u64 last_id = 0;
  for (int i = 0; i < N; i++) {
    input_set.erase(v[i]);
    last_id = v[i];
  }
  BOOST_CHECK_EQUAL(input_set.size(), 0);

  auto id = input_set.CreateOnMemory((const u8*)"test", 4)->GetID();
  BOOST_CHECK(input_set.get_ref(id) != std::nullopt);
  BOOST_CHECK(input_set.get_ref(last_id) == std::nullopt);
}
