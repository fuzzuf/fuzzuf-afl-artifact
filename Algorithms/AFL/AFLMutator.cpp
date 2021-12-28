#include "Algorithms/AFL/AFLMutator.hpp"

#include <cassert>
#include <random>

#include "Utils/Common.hpp"
#include "Mutator/Mutator.hpp"
#include "ExecInput/ExecInput.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"

/* TODO: Implement generator class */

AFLMutator::AFLMutator( const ExecInput &input, const AFLState& state ) 
    : Mutator(input), state(state) {}

AFLMutator::~AFLMutator() {}

AFLMutator::AFLMutator(AFLMutator&& src)
    : Mutator(std::move(src)), state(src.state) {}

/* Helper to choose random block len for block operations in fuzz_one().
   Doesn't return zero, provided that max_len is > 0. */

u32 AFLMutator::ChooseBlockLen(u32 limit) {
    u32 min_value, max_value;
    u32 rlim = std::min(state.queue_cycle, 3ULL);

    if (!state.run_over10m) rlim = 1;

    // just an alias of afl::util::UR
    auto UR = [this](u32 limit) {
        return afl::util::UR(limit, state.rand_fd);
    };

    switch (UR(rlim)) {
    case 0:  min_value = 1;
             max_value = AFLOption::HAVOC_BLK_SMALL;
             break;

    case 1:  min_value = AFLOption::HAVOC_BLK_SMALL;
             max_value = AFLOption::HAVOC_BLK_MEDIUM;
             break;
    default:
        if (UR(10)) {
            min_value = AFLOption::HAVOC_BLK_MEDIUM;
            max_value = AFLOption::HAVOC_BLK_LARGE;
        } else {
            min_value = AFLOption::HAVOC_BLK_LARGE;
            max_value = AFLOption::HAVOC_BLK_XL;
        }
    }

    if (min_value >= limit) min_value = 1;

    return min_value + UR(std::min(max_value, limit) - min_value + 1);
}

// FIXME: these three functions easily get buggy and
// difficult to check with eyes. We should add tests.

/* Helper function to see if a particular change (xor_val = old ^ new) could
   be a product of deterministic bit flips with the lengths and stepovers
   attempted by afl-fuzz. This is used to avoid dupes in some of the
   deterministic fuzzing operations that follow bit flips. We also
   return 1 if xor_val is zero, which implies that the old and attempted new
   values are identical and the exec would be a waste of time. */

bool AFLMutator::CouldBeBitflip(u32 val) {
    u32 sh = 0;
    if (!val) return true;
    /* Shift left until first bit set. */
    while (!(val & 1)) { sh++; val >>= 1; }
    /* 1-, 2-, and 4-bit patterns are OK anywhere. */
    if (val == 1 || val == 3 || val == 15) return true;
    /* 8-, 16-, and 32-bit patterns are OK only if shift factor is
        divisible by 8, since that's the stepover for these ops. */

    if (sh & 7) return false;

    if (val == 0xff || val == 0xffff || val == 0xffffffff)
        return true;
    return false;
}

/* Helper function to see if a particular value is reachable through
   arithmetic operations. Used for similar purposes. */

bool AFLMutator::CouldBeArith(u32 old_val, u32 new_val, u8 blen) {    
    u32 i, ov = 0, nv = 0, diffs = 0;
    if (old_val == new_val) return true;

    /* See if one-byte adjustments to any byte could produce this result. */

    for (i = 0; i < blen; i++) {
        u8 a = old_val >> (8 * i),
           b = new_val >> (8 * i);

        if (a != b) { diffs++; ov = a; nv = b; }
    }
    /* If only one byte differs and the values are within range, return 1. */
    if (diffs == 1) {
        if ((u8)(ov - nv) <= AFLOption::ARITH_MAX ||
            (u8)(nv - ov) <= AFLOption::ARITH_MAX) return true;
    }

    if (blen == 1) return false;

    /* See if two-byte adjustments to any byte would produce this result. */

    diffs = 0;

    for (i = 0; i < blen / 2; i++) {
        u16 a = old_val >> (16 * i),
            b = new_val >> (16 * i);
        if (a != b) { diffs++; ov = a; nv = b; }
    }
    /* If only one word differs and the values are within range, return 1. */
    if (diffs == 1) {
        if ((u16)(ov - nv) <= AFLOption::ARITH_MAX ||
            (u16)(nv - ov) <= AFLOption::ARITH_MAX) return true;

        ov = SWAP16(ov); nv = SWAP16(nv);

        if ((u16)(ov - nv) <= AFLOption::ARITH_MAX ||
            (u16)(nv - ov) <= AFLOption::ARITH_MAX) return true;
    }
    /* Finally, let's do the same thing for dwords. */
    if (blen == 4) {
        if ((u32)(old_val - new_val) <= AFLOption::ARITH_MAX ||
            (u32)(new_val - old_val) <= AFLOption::ARITH_MAX) return true;

        new_val = SWAP32(new_val);
        old_val = SWAP32(old_val);

        if ((u32)(old_val - new_val) <= AFLOption::ARITH_MAX ||
            (u32)(new_val - old_val) <= AFLOption::ARITH_MAX) return true;
    }
    return false;
    
}
/* Last but not least, a similar helper to see if insertion of an 
   interesting integer is redundant given the insertions done for
   shorter blen. The last param (check_le) is set if the caller
   already executed LE insertion for current blen and wants to see
   if BE variant passed in new_val is unique. */

// We implicitly assume blen <= 4
bool AFLMutator::CouldBeInterest(u32 old_val, u32 new_val, u8 blen, u8 check_le) {
    u32 i, j;

    if (old_val == new_val) return true;

    /* See if one-byte insertions from interesting_8 over old_val could
        produce new_val. */
    for (i = 0; i < blen; i++) {
        for (j = 0; j < interesting_8.size(); j++) {
            // NOTE: "u32(interesting_8[j])" and "u32(u8(interesting_8[j]))"
            // are different(don't use the former).
            // For example, if interesting_8[j] == -1, they are FFFFFFFF and FF
            u8 ubyte = interesting_8[j];
            u32 tval = (old_val & ~u32(0xfful << (i * 8))) |
                    (u32(ubyte) << (i * 8));

            if (new_val == tval) return true;
        }
    }

    /* Bail out unless we're also asked to examine two-byte LE insertions
        as a preparation for BE attempts. */

    if (blen == 2 && !check_le) return false;

    /* See if two-byte insertions over old_val could give us new_val. */

    for (i = 0; i < blen - 1u; i++) {
        for (j = 0; j < interesting_16.size(); j++) {
            u16 uword = interesting_16[j];
            u32 tval = (old_val & ~u32(0xfffful << (i * 8))) |
                    (u32(uword) << (i * 8));

            if (new_val == tval) return true;

            /* Continue here only if blen > 2. */

            if (blen > 2) {
                tval = (old_val & ~u32(0xfffful << (i * 8))) |
                        (u32(SWAP16(uword)) << (i * 8));
                if (new_val == tval) return true;
            }
        }
    }

    if (blen == 4 && check_le) {
    /* See if four-byte insertions could produce the same result
       (LE only). */
       for (j = 0; j < interesting_32.size(); j++)
            if (new_val == (u32)interesting_32[j]) return true;
    }

    return false;
}
