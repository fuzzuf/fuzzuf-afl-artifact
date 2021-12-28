#include "CLI/FuzzerBuilderRegister.hpp"
#include "Logger/Logger.hpp"
#include "Exceptions.hpp"
#include <utility>


// マップの初期化前にマップへの挿入を防ぐための設計
// ref. https://stackoverflow.com/a/3746390
BuilderMap& FuzzerBuilderRegister::GetBuilderMap() {
    static BuilderMap builder_map;
    return builder_map;
}

// 各ファザーの Builder をリンク前に登録するためのAPI
FuzzerBuilderRegister::FuzzerBuilderRegister(std::string name, FuzzerBuilder builder) {
    GetBuilderMap().insert(std::make_pair(name, builder));
}

FuzzerBuilder FuzzerBuilderRegister::Get(std::string name) {
    auto res = GetBuilderMap().find(name);
    if (res == GetBuilderMap().end()) {
        throw exceptions::cli_error(
            "Failed to get builder of fuzzer \"" + name + "\"",
            __FILE__, __LINE__);
    }
    DEBUG("[*] Starting fuzzer \"%s\"", name.c_str());
    return res->second;
}