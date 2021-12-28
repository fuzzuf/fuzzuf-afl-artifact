#ifndef FUZZUF_INCLUDE_ALGORITHM_LIBFUZZER_DICTIONARY_HPP
#define FUZZUF_INCLUDE_ALGORITHM_LIBFUZZER_DICTIONARY_HPP
#include <cstddef>
#include <vector>
#include <string>
#include <functional>
#include <iostream>
#include <algorithm>
#include <boost/container/static_vector.hpp>
#include <nlohmann/json.hpp>
namespace fuzzuf::algorithm::libfuzzer::dictionary {
  template< typename Word >
  class basic_dictionary_entry_t {
  public:
    using word_t = Word;

    basic_dictionary_entry_t() {
    }

    basic_dictionary_entry_t( const word_t &w ) : word( w ) {
    }

    basic_dictionary_entry_t( const word_t &w, size_t ph ) : word( w ), position_hint( ph ) {
    }

    const word_t &get() const {
      return word;
    }

    bool has_position_hint() const {
      return position_hint != std::numeric_limits< size_t >::max();
    }

    size_t get_position_hint() const {
      return position_hint;
    }

    void increment_use_count() {
      ++use_count;
    }

    void increment_success_count() {
      ++success_count;
    }

    size_t get_use_count() const {
      return use_count;
    }

    size_t get_success_count() const {
      return success_count;
    }

    bool operator==( const basic_dictionary_entry_t &r ) const {
      return
	std::equal( word.begin(), word.end(), r.word.begin(), r.word.end() ) &&
        position_hint == r.position_hint &&
	use_count == r.use_count &&
	success_count == r.success_count;
    }

    bool operator!=( const basic_dictionary_entry_t &r ) const {
      return
	!std::equal( word.begin(), word.end(), r.word.begin(), r.word.end() ) ||
        position_hint != r.position_hint ||
	use_count != r.use_count ||
	success_count != r.success_count;
    }

    nlohmann::json to_json() const {
      auto root = nlohmann::json::object();
      root[ "word" ] = nlohmann::json::array();
      for( const auto &v: word ) root[ "word" ].push_back( v );
      root[ "position_hint" ] = position_hint;
      root[ "use_count" ] = use_count;
      root[ "success_count" ] = success_count;
      return root;
    }

  private:
    word_t word;
    size_t position_hint = std::numeric_limits< size_t >::max();
    size_t use_count = 0u;
    size_t success_count = 0u;
  };

  template< typename Word >
  size_t get_position_hint( const basic_dictionary_entry_t< Word > &v ) {
    return v.get_position_hint();
  }

  template< typename Traits, typename T >
  std::basic_ostream< char, Traits > &operator<<( std::basic_ostream<char, Traits> &l, const basic_dictionary_entry_t< T > &r ) {
   l << r.to_json().dump();
   return l;
  }

  // static_dictionaryは辞書の要素を持つコンテナと個々の要素のバイト列を持つコンテナがstatic_vectorになっている
  // これはオリジナルのlibFuzzerの辞書型と正確に同じ振る舞いをする為の型で、libFuzzerの辞書と同じサイズの上限を持つ
  using static_dictionary_entry_t = basic_dictionary_entry_t< boost::container::static_vector< uint8_t, 64u > >;
  using static_dictionary_t = boost::container::static_vector< static_dictionary_entry_t, 1u << 14 >;
  
  template< typename Traits >
  std::basic_ostream< char, Traits > &operator<<( std::basic_ostream<char, Traits> &l, const static_dictionary_t &r ) {
   auto root = nlohmann::json::array();
   for( auto &v: r ) root.push_back( v.to_json() );
   l << root.dump();
   return l;
  }
  
  // dynamic_dictionaryは辞書の要素を持つコンテナと個々の要素のバイト列を持つコンテナがvectorになっている
  // オリジナルのlibFuzzerの辞書型と異なり挿入時にメモリ確保を伴うが、より多くの要素、大きなバイト列を保持する事ができる
  using dynamic_dictionary_entry_t = basic_dictionary_entry_t< std::vector< uint8_t > >;
  using dynamic_dictionary_t = std::vector< dynamic_dictionary_entry_t >;
  
  template< typename Traits >
  std::basic_ostream< char, Traits > &operator<<( std::basic_ostream<char, Traits> &l, const dynamic_dictionary_t &r ) {
   auto root = nlohmann::json::array();
   for( auto &v: r ) root.push_back( v.to_json() );
   l << root.dump();
   return l;
  }
  
  void load(
    const std::string &filename,
    static_dictionary_t&,
    bool strict,
    const std::function< void( std::string&& ) > &eout
  );
  
  void load(
    const std::string &filename,
    dynamic_dictionary_t&,
    bool strict,
    const std::function< void( std::string&& ) > &eout
  );

}
#endif

