#include <cstring>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <Utils/Filesystem.hpp>

namespace fuzzuf::utils {

fs::path which( const fs::path &name ) {
  if( name.is_absolute() ) return name;
  if( name.has_parent_path() ) return name;

  {
    auto iter = std::getenv( "PATH" );
    const auto global_end = std::next( iter, std::strlen( iter ) );
    if( iter != global_end ) {
      do {
        auto local_end = std::find( iter, global_end, ':' );
 
        const auto deterministic = fs::path( iter, local_end ) / name;
        if( fs::exists( deterministic ) )
          return deterministic;
 
        iter = std::find_if(
          local_end,
          global_end,
         	[]( auto c ) {
            return c != ':';
          }
        );
      } while( iter != global_end );
    }
  }

  const size_t default_path_size = confstr( _CS_PATH, nullptr, 0u );
  if( default_path_size ) {
    std::vector< char > default_path( default_path_size + 1u );
    const size_t default_path_size_ = confstr( _CS_PATH, default_path.data(), default_path.size() );
    assert( default_path_size == default_path_size_ );
    
    auto iter = default_path.begin();
    const auto global_end = default_path.end();
    if( iter != global_end ) {
      do {
        auto local_end = std::find( iter, global_end, ':' );
 
        const auto deterministic = fs::path( iter, local_end ) / name;
        if( fs::exists( deterministic ) )
          return deterministic;
 
        iter = std::find_if(
          local_end,
          global_end,
         	[]( auto c ) {
            return c != ':';
          }
        );
      } while( iter != global_end );
    }
  }
  return name;
}

}

