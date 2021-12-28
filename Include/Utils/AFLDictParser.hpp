#ifndef FUZZUF_INCLUDE_UTILS_AFL_DICT_PARSER_HPP
#define FUZZUF_INCLUDE_UTILS_AFL_DICT_PARSER_HPP
#include <cstdint>
#include <functional>
#include <string>
#include <system_error>
#include <fcntl.h>
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix.hpp>
#include <Exceptions.hpp>
#include <Utils/RemoveCVR.hpp>
#include <Utils/MapFile.hpp>
#include <Utils/RangeTraits.hpp>
#include <Utils/CheckCapability.hpp>
#include <Utils/Filesystem.hpp>
/*
 * AFLの辞書をロードできるようにした
 * ロード先の型は
 * T型のsequentialコンテナ。ただしT::word_tが定義されていて、word_tはcharと互換のある型を要素とするsequentialコンテナでTはword_tを引数とするコンストラクタを持つ
 * を満たす必要がある
 */
namespace fuzzuf::utils::dictionary {
  FUZZUF_CHECK_CAPABILITY( has_get, std::declval< T >().get() )

  // 辞書型Tの要素のバイト列を持つコンテナの型を返す
  template< typename T, typename Enable = void >
  struct dictionary_word {
  };

  // 条件: Tのrange_valueがメンバ関数get()を持っている
  template< typename T >
  struct dictionary_word< T, std::enable_if_t<
    has_get_v< range::range_value_t< T > >
  > > {
    using type = typename range::range_value_t< T >::word_t;
  };

  template< typename T >
  using dictionary_word_t = typename dictionary_word< T >::type;

  // Dest型の辞書に対してWord型の新しいエントリを追加する
  // 条件: Tのrange_valueがメンバ関数get()を持っている
  template< typename Dest, typename Word >
  auto emplace_word(
    Dest &dest,
    Word &&word
  ) -> std::enable_if_t<
    has_get_v< range::range_value_t< Dest > >
  > {
    dest.emplace_back( range::range_value_t< Dest >{ std::move( word ) } );
  }

  template< typename Iterator, typename Dict >
  class afl_dict_rule : public boost::spirit::qi::grammar< Iterator, Dict() > {
    using dictionary_t = Dict;
    using word_t = dictionary_word_t< dictionary_t >;
    public:

    afl_dict_rule(
      unsigned int filter,
      bool strict,
      const std::function< void( std::string&& ) > &eout
    ) : afl_dict_rule::base_type( root ) {
      namespace qi = boost::spirit::qi;
      namespace phx = boost::phoenix;
      
      escape =
        ( qi::lit( "\\\\" )[ qi::_val = '\\' ] )|
	( qi::lit( "\\\"" )[ qi::_val = '"' ] )|
        (( "\\x" >> hex8_p )[ qi::_val = qi::_1 ] );

      if( strict ) {
        escaped_text = qi::as_string[ *( ( ( qi::standard::blank | qi::standard::graph ) - '"' - '\\' ) | escape ) ];
	name = qi::as_string[ +( qi::standard::graph - '@' - '=' - '"' - '#' ) ];
	comment = ( '#' >> *( qi::standard::blank | qi::standard::graph ))[ qi::_pass = true ];
      }
      else {
        escaped_text = qi::as_string[ *( ( qi::char_ - '"' - '\\' ) | escape ) ];
	name = qi::as_string[ +( qi::standard::char_ - '@' - '=' - '"' - '#' - qi::standard::space ) ];
	comment = ( '#' >> *( qi::standard::char_ - qi::eol ))[ qi::_pass = true ];
      }

      quoted_text = qi::omit[ qi::lit( '"' ) ] >> escaped_text >> qi::omit[ qi::lit( '"' ) ];
      root = qi::skip( qi::standard::blank )[
        (comment[ qi::_pass = true ])|
        ((
	  quoted_text >> qi::omit[ *qi::standard::blank >> -comment ]
	)[ qi::_pass = phx::bind(
          &afl_dict_rule::without_level,
	  qi::_val,
	  std::string( "(no name)" ),
	  qi::_1,
	  eout
	) ])|
        ((
          name >> '@' >> qi::uint_ >> '=' >> quoted_text >> qi::omit[ *qi::standard::blank >> -comment ]
        )[ qi::_pass = phx::bind(
          &afl_dict_rule::with_level,
	  qi::_val,
	  qi::_1,
	  qi::_2,
	  filter,
	  qi::_3,
	  eout
	) ])|
        ((
          name >> '=' >> quoted_text >> qi::omit[ *qi::standard::blank >> -comment ]
        )[ qi::_pass = phx::bind(
          &afl_dict_rule::without_level,
	  qi::_val,
	  qi::_1,
	  qi::_2,
	  eout
	) ])|
        (( *qi::standard::blank )[ qi::_pass = true ])
      ] % qi::eol >> qi::omit[ *qi::standard::space ];
    }

