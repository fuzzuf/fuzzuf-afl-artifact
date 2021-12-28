#pragma once

#include <ostream>

// PUTの実行が終了した理由の種別(AFL互換)
enum class PUTExitReasonType {
    FAULT_NONE,   // 正常終了 
    FAULT_TMOUT,  // 実行時間の制限超過
    FAULT_CRASH,  // SEGVなどによるcrash
    FAULT_ERROR,  // 実行がそもそもできなかったなど
    FAULT_NOINST, // おそらくinstrumentが見つからな>かった場合のエラーだが今使われていない 
    FAULT_NOBITS  // 用途不明
};

// we need to give how to print these enum values, for tests using boost
std::ostream& boost_test_print_type(std::ostream& ostr, PUTExitReasonType const &val);
