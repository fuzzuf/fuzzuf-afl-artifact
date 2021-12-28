#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <Algorithms/libFuzzer/Dictionary.hpp>

// AFL辞書形式の内容をmsgpackの配列に変換する

int main( int argc, char *argv[] ) {

  namespace po = boost::program_options;

  po::options_description desc( "Options" );
  std::string in_file( "input.dict" );
  std::string out_file( "output.mp" );
  bool strict = false;
  desc.add_options()
    ( "help,h", "show this message" )
    ( "input,i", po::value< std::string >( &in_file ), "input filename" )
    ( "output,o", po::value< std::string >( &out_file ), "output filename" )
    ( "strict,s", po::bool_switch( &strict ), "strict mode" );
  po::variables_map vm;
  po::store( po::parse_command_line( argc, argv, desc ), vm );
  po::notify( vm );
  if( vm.count( "help" ) ) {
    std::cout << desc << std::endl;
    exit( 0 );
  }

  fuzzuf::algorithm::libfuzzer::dictionary::static_dictionary_t dict;
  load(
    in_file,
    dict,
    strict,
    []( std::string &&m ) {
      std::cerr << m << std::endl;
    }
  );

  std::vector< uint8_t > temp;

  if( dict.size() < ( size_t( 1u ) << 4 ) ) {
    temp.push_back( 0x90u | dict.size() );
  }
  else if( dict.size() < ( size_t( 1u ) << 16 ) ) {
    temp.push_back( 0xdcu );
    temp.push_back( ( dict.size() >> 8 ) & 0xFFu );
    temp.push_back(   dict.size()        & 0xFFu );
  }
  else if( dict.size() < ( size_t( 1u ) << 32 ) ) {
    temp.push_back( 0xddu );
    temp.push_back( ( dict.size() >> 24 ) & 0xFFu );
    temp.push_back( ( dict.size() >> 16 ) & 0xFFu );
    temp.push_back( ( dict.size() >>  8 ) & 0xFFu );
    temp.push_back(   dict.size()         & 0xFFu );
  }
  else {
    std::cerr << "too many entries" << std::endl;
    exit( 1 );
  }

  for( const auto &v: dict ) {
    const size_t size = v.get().size();
    if( size < ( size_t( 1u ) << 8 ) ) {
      temp.push_back( 0xc4 );
      temp.push_back( size );
    }
    else if( v.get().size() < ( size_t( 1u ) << 16 ) ) {
      temp.push_back( 0xc5 );
      temp.push_back( ( size >> 8 ) & 0xFFu );
      temp.push_back(   size        & 0xFFu );
    }
    else if( v.get().size() < ( size_t( 1u ) << 32 ) ) {
      temp.push_back( 0xc6 );
      temp.push_back( ( size >> 24 ) & 0xFFu );
      temp.push_back( ( size >> 16 ) & 0xFFu );
      temp.push_back( ( size >>  8 ) & 0xFFu );
      temp.push_back(   size         & 0xFFu );
    }
    else {
      std::cerr << "too long entry" << std::endl;
      exit( 1 );
    }
    temp.insert( temp.end(), v.get().begin(), v.get().end() );
  }
  
  std::ofstream file( out_file );
  file.write( reinterpret_cast< const char* >( temp.data() ), temp.size() );
}