    private:

    static bool with_level(
      dictionary_t &dest,
      const std::string &name,
      unsigned int level,
      unsigned int threshold,
      const std::string &text,
      const std::function< void( std::string&& ) > &eout
    ) {
      if( level < threshold ) return true;

      return without_level( dest, name, text, eout );
    }

    static bool without_level(
      dictionary_t &dest,
      const std::string &name,
      const std::string &text,
      const std::function< void( std::string&& ) > &eout
    ) {
      if( dest.size() == dest.max_size() ) {
      	eout( "Too many entries." );
	return false;
      }

      static const word_t word;
      if( word.max_size() < text.size() ) {
        eout( name + " is too long." );
        return false;
      }
      
      emplace_word( dest, word_t( text.begin(), text.end() ) );
      
      return true;
    }

    boost::spirit::qi::uint_parser< uint8_t, 16, 2, 2 > hex8_p;
    boost::spirit::qi::rule< Iterator, char() > escape;
    boost::spirit::qi::rule< Iterator, int() > comment;
    boost::spirit::qi::rule< Iterator, std::string() > name;
    boost::spirit::qi::rule< Iterator, std::string() > escaped_text;
    boost::spirit::qi::rule< Iterator, std::string() > quoted_text;
    boost::spirit::qi::rule< Iterator, dictionary_t() > root;
  };

  template< typename T >
  auto load_afl_dictionary(
    const std::string &filename_,
    T &dest,
    bool strict,
   const std::function< void( std::string&& ) > &eout
  ) -> std::void_t< dictionary_word_t< T > > {
    
    namespace qi = boost::spirit::qi;
    
    unsigned int level = 0u;
    std::string filename;
    
    {
      fs::path path( filename_ );
      const std::string leaf = path.filename().string();
      boost::fusion::vector< std::string, unsigned int > parsed;
      auto iter = leaf.begin();
      const auto end = leaf.end();
      if(
        !qi::parse(
	  iter,
	  end,
	  qi::as_string[ *( qi::char_ - '@' )] >> '@' >> qi::uint_,
	  parsed
	) ||
	iter != end
      )
        filename = filename_;
      else {
        filename = (
          path.remove_filename() /
	  fs::path( boost::fusion::at_c< 0 >( parsed ) )
	).string();
      	level = boost::fusion::at_c< 1 >( parsed );
      }
    }
    
    auto mapped_file = utils::map_file( filename, O_RDONLY, true );
    const afl_dict_rule< uint8_t*, T > rule( level, strict, eout );
    auto iter = mapped_file.begin().get();
    const auto end = mapped_file.end().get();
    T temp;
    if( !qi::parse( iter, end, rule, temp ) || iter != end )
      throw exceptions::invalid_file(
        "invalid dictionary file",
	__FILE__,
	__LINE__
      );

    dest.insert( dest.end(), temp.begin(), temp.end() );
  }
}
#endif

