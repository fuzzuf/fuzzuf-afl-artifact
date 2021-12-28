#ifndef FUZZUF_INCLUDE_UTILS_IS_EXECUTABLE_HPP
#define FUZZUF_INCLUDE_UTILS_IS_EXECUTABLE_HPP
#include <Utils/Filesystem.hpp>
namespace fuzzuf::utils {

// PATHで指定されたファイルを実行する権限がある場合trueが返る
// 実行権限がついていても指定されたファイルが通常ファイルでない場合falseが返る
bool is_executable( const fs::path &name );

}
#endif

