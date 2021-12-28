#pragma once

#include <vector>
#include <string>
#include <functional>
#include "Utils/Common.hpp"

namespace fuzzuf::algorithm::afl::dictionary {
struct AFLDictData {
    using word_t = std::vector<u8>;

    AFLDictData() {
    }

    AFLDictData( const word_t &v ) :
      data( v ) {
    }

    AFLDictData( const word_t &v, u32 h ) :
      data( v ), hit_cnt( h ) {
    }

    const std::vector<u8> get() const {
      return data;
    }

    std::vector<u8> data;
    u32 hit_cnt = 0u;
};

void load(
  const std::string &filename_,
  std::vector< AFLDictData > &dest,
  bool strict,
  const std::function< void( std::string&& ) > &eout
);

}

using AFLDictData = fuzzuf::algorithm::afl::dictionary::AFLDictData;

