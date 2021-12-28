#include <fstream>
#include "Exceptions.hpp"

void create_file( const std::string &filename, const std::string &data ) {
  std::ofstream out( filename );
  if( out.fail() )
    throw exceptions::unable_to_create_file(
      std::string( "Unable to create " ) + filename, __FILE__, __LINE__
    );
  out.write( data.c_str(), data.size() );
  if( out.fail() )
    throw exceptions::unable_to_create_file(
      std::string( "Unable to create " ) + filename, __FILE__, __LINE__
    );
}
