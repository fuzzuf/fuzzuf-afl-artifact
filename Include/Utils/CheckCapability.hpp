#ifndef FUZZUF_INCLUDE_UTILS_CHECK_CAPABILITY_HPP
#define FUZZUF_INCLUDE_UTILS_CHECK_CAPABILITY_HPP
#include <type_traits>
// 与えられた型Tがexprを評価出来る場合にtrueになるtype_traits <name>を生成する
// 使用例: Tがメンバ変数hogeを持っている場合だけ呼べる関数fugaを作る
// FUZZUF_CHECK_CAPABILITY( has_hoge, std::declval< T >().hoge )
// template< typename T >auto fuga( T ) -> std::enable_if_t< has_hoge_v< T > > { ... } 
#define FUZZUF_CHECK_CAPABILITY( name, expr ) \
template< typename T, typename Enable = void > \
struct name : public std::false_type {}; \
template< typename T > \
struct name < T, std::void_t< \
  decltype( expr ) \
> > : public std::true_type {}; \
template< typename T > \
constexpr bool name ## _v = name < T >::value;
#endif

