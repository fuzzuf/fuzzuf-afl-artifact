#include <cstdint>
#include <string>
#include <cerrno>
#include <memory>
#include <system_error>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/scope_exit.hpp>
#include <Utils/MapFile.hpp>

namespace fuzzuf::utils {

mapped_file_t map_file(
  const std::string &filename,
  int flags,
  bool populate
) {
  int fd = open( filename.c_str(), flags );
  if( fd == -1 )
    throw std::system_error(
      errno,
      std::generic_category(),
      filename
    );
  BOOST_SCOPE_EXIT( &fd ) {
    close( fd );
  } BOOST_SCOPE_EXIT_END

  size_t file_size = 0;
  {
    struct stat stat_;
    if( fstat( fd, &stat_ ) == -1 ) {
      throw std::system_error(
	errno,
	std::generic_category(),
	filename
      );
    }
    file_size = stat_.st_size;
  }
  
  int map_prot = 0;
  int map_flags = 0;
  if( ( flags & O_RDONLY ) == O_RDONLY ) {
    map_prot = PROT_READ;
    map_flags = MAP_PRIVATE;
  }
  else if( ( flags & O_WRONLY ) == O_WRONLY ) {
    map_prot = PROT_WRITE;
    map_flags = MAP_SHARED;
  }
  else if( ( flags & O_RDWR ) == O_RDWR ) {
    map_prot = PROT_READ|PROT_WRITE;
    map_flags = MAP_SHARED;
  }
  
  if( populate )
    map_flags |= MAP_POPULATE;
  if( file_size >= 2u * 1024u* 1024u )
    map_flags |= MAP_HUGETLB;
  
  void *addr = mmap( nullptr, file_size, map_prot, map_flags, fd, 0 );
  if( addr == reinterpret_cast< void* >( std::intptr_t( -1 ) ) ) {
    // カーネルがhugepageをサポートしていない可能性がある為、hugepageを使わずにmmapし直す
    if( map_flags & MAP_HUGETLB ) {
      map_flags ^= MAP_HUGETLB;
      addr = mmap( nullptr, file_size, map_prot, map_flags, fd, 0 );
      if( addr == reinterpret_cast< void* >( std::intptr_t( -1 ) ) ) {
        throw std::system_error(
	  errno,
	  std::generic_category(),
	  filename
	);
      }
    }
    else {
      throw std::system_error(
        errno,
	std::generic_category(),
	filename
      );
    }
  }
  
  uint8_t *addr_ = reinterpret_cast< uint8_t* >( addr );
  std::shared_ptr< uint8_t > addr_sp(
    addr_,
    [file_size]( uint8_t *p ) {
      munmap( reinterpret_cast< void* >( p ), file_size );
    }
  );

  return boost::make_iterator_range(
    range::shared_iterator< uint8_t*, std::shared_ptr< uint8_t > >(
      addr_sp.get(),
      addr_sp
    ),
    range::shared_iterator< uint8_t*, std::shared_ptr< uint8_t > >(
      std::next( addr_sp.get(), file_size ),
      addr_sp
    )
  );
}

}

