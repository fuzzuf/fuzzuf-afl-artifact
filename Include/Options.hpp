#ifndef __OPTIONS_HPP__
#define __OPTIONS_HPP__

#include "Utils/Common.hpp"

// FIXME: this should be moved to Include/Algorithms/AFL/
namespace AFLOption{

static const u32 RESEED_RNG         =   10000;

static const u32 EXEC_TIMEOUT       =    1000;

// FIXME: use MEM_LIMIT=50 in 32-bit environments
static const u32 MEM_LIMIT          =      25;

/* Stage value types */
static const u32 STAGE_VAL_NONE     =       0;
static const u32 STAGE_VAL_LE       =       1;
static const u32 STAGE_VAL_BE       =       2;

static const u32 STAGE_FLIP1        =       0;
static const u32 STAGE_FLIP2        =       1;
static const u32 STAGE_FLIP4        =       2;
static const u32 STAGE_FLIP8        =       3;
static const u32 STAGE_FLIP16       =       4;
static const u32 STAGE_FLIP32       =       5;
static const u32 STAGE_ARITH8       =       6;
static const u32 STAGE_ARITH16      =       7;
static const u32 STAGE_ARITH32      =       8;
static const u32 STAGE_INTEREST8    =       9;
static const u32 STAGE_INTEREST16   =      10;
static const u32 STAGE_INTEREST32   =      11;
static const u32 STAGE_EXTRAS_UO    =      12;
static const u32 STAGE_EXTRAS_UI    =      13;
static const u32 STAGE_EXTRAS_AO    =      14;
static const u32 STAGE_HAVOC        =      15;
static const u32 STAGE_SPLICE       =      16;

static const u32 CAL_CYCLES         =       8;
static const u32 CAL_CYCLES_LONG    =       40;
/* Number of subsequent timeouts before abandoning an input file: */
static const u32 TMOUT_LIMIT        =       250;
/* Maximum number of unique hangs or crashes to record: */
static const u32 KEEP_UNIQUE_HANG   =       500;
static const u32 KEEP_UNIQUE_CRASH  =       5000;
/* Baseline number of random tweaks during a single 'havoc' stage: */
static const u32 HAVOC_CYCLES       =       256;
static const u32 HAVOC_CYCLES_INIT  =       1024;
/* Maximum multiplier for the above (should be a power of two, beware
    of 32-bit int overflows): */
static const u32 HAVOC_MAX_MULT     =       16;
/* Absolute minimum number of havoc cycles (after all adjustments): */
static const s32 HAVOC_MIN          =       16;
    
/* Maximum stacking for havoc-stage tweaks. The actual value is calculated
    like this: 

    n = random between 1 and HAVOC_STACK_POW2
    stacking = 2^n

    In other words, the default (n = 7) produces 2, 4, 8, 16, 32, 64, or
    128 stacked tweaks: */
static const u32 HAVOC_STACK_POW2   =       7;

/* Caps on block sizes for cloning and deletion operations. Each of these
    ranges has a 33% probability of getting picked, except for the first
  two cycles where smaller blocks are favored: */

static const u32 HAVOC_BLK_SMALL    =       32;
static const u32 HAVOC_BLK_MEDIUM   =       128;
static const u32 HAVOC_BLK_LARGE    =       1500;

/* Extra-large blocks, selected very rarely (<5% of the time): */
static const u32 HAVOC_BLK_XL       =       32768;

/* Calibration timeout adjustments, to be a bit more generous when resuming
    fuzzing sessions or trying to calibrate already-added internal finds.
    The first value is a percentage, the other is in milliseconds: */
static const u32 CAL_TMOUT_PERC     =       125;
static const u32 CAL_TMOUT_ADD      =       50;
/* Number of chances to calibrate a case before giving up: */
static const u32 CAL_CHANCES        =       3;
    
static const u32 MAP_SIZE_POW2      =       16;
static const u32 MAP_SIZE           =       (1 << MAP_SIZE_POW2);
static const u32 STATUS_UPDATE_FREQ =       1;
static const u32 EXEC_FAIL_SIG      =       0xfee1dead;


constexpr const char *DEFAULT_OUTFILE  =       ".cur_input";
constexpr const char *CLANG_ENV_VAR    =       "__AFL_CLANG_MODE";
constexpr const char *AS_LOOP_ENV_VAR  =       "__AFL_AS_LOOPCHECK";
constexpr const char *PERSIST_ENV_VAR  =       "__AFL_PERSISTENT";
constexpr const char *DEFER_ENV_VAR    =       "__AFL_DEFER_FORKSRV";
constexpr const char *AFL_SHM_ENV_VAR     =    "__AFL_SHM_ID";
constexpr const char *WYVERN_SHM_ENV_VAR  =    "__WYVERN_SHM_ID";   
 
/* those fd numbers are used by the fork server inside PUT */
const int FORKSRV_FD_READ  = 198;
const int FORKSRV_FD_WRITE = 199;

/* Distinctive exit code used to indicate MSAN trip condition: */
static const u32 MSAN_ERROR         =       86;
static const u32 SKIP_TO_NEW_PROB   =       99; /* ...when there are new, pending favorites */
static const u32 SKIP_NFAV_OLD_PROB =       95; /* ...no new favs, cur entry already fuzzed */
static const u32 SKIP_NFAV_NEW_PROB =       75; /* ...no new favs, cur entry not fuzzed yet */

/* Splicing cycle count: */
static const u32 SPLICE_CYCLES      =       15;
/* Nominal per-splice havoc cycle length: */
static const u32 SPLICE_HAVOC       =       32;
/* Maximum offset for integer addition / subtraction stages: */
static const u32 ARITH_MAX          =       35;

/* Maximum size of input file, in bytes (keep under 100MB): */
static const u32 MAX_FILE           =       (1 * 1024 * 1024);

/* The same, for the test case minimizer: */
static const u32 TMIN_MAX_FILE      =       (10 * 1024 * 1024);

/* Block normalization steps for afl-tmin: */
static const u32 TMIN_SET_MIN_SIZE  =       4;
static const u32 TMIN_SET_STEPS     =       128;

/* Maximum dictionary token size (-x), in bytes: */
static const u32 MAX_DICT_FILE      =       128;

/* Length limits for auto-detected dictionary tokens: */
static const u32 MIN_AUTO_EXTRA     =       3;
static const u32 MAX_AUTO_EXTRA     =       32;

/* Maximum number of user-specified dictionary tokens to use in deterministic
    steps; past this point, the "extras/user" step will be still carried out,
    but with proportionally lower odds: */
static const u32 MAX_DET_EXTRAS     =       200;

/* Maximum number of auto-extracted dictionary tokens to actually use in fuzzing
    (first value), and to keep in memory as candidates. The latter should be much
    higher than the former. */
static const u32 USE_AUTO_EXTRAS    =       50;
static const u32 MAX_AUTO_EXTRAS    =       (USE_AUTO_EXTRAS * 10);

/* Scaling factor for the effector map used to skip some of the more
    expensive deterministic steps. The actual divisor is set to
    2^EFF_MAP_SCALE2 bytes: */
static const u32 EFF_MAP_SCALE2     =       3;
/* Minimum input file length at which the effector logic kicks in: */
static const u32 EFF_MIN_LEN        =       128;
/* Maximum effector density past which everything is just fuzzed
    unconditionally (%): */
static const u32 EFF_MAX_PERC       =       90;

/* UI refresh frequency (Hz): */
static const u32 UI_TARGET_HZ       =       5;

/* Fuzzer stats file and plot update intervals (sec): */
static const u32 STATS_UPDATE_SEC   =       60;
static const u32 PLOT_UPDATE_SEC    =       60;

/* Smoothing divisor for CPU load and exec speed stats (1 - no smoothing). */
static const u32 AVG_SMOOTHING      =       16;
    
/* Limits for the test case trimmer. The absolute minimum chunk size; and
    the starting and ending divisors for chopping up the input file: */
static const u32 TRIM_MIN_BYTES     =       4;
static const u32 TRIM_START_STEPS   =       16;
static const u32 TRIM_END_STEPS     =       1024;
    
/* A made-up hashing seed: */
static const u32 HASH_CONST         =       0xa5b35705;
    
};

#endif
