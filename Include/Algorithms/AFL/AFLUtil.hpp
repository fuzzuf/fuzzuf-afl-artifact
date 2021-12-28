#pragma once

#include <string>
#include "Utils/Common.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/PUTExitReasonType.hpp"
#include "Algorithms/AFL/AFLState.hpp"
#include "Algorithms/AFL/AFLTestcase.hpp"

namespace afl {
namespace util {
    
    template<class UInt>
    UInt EFF_APOS(UInt p);

    template<class UInt>
    UInt EFF_REM(UInt x);

    template<class UInt>
    UInt EFF_ALEN(UInt l);

    template<class UInt>
    UInt EFF_SPAN_ALEN(UInt p, UInt l);

    u32 UR(u32 limit, int rand_fd);

    std::string DescribeInteger(u64 val);
    std::string DescribeFloat(double val);
    std::string DescribeMemorySize(u64 val);
    std::string DescribeTimeDelta(u64 cur_ms, u64 event_ms);

    void MarkAsDetDone(const AFLState& state, AFLTestcase &testcase);
    void MarkAsVariable(const AFLState& state, AFLTestcase &testcase);
    void MarkAsRedundant(const AFLState& state, AFLTestcase &testcase, bool val);

    template<class UInt> 
    u8 DoHasNewBits(const u8 *trace_bits, u8 *virgin_map, u32 map_size, AFLState &state);

    u8 HasNewBits(const u8 *trace_bits, u8 *virgin_map, u32 map_size, AFLState &state);

    template<class UInt>
    void SimplifyTrace(UInt *mem, u32 map_size);

    void UpdateBitmapScoreWithRawTrace(
        AFLTestcase &testcase,
        AFLState &state,
        const u8 *trace_bits,
        u32 map_size
    );

    void UpdateBitmapScore(
        AFLTestcase &testcase,
        AFLState &state,
        const InplaceMemoryFeedback &inp_feed
    );

    PUTExitReasonType CalibrateCaseWithFeedDestroyed(
        AFLTestcase &testcase,
        const u8 *buf,
        u32 len,
        AFLState &state,
        InplaceMemoryFeedback &inp_feed,
        ExitStatusFeedback &exit_status,
        u32 handicap,
        bool from_queue
    );
 
std::shared_ptr<AFLTestcase> AddToQueue(
    AFLState &state,
    const std::string &fn,
    const u8 *buf,
    u32 len,
    bool passed_det
);

} // namespace util
} // namespace afl

template<class UInt>
UInt afl::util::EFF_APOS(UInt p) {
    return p >> AFLOption::EFF_MAP_SCALE2;
}

template<class UInt>
UInt afl::util::EFF_REM(UInt x) {
    return x & ((UInt(1) << AFLOption::EFF_MAP_SCALE2) - 1);
}

template<class UInt>
UInt afl::util::EFF_ALEN(UInt l) {
    return EFF_APOS(l) + !!EFF_REM(l);
}

template<class UInt>
UInt afl::util::EFF_SPAN_ALEN(UInt p, UInt l) {
    return EFF_APOS(p + l - 1) - EFF_APOS(p) + 1;
}

/* Check if the current execution path brings anything new to the table.
   Update virgin bits to reflect the finds. Returns 1 if the only change is
   the hit-count for a particular tuple; 2 if there are new tuples seen.
   Updates the map, so subsequent calls will always return 0.
   This function is called after every exec() on a fairly large buffer, so
   it needs to be fast. We do this in 32-bit and 64-bit flavors. */

template<class UInt>
u8 afl::util::DoHasNewBits(const u8 *trace_bits, u8 *virgin_map, u32 map_size, AFLState &state) {
    // we assume the word size is the same as sizeof(size_t)
    static_assert(sizeof(UInt) == sizeof(size_t));

    constexpr int width = sizeof(UInt);

    int wlog;
    if constexpr (width == 4) {
        wlog = 2;
    } else if constexpr (width == 8) {
        wlog = 3;
    }

    UInt* virgin  = (UInt*)virgin_map;
    const UInt* current = (const UInt*)trace_bits;

    u32 i = map_size >> wlog;

    u8 ret = 0;
    while (i--) {
        /* Optimize for (*current & *virgin) == 0 - i.e., no bits in current bitmap
           that have not been already cleared from the virgin map - since this will
           almost always be the case. */

        if (unlikely(*current) && unlikely(*current & *virgin)) {
            if (likely(ret < 2)) {
                u8* cur = (u8*)current;
                u8* vir = (u8*)virgin;

                /* Looks like we have not found any new bytes yet; see if any non-zero
                   bytes in current[] are pristine in virgin[]. */

                for (int j=0; j < width; j++) {
                    if (cur[j] && vir[j] == 0xff) {
                        ret = 2;
                        break;
                    }
                }
                if (ret != 2) ret = 1;
            }

            *virgin &= ~*current;
        }

        current++;
        virgin++;
    }

    if (ret && virgin_map == &state.virgin_bits[0]) state.bitmap_changed = 1;

    return ret;
}

/* Destructively simplify trace by eliminating hit count information
   and replacing it with 0x80 or 0x01 depending on whether the tuple
   is hit or not. Called on every new crash or timeout, should be
   reasonably fast. */

template<class UInt>
void afl::util::SimplifyTrace(UInt *mem, u32 map_size) {
    static std::vector<u8> simplify_lookup(256, 128);
    simplify_lookup[0] = 1;

    constexpr int width = sizeof(UInt);

    int wlog;
    UInt val_not_found;
    if constexpr (width == 4) {
        wlog = 2;
        val_not_found = 0x01010101;
    } else if constexpr (width == 8) {
        wlog = 3;
        val_not_found = 0x0101010101010101;
    }
    
    u32 i = map_size >> wlog;
    while (i--) {
        /* Optimize for sparse bitmaps. */
        
        if (unlikely(*mem)) {
            u8* mem8 = (u8*)mem;
            for (int j=0; j<width; j++) {
                mem8[j] = simplify_lookup[mem8[j]];
            }
        } else *mem = val_not_found;

        mem++;
    }
}

