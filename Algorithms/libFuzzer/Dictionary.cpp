#include <Utils/AFLDictParser.hpp>
#include <Algorithms/libFuzzer/Dictionary.hpp>

namespace fuzzuf::algorithm::libfuzzer::dictionary {

/**
 * @fn
 * filenameで指定されたAFL辞書をdestにロードする
 * destに最初から要素がある場合、ロードした内容は既にある要素の後ろにinsertされる
 * @brief filenameで指定されたAFL辞書をdestにロードする
 * @param filename ファイル名
 * @param dest ロードした内容の出力先
 * @param eout ファイルのパースに失敗した場合にエラーメッセージが文字列で渡ってくるコールバック
 */
  void load(
    const std::string &filename_,
    static_dictionary_t &dest,
    bool strict,
    const std::function< void( std::string&& ) > &eout
  ) {
    utils::dictionary::load_afl_dictionary( filename_, dest, strict, eout );
  }

/**
 * @fn
 * filenameで指定されたAFL辞書をdestにロードする
 * destに最初から要素がある場合、ロードした内容は既にある要素の後ろにinsertされる
 * @brief filenameで指定されたAFL辞書をdestにロードする
 * @param filename ファイル名
 * @param dest ロードした内容の出力先
 * @param eout ファイルのパースに失敗した場合にエラーメッセージが文字列で渡ってくるコールバック
 */
  void load(
    const std::string &filename_,
    dynamic_dictionary_t &dest,
    bool strict,
    const std::function< void( std::string&& ) > &eout
  ) {
    utils::dictionary::load_afl_dictionary( filename_, dest, strict, eout );
  }

}

