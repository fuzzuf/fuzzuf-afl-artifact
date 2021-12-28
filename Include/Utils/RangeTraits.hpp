#ifndef FUZZUF_INCLUDE_UTILS_RANGE_TRAITS_HPP
#define FUZZUF_INCLUDE_UTILS_RANGE_TRAITS_HPP
#include <utility>
#include <Utils/RemoveCVR.hpp>
#include <Utils/CheckCapability.hpp>
namespace fuzzuf::utils::range {

// メンバ関数begin()がある
FUZZUF_CHECK_CAPABILITY(
  has_begin,
  std::declval< T& >().begin()
)

// メンバ関数end()がある
FUZZUF_CHECK_CAPABILITY(
  has_end,
  std::declval< T& >().end()
)

// rangeである
template< typename T >
struct is_range : public std::integral_constant< bool,
  has_begin_v< T > && // Tにはbegin()がある
  has_end_v< T > // Tにはend()がある
> {};
template< typename T >
constexpr bool is_range_v = is_range< T >::value;

// Rangeのvalue_typeを取得する
// C++20のstd::ranges::range_value互換
template< typename Range, typename Enable = void >
struct range_value {
};
template< typename Range >
struct range_value< Range, std::enable_if_t<
  is_range_v< Range > // Rangeはrangeである
> > {
  using type = type_traits::remove_cvr_t< decltype( *std::declval< Range >().begin() ) >;
};
template< typename Range >
using range_value_t = typename range_value< Range >::type;
// Rangeのイテレータを取得する
// C++20のstd::ranges::iterator互換
template< typename Range, typename Enable = void >
struct range_iterator {
};
template< typename Range >
struct range_iterator< Range, std::enable_if_t<
  is_range_v< Range > // Rangeはrangeである
> > {
  using type = type_traits::remove_cvr_t< decltype( std::declval< Range >().begin() ) >;
};
template< typename Range >
using range_iterator_t = typename range_iterator< Range >::type;
}
#endif

