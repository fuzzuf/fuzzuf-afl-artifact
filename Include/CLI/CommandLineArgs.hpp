#pragma once
#include <string>
#include <vector>

// Used only for CLI
// NOTE: 生のポインタを扱う構造体です。
//      この構造体のライフタイムがポインタより長くならないように気をつけてください。
// NOTE: パーサーに The Lean Mean C++ Option Parser を使用している関係で、こうなっているという背景もあります
// NOTE: main(argc, argv) 由来のargvをパースしたときは、そんなにライフタイムの心配いらないかも
typedef struct {
    int argc;
    const char** argv;

    std::vector<std::string> Args();
} CommandLineArgs;