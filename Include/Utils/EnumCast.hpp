#ifndef FUZZUF_INCLUDE_UTILS_ENUM_CAST_HPP
#define FUZZUF_INCLUDE_UTILS_ENUM_CAST_HPP
#include <type_traits>
#include <string>
#include <Utils/RemoveCVR.hpp>
#include <Utils/CheckCapability.hpp>

/*
 * enumを値ではなく名前が一致するものにキャストする関数を作るためのマクロ群
 * 詳しい使い方はUtils/Status.hppで実際に使用している箇所を参照
 */

#define FUZZUF_ENUM_CAST_CHECK( ns, name ) \
  namespace detail::enum_cast:: ns { \
    FUZZUF_CHECK_CAPABILITY( has_ ## name , T :: name ) \
    template< typename T > \
    auto get_ ## name () -> std::enable_if_t< \
      std::is_enum_v< T >, \
      decltype( T :: name ) \
    > { \
      return T :: name ; \
    } \
    template< typename T > \
    auto get_ ## name () -> std::enable_if_t< \
      std::is_convertible_v< T, std::string >, \
      std::string \
    > { \
      return # name ; \
    } \
  }

#define FUZZUF_ENUM_CAST_BEGIN( ns, fallback ) \
  template< typename T, bool strict = true, typename U > \
  auto ns ## _cast( U value ) \
  -> std::enable_if_t< \
    std::is_convertible_v< U, std::string > || \
    std::is_enum_v< U >, \
    decltype( detail::enum_cast:: ns ::get_ ## fallback < T >() ) \
  > { \
    using namespace detail::enum_cast:: ns ; \
    static const auto fb = get_ ## fallback < T >();

#define FUZZUF_ENUM_CAST_END \
    return fb; \
  } \
  
    
#define FUZZUF_ENUM_CAST_CONVERT( name ) \
  if constexpr ( \
    std::is_convertible_v< U, std::string > || \
    has_ ## name ## _v< U > \
  ) { \
    if( \
      get_ ## name < U >() == \
      static_cast< decltype( get_ ## name < U >() ) >( value ) \
    ) { \
      if constexpr ( \
        std::is_same_v< \
          fuzzuf::utils::type_traits::remove_cvr_t< T >, \
	  std::string \
	> || \
        has_ ## name ## _v< T > \
      ) { \
        return get_ ## name < T >(); \
      } \
      else if constexpr ( strict ) { \
        static_assert( \
          !strict, \
	  "enum_cast is rejected due to lack of correspondig value for " \
          # name \
	); \
      } \
    } \
  }

#endif

