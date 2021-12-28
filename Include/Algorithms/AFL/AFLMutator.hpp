#pragma once

#include <cassert>
#include <random>

#include "Mutator/Mutator.hpp"
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"
#include "Algorithms/AFL/AFLState.hpp"

// MutatorにAFL独自の枝刈りを足したもの
// NOTE: AFLMutator is not "fully" inherited from Mutator
// Mutator's member functions are not virtual. 
// Hence you must not treat this as Mutator instance

class AFLMutator : public Mutator {
protected:
    u32 val_for_restore;
    u32 pos_for_restore;

    const AFLState &state;

public:
    // コピーコンストラクタを廃止
    // コンパイルエラー回避のために `return std::move(mutator)` を `return mutator` と書いたときに、暗黙でコピーが走ることを避ける
    AFLMutator(const AFLMutator&) = delete;
    AFLMutator(AFLMutator&) = delete;

    // ムーブコンストラクタ
    AFLMutator(AFLMutator&&);

    AFLMutator( const ExecInput&, const AFLState& );
    ~AFLMutator();

    u32 ChooseBlockLen(u32);

    template<typename T> u32 AddN(int pos, int val, int be);
    template<typename T> u32 SubN(int pos, int val, int be);
    template<typename T> u32 InterestN(int pos, int idx, int be);
    template<typename T> void RestoreOverwrite(void);

    bool CouldBeBitflip(u32);
    bool CouldBeArith(u32, u32, u8);
    bool CouldBeInterest(u32, u32, u8, u8);
};

// Since template function is known to easily cause link errors when its definitions are in cpp files, so we define tempalte members here

/* TODO: Implement generator class */

// 戻り値: 実際に書き換えたかどうか(0, 1)
template<typename T>
u32 AFLMutator::AddN(int pos, int val, int be) {
    T orig = ReadMem<T>(pos);
    
    T r;
    bool need;

    constexpr int n = sizeof(T);

    if constexpr (n == 1) {
        if (be) need = false;
        else {
            need = true;
            r = orig + val;
        }
    } else if constexpr (n == 2) {
        if (be) {
            need = (orig >> 8) + val > 0xff;
            r = SWAP16(SWAP16(orig) + val);
        } else {
            need = (orig & 0xff) + val > 0xff;
            r = orig + val;
        }
    } else if constexpr (n == 4) {
        if (be) {
            need = (SWAP32(orig) & 0xffff) + val > 0xffff;
            r = SWAP32(SWAP32(orig) + val);
        } else {
            need = (orig & 0xffff) + val > 0xffff;
            r = orig + val;
        }
    }

    if (need && !CouldBeBitflip(orig ^ r)) { 
        Mutator::Overwrite<T>(pos, r);
        val_for_restore = orig;
        pos_for_restore = pos;
        return 1;
    }

    return 0;
}

// 戻り値: 実際に書き換えたかどうか(0, 1)
template<typename T>
u32 AFLMutator::SubN(int pos, int val, int be) {
    T orig = ReadMem<T>(pos);
    
    T r;
    bool need;

    constexpr int n = sizeof(T);        

    if constexpr (n == 1) {
        if (be) need = false;
        else {
            need = true;
            r = orig - val;
        }
    } else if constexpr (n == 2) {
        if (be) {
            need = int(orig >> 8) < val;
            r = SWAP16(SWAP16(orig) - val);
        } else {
            need = int(orig & 0xff) < val;
            r = orig - val;
        }
    } else if constexpr (n == 4) {
        if (be) {
            need = int(SWAP32(orig) & 0xffff) < val;
            r = SWAP32(SWAP32(orig) - val);
        } else {
            need = int(orig & 0xffff) < val;
            r = orig - val;
        }
    }

    if (need && !CouldBeBitflip(orig ^ r)) { 
        Mutator::Overwrite<T>(pos, r);
        val_for_restore = orig;
        pos_for_restore = pos;
        return 1;
    }

    return 0;
}

// 戻り値: 実際に書き換えたかどうか(0, 1)
template<typename T>
u32 AFLMutator::InterestN(int pos, int idx, int be) {
    T orig = ReadMem<T>(pos);
    T r;

    bool need;

    constexpr int n = sizeof(T);

    if constexpr (n == 1) {
        r = (u8)interesting_8[idx];

        if (be) need = false;
        else {
            need = !CouldBeBitflip(orig ^ r) 
                && !CouldBeArith(orig, r, 1);
        }
    } else if constexpr (n == 2) {
        if (be) {
            r = SWAP16(interesting_16[idx]);
            need = (u16)interesting_16[idx] != r;
        } else {
            r = interesting_16[idx];
            need = true;
        }

        need &= !CouldBeBitflip(orig ^ r);
        need &= !CouldBeArith(orig, r, 2);
        need &= !CouldBeInterest(orig, r, 2, be);
    } else if constexpr (n == 4) {
        if (be) {
            r = SWAP32(interesting_32[idx]);
            need = (u32)interesting_32[idx] != r;
        } else {
            r = interesting_32[idx];
            need = true;
        }

        need &= !CouldBeBitflip(orig ^ r);
        need &= !CouldBeArith(orig, r, 4);
        need &= !CouldBeInterest(orig, r, 4, be);
    }

    if (need) {
        Mutator::Overwrite<T>(pos, r);
        val_for_restore = orig;
        pos_for_restore = pos;
        return 1;
    }

    return 0;
}

// For restoration of AddN, SubN, InterestN
template<typename T>
void AFLMutator::RestoreOverwrite(void) {
    Mutator::Overwrite<T>(pos_for_restore, val_for_restore);
}
