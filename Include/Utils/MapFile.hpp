#ifndef FUZZUF_INCLUDE_UTILS_MAP_FILE_HPP
#define FUZZUF_INCLUDE_UTILS_MAP_FILE_HPP
#include <cstdint>
#include <memory>
#include <boost/range/iterator_range.hpp>
#include <Utils/SharedRange.hpp>
namespace fuzzuf::utils {
using mapped_file_t = boost::iterator_range< range::shared_iterator< uint8_t*, std::shared_ptr< uint8_t > > >;

/**
 * @fn
 * filenameで指定されたファイルをmmapして、mmapした領域をshared_rangeで返す
 * @brief filenameで指定されたファイルをmmapして、mmapした領域をshared_rangeで返す
 * @param filename ファイル名
 * @param flags ファイルをopen(2)する際に渡すフラグ
 * @param populate trueの場合mmapと同時に全てのページをメモリに乗せる事を要求する
 * @return mmapされた領域のrange
 */
mapped_file_t map_file( const std::string &filename, int flags, bool populate );

}
#endif

