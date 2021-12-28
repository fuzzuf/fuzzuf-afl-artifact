#ifndef __EXCEPTIONS_HPP__
#define __EXCEPTIONS_HPP__
#include <stdexcept>
namespace exceptions {

#define FUZZUF_BASE_EXCEPTION( base, name ) \
  struct name : public base { \
    name () : base ( # name ), file( "unknown" ), line( 0 ) {} \
    name ( \
      const std::string& what_arg, \
      const char *file, \
      int line \
    ) : base ( what_arg ), file( file ), line( line ) {} \
    name ( \
      const char *what_arg, \
      const char *file, \
      int line \
    ) : base ( what_arg ), file( file ), line( line ) {} \
    const char *file; \
    int line; \
  };
#define FUZZUF_INHERIT_EXCEPTION( base, name ) \
  struct name : public base { \
    using base :: base ; \
    name () : base( # name , "unknown", 0 ) {} \
  };
  FUZZUF_BASE_EXCEPTION( std::logic_error, wyvern_logic_error )
  FUZZUF_BASE_EXCEPTION( std::runtime_error, wyvern_runtime_error )
  FUZZUF_INHERIT_EXCEPTION( wyvern_logic_error, used_after_free )
  FUZZUF_INHERIT_EXCEPTION( wyvern_logic_error, not_implemented )
  FUZZUF_INHERIT_EXCEPTION( wyvern_runtime_error, execution_failure )
  FUZZUF_INHERIT_EXCEPTION( wyvern_runtime_error, unable_to_create_file )
  FUZZUF_INHERIT_EXCEPTION( wyvern_runtime_error, invalid_file )
  FUZZUF_INHERIT_EXCEPTION( wyvern_runtime_error, cli_error )
  FUZZUF_INHERIT_EXCEPTION( wyvern_runtime_error, logger_error )
}
#endif

