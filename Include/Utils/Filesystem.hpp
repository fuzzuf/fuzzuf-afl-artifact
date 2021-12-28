#pragma once

#include "config.h"
#ifdef HAS_CXX_STD_FILESYSTEM
#include <filesystem>
#else
#include <boost/filesystem.hpp>
#endif

#ifdef HAS_CXX_STD_FILESYSTEM
  namespace fs = std::filesystem;
#else
  namespace fs = boost::filesystem;
#endif
