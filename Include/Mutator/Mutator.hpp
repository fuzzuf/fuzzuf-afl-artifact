#pragma once

#include <cassert>
#include <random>

#include "Options.hpp"
#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"
#include "Algorithms/AFL/AFLDictData.hpp"

// 責務：
//  - 本クラスのインスタンスは、指定されたアルゴリズムにしたがい、ある1つのシードからファズを任意回数生成する
//      - 「指定されたアルゴリズム」には、本クラスのメソッドで提供されるプリミティブの単体の他に、それらプリミティブを組み合わせた呼び出しも含まれる
//      - 逆に言うと、この責務は、1つのMutatorインスタンスにより、複数のシードから複数のファズが生成されることがないことを保証する
//      - この責務は、インスタンスが抱える副作用が別のシードを用いたファズ生成に波及することを防ぐためである
//  - 本クラスのインスタンスの生存期間は、シードを与えてインスタンスを生成したときから、最後のファズ生成終了後までとする
//  - ファズはメンバー変数 `outbuf` に一時保存され、`GetBuf()` メソッドにより依存元に与えられる
class Mutator {
protected:
    // NOTE: const参照を持つ時点でMutatorはExecInputよりライフタイムが短くあるべき
    const ExecInput &input;

    u32 len;
    u8 *outbuf;
    u8 *tmpbuf;
    u32 temp_len;
    u8 *splbuf;
    u32 spl_len;

    std::mt19937 mt_engine; // TODO: テスト容易性のために Dependency Injection 可能にする
    int rand_fd;

public:
    static const std::vector<s8>  interesting_8;
    static const std::vector<s16> interesting_16;
    static const std::vector<s32> interesting_32;

    // コピーコンストラクタを廃止
    // コンパイルエラー回避のために `return std::move(mutator)` を `return mutator` と書いたときに、暗黙でコピーが走ることを避ける
    Mutator(const Mutator&) = delete;
    Mutator(Mutator&) = delete;

    // ムーブコンストラクタ
    Mutator(Mutator&&);

    Mutator( const ExecInput& );
    virtual ~Mutator();

    u8 *GetBuf() { return outbuf; }
    u32 GetLen() { return len; }
    virtual u32 ChooseBlockLen(u32);
    u32 OverwriteWithSet(u32 pos, const std::vector<char> &char_set);
    u32 FlipBit(u32 pos, int n);
    u32 FlipByte(u32, int);
    
    void Replace(int pos, u8 *buf, u32 len);

    template<typename T> T   ReadMem(u32 pos);
    template<typename T> u32 Overwrite(u32 pos, T chr);

    template<typename T> u32 AddN(int pos, int val, int be);
    template<typename T> u32 SubN(int pos, int val, int be);
    template<typename T> u32 InterestN(int pos, int idx, int be);

    void Havoc(
            u32 stacking, 
            const std::vector<AFLDictData>& extras, 
            const std::vector<AFLDictData>& a_extras
         );
    void RestoreHavoc(void);

    const ExecInput &GetSource();
    
    bool Splice(const ExecInput &target);
    void RestoreSplice(void);
};

// Since template function is known to easily cause link errors when its definitions are in cpp files, so we define tempalte members here

/* TODO: Implement generator class */

template<typename T>
T Mutator::ReadMem(u32 pos) {
    // to avoid unaligned memory access, we use memcpy
    T ret;
    std::memcpy(&ret, outbuf + pos, sizeof(T));
    return ret;
}

// special case: we don't need to care about alignment on u8
// the definition is in the cpp file
template<>
u8 Mutator::ReadMem<u8>(u32 pos);

template<typename T>
u32 Mutator::Overwrite(u32 pos, T chr) {
    // to avoid unaligned memory access, we use memcpy
    std::memcpy(outbuf + pos, &chr, sizeof(T));
    return 1;
}

// special case: we don't need to care about alignment on u8
// the definition is in the cpp file
template<>
u32 Mutator::Overwrite<u8>(u32 pos, u8 chr);

template<typename T>
u32 Mutator::AddN(int pos, int val, int be) {
    T orig = ReadMem<T>(pos);
    T r;

    const int n = sizeof(T);
    if (n == 1) {
        r = orig + val;
    } else if (n == 2) {
        if (be) r = SWAP16(SWAP16(orig) + val);
        else r = orig + val;
    } else if (n == 4) {
        if (be) r = SWAP32(SWAP32(orig) + val);
        else r = orig + val;
    }

    Overwrite<T>(pos, r);
    return 1;
}

template<typename T>
u32 Mutator::SubN(int pos, int val, int be) {
    T orig = ReadMem<T>(pos);
    T r;

    const int n = sizeof(T);
    if (n == 1) {
        r = orig - val;
    } else if (n == 2) {
        if (be) r = SWAP16(SWAP16(orig) - val);
        else r = orig - val;
    } else if (n == 4) {
        if (be) r = SWAP32(SWAP32(orig) - val);
        else r = orig - val;
    }

    Overwrite<T>(pos, r);

    return 1;
}

template<typename T>
u32 Mutator::InterestN(int pos, int idx, int be) {
    constexpr int n = sizeof(T);
    static_assert( n == 1 || n == 2 || n == 4, "Mutator::InterestN : T has unsupported size" );

    if (n == 1) {
        Overwrite<T>(pos, interesting_8[idx]);
    }
    if (n == 2) {
        if (be) Overwrite<T>(pos, SWAP16(interesting_16[idx]));
        else Overwrite<T>(pos, interesting_16[idx]);
    }
    if (n == 4) {
        if (be) Overwrite<T>(pos, SWAP32(interesting_32[idx]));
        else Overwrite<T>(pos, interesting_32[idx]);
    }

    return 1;
}
