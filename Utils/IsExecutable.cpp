#include <unistd.h>
#include <Utils/Filesystem.hpp>

namespace fuzzuf::utils {

bool is_executable( const fs::path &name ) {
  if( !fs::exists( name ) ) return false;
  if( !fs::is_regular_file( name ) ) return false;
  return access( name.c_str(), X_OK ) == 0;
}

}
