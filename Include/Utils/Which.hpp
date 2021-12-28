#ifndef FUZZUF_INCLUDE_UTILS_WHICH_HPP
#define FUZZUF_INCLUDE_UTILS_WHICH_HPP
#include <Utils/Filesystem.hpp>
namespace fuzzuf::utils {

// PATHに基づいて実行可能バイナリの場所を特定する
// nameが絶対パスの場合、nameがそのまま返る
//   例: /hoge/fuga
// nameが親ディレクトリの名前を1つ以上持つ場合、nameがそのまま返る
//   例: ../fuga
// 環境変数PATHの要素/nameが存在する場合、環境変数PATHの要素/nameが返る
// 候補が複数ある場合PATHに先に書かれた要素が優先される
//   例: bash
// それ以外の場合、nameがそのまま返る
fs::path which( const fs::path &name );

}
#endif

