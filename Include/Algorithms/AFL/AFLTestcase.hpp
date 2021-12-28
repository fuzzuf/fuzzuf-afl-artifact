#pragma once

#include <memory>
#include <bitset>

#include "Options.hpp"
#include "ExecInput/OnDiskExecInput.hpp"

struct AFLTestcase {
    explicit AFLTestcase(std::shared_ptr<OnDiskExecInput> input);
    ~AFLTestcase();

    std::shared_ptr<OnDiskExecInput> input;

    u8 cal_failed = 0;            /* Calibration failed?              */
    bool trim_done = false;       /* Trimmed?                         */
    bool was_fuzzed = false;      /* Had any fuzzing done yet?        */
    bool passed_det = false;      /* Deterministic stages passed?     */
    bool has_new_cov = false;     /* Triggers new coverage?           */
    bool var_behavior = false;    /* Variable behavior?               */
    bool favored = false;         /* Currently favored?               */
    bool fs_redundant = false;    /* Marked as redundant in the fs?   */

    u32 bitmap_size = 0;          /* Number of bits set in bitmap     */
    u32 exec_cksum = 0;           /* Checksum of the execution trace  */

    u64 exec_us = 0;              /* Execution time (us)              */
    u64 handicap = 0;             /* Number of queue cycles behind    */
    u64 depth = 0;                /* Path depth                       */

    /* Trace bytes, if kept             */
    std::unique_ptr<std::bitset<AFLOption::MAP_SIZE>> trace_mini;

    u32 tc_ref = 0;               /* Trace bytes ref count            */
};
