#ifndef FUZZUF_INCLUDE_UTILS_REMOVE_CVR_HPP
#define FUZZUF_INCLUDE_UTILS_REMOVE_CVR_HPP
#include <type_traits>
namespace fuzzuf::utils::type_traits {
  // 与えられた型Tをremove_referenceしてからremove_cvする
  // C++20のremove_cvref互換
  template< typename T >
  using remove_cvr_t = std::remove_cv_t< std::remove_reference_t< T > >;
}
#endif

