#include "Algorithms/AFL/AFLFuzzer.hpp"
#include "CLI/FuzzerBuilderRegister.hpp"
#include "CLI/Fuzzers/AFL/BuildAFLFuzzerFromArgs.hpp"

// 以下をglobal変数として宣言し、かつこれがobject fileとしてリンクされると
// main関数より先にbuilder_map.insertされる。
// 逆に、「この環境だとAFLはビルドできない」みたいなのがあるなら、object fileをそもそも生成しなければ、誤ってAFLが登録されることがなくて嬉しい
static FuzzerBuilderRegister global_afl_register("afl", BuildAFLFuzzerFromArgs<Fuzzer, AFLFuzzer>);