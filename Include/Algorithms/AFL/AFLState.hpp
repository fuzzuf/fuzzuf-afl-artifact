#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Utils/Filesystem.hpp"
#include "ExecInput/ExecInput.hpp"
#include "ExecInput/ExecInputSet.hpp"
#include "Executor/NativeLinuxExecutor.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"
#include "Algorithms/AFL/AFLSetting.hpp"
#include "Algorithms/AFL/AFLMacro.hpp"
#include "Algorithms/AFL/AFLTestcase.hpp"
#include "Algorithms/AFL/AFLDictData.hpp"

// 責務：
//   - 本クラスのインスタンスのライフタイムは、HierarFlowのそれよりも長くなければならない

struct AFLState {
    // FIXME: how to support other executors?
    explicit AFLState(const AFLSetting &setting, NativeLinuxExecutor &executor);
    ~AFLState();

    AFLState( const AFLState& ) = delete;
    AFLState& operator=( const AFLState& ) = delete;

    InplaceMemoryFeedback RunExecutorWithClassifyCounts(
        const u8* buf,
        u32 len,
        ExitStatusFeedback &exit_status,
        u32 tmout = 0
    );

    void WriteStatsFile(double bitmap_cvg, double stability, double eps);
    void SaveAuto(void);
    void WriteBitmap(void);
    void ReadBitmap(fs::path fname);
    void MaybeUpdatePlotFile(double bitmap_cvg, double eps);
    void ShowStats(void);

    void ReceiveStopSignal(void);

    bool ShouldConstructAutoDict(void);
    void SetShouldConstructAutoDict(bool v);

    const AFLSetting &setting;
    NativeLinuxExecutor& executor;
    ExecInputSet input_set;

    // TODO: what if this product works on environments other than *NIX?
    int rand_fd = -1;

    // these will be required in dictionary construction
    std::vector<u8> a_collect;
    u32 a_len;

    // this will be required in dictionary construction and eff_map construction
    u32 queue_cur_exec_cksum = 0;

    // this will be required in havoc
    bool doing_det;

    // this will be required in havoc and splicing
    u32 orig_perf;

    // these will be required in WriteStatsFile
    // (originally, these are defined as its static variables)
    double last_bcvg;
    double last_stab;
    double last_eps;

    // these will be required in ShowStats
    // (originally, these are defined as its static variables)
    u64 last_ms = 0;
    u64 last_execs = 0;
    u64 last_plot_ms = 0;
    u64 last_stats_ms = 0;
    double avg_exec = 0.0;

    // these will be request in MaybeUpdatePlotFile
    // (originally, these are defined as its static variables)
    u32 prev_qp, prev_pf, prev_pnf, prev_ce, prev_md;
    u64 prev_qc, prev_uc, prev_uh;

    // FILE used in MaybeUpdatePlotFile
    FILE* plot_file;

    // AFLState has to own eff_map and prev_cksum in fuzz_one
    u32 eff_cnt;
    std::vector<u8> eff_map;
    u32 prev_cksum;

    u32 seek_to = 0; // = find_start_position();

    /*
     * Global variables in afl-fuzz.c(some of them are omitted) *
                                      */
    // FIXME: maybe we can split the below into subclasses?

    std::string sync_id;                    /* Fuzzer ID                        */
    std::string use_banner;                 /* Display banner                   */
    std::string in_bitmap;                  /* Input bitmap                     */
    std::string orig_cmdline;               /* Original command line            */

    /* Timeout used for hang det (ms)   */
    u32 hang_tmout = AFLOption::EXEC_TIMEOUT;

    u32 stats_update_freq = 1;              /* Stats update frequency (execs)   */

    bool skip_deterministic = false;        /* Skip deterministic stages?       */
    bool force_deterministic = false;       /* Force deterministic stages?      */
    bool use_splicing = false;              /* Recombine input files?           */
    bool score_changed = false;             /* Scoring for favorites changed?   */
    bool kill_signal = false;               /* Signal that killed the child     */
    bool resuming_fuzz = false;             /* Resuming an older fuzzing job?   */
    u8 timeout_given = 0;                   /* Specific timeout given?          */
    bool not_on_tty = false;                /* stdout is not a tty              */
    bool term_too_small = false;            /* terminal dimensions too small    */
    bool uses_asan = false;                 /* Target uses ASAN?                */
    bool no_forkserver = false;             /* Disable forkserver?              */
    /* Crash mode! Yeah!                */
    PUTExitReasonType crash_mode = PUTExitReasonType::FAULT_NONE;
    bool in_place_resume = false;           /* Attempt in-place resume?         */
    bool auto_changed = false;              /* Auto-generated tokens changed?   */
    bool no_cpu_meter_red = false;          /* Feng shui on the status screen   */
    bool no_arith = false;                  /* Skip most arithmetic ops         */
    bool shuffle_queue = false;             /* Shuffle input queue?             */
    bool bitmap_changed = true;             /* Time to update bitmap?           */
    bool qemu_mode = false;                 /* Running in QEMU mode?            */
    bool skip_requested = false;            /* Skip request, via SIGUSR1        */
    bool run_over10m = false;               /* Run time over 10 minutes?        */
    bool persistent_mode = false;           /* Running in persistent mode?      */
    bool deferred_mode = false;             /* Deferred forkserver mode?        */
    bool fast_cal = false;                  /* Try to calibrate faster?         */

    /* Regions yet untouched by fuzzing */
    std::vector<u8> virgin_bits; // its initialization depends on in_bitmap

    /* Bits we haven't seen in tmouts   */
    std::vector<u8> virgin_tmout = std::vector<u8>(AFLOption::MAP_SIZE, 255);

    /* Bits we haven't seen in crashes  */
    std::vector<u8> virgin_crash = std::vector<u8>(AFLOption::MAP_SIZE, 255);

    /* Bytes that appear to be variable */
    std::vector<u8> var_bytes    = std::vector<u8>(AFLOption::MAP_SIZE, 0);

    u8 stop_soon = 0;                       /* Ctrl-C pressed?                  */
    bool clear_screen = true;               /* Window resized?                  */

    u32 queued_paths = 0;                   /* Total number of queued testcases */
    u32 queued_variable = 0;                /* Testcases with variable behavior */
    u32 queued_at_start = 0;                /* Total number of initial inputs   */
    u32 queued_discovered = 0;              /* Items discovered during this run */
    u32 queued_imported = 0;                /* Items imported via -S            */
    u32 queued_favored = 0;                 /* Paths deemed favorable           */
    u32 queued_with_cov = 0;                /* Paths with new coverage bytes    */
    u32 pending_not_fuzzed = 0;             /* Queued but not done yet          */
    u32 pending_favored = 0;                /* Pending favored paths            */
    u32 cur_skipped_paths = 0;              /* Abandoned inputs in cur cycle    */
    u32 cur_depth = 0;                      /* Current path depth               */
    u32 max_depth = 0;                      /* Max path depth                   */
    u32 useless_at_start = 0;               /* Number of useless starting paths */
    u32 var_byte_count = 0;                 /* Bitmap bytes with var behavior   */
    u32 current_entry = 0;                  /* Current queue entry ID           */
    u32 havoc_div = 1;                      /* Cycle count divisor for havoc    */

    u64 total_crashes = 0;                  /* Total number of crashes          */
    u64 unique_crashes = 0;                 /* Crashes with unique signatures   */
    u64 total_tmouts = 0;                   /* Total number of timeouts         */
    u64 unique_tmouts = 0;                  /* Timeouts with unique signatures  */
    u64 unique_hangs = 0;                   /* Hangs with unique signatures     */
    u64 total_execs = 0;                    /* Total execve() calls             */
    u64 slowest_exec_ms = 0;                /* Slowest testcase non hang in ms  */
    u64 start_time = 0;                     /* Unix start time (ms)             */
    u64 last_path_time = 0;                 /* Time for most recent path (ms)   */
    u64 last_crash_time = 0;                /* Time for most recent crash (ms)  */
    u64 last_hang_time = 0;                 /* Time for most recent hang (ms)   */
    u64 last_crash_execs = 0;               /* Exec counter at last crash       */
    u64 queue_cycle = 0;                    /* Queue round counter              */
    u64 cycles_wo_finds = 0;                /* Cycles without any new paths     */
    u64 trim_execs = 0;                     /* Execs done to trim input files   */
    u64 bytes_trim_in = 0;                  /* Bytes coming into the trimmer    */
    u64 bytes_trim_out = 0;                 /* Bytes coming outa the trimmer    */
    u64 blocks_eff_total = 0;               /* Blocks subject to effector maps  */
    u64 blocks_eff_select = 0;              /* Blocks selected as fuzzable      */

    u32 subseq_tmouts = 0;                  /* Number of timeouts in a row      */

    std::string stage_name;                 /* Name of the current fuzz stage   */
    std::string stage_short;                /* Short stage name                 */
    std::string syncing_party;              /* Currently syncing with...        */

    s32 stage_cur = 0;                      /* Stage progression                */
    s32 stage_max = 0;                      /* Stage progression                */

    s32 splicing_with = -1;                 /* Splicing with which test case?   */

    u32 master_id = 0;                      /* Master instance job splitting    */
    u32 master_max = 0;                     /* Master instance job splitting    */

    u32 syncing_case = 0;                   /* Syncing with case #...           */

    s32 stage_cur_byte = 0;                 /* Byte offset of current stage op  */
    s32 stage_cur_val = 0;                  /* Value used for stage op          */

    // TODO: consider makeing enum for this
    /* Value type (STAGE_VAL_*)         */
    u8 stage_val_type = AFLOption::STAGE_VAL_NONE;

    /* Patterns found per fuzz stage    */
    std::vector<u64> stage_finds =  std::vector<u64>(32, 0);
    /* Execs per fuzz stage             */
    std::vector<u64> stage_cycles = std::vector<u64>(32, 0);

    u64 total_cal_us = 0;                   /* Total calibration time (us)      */
    u64 total_cal_cycles = 0;               /* Total calibration cycles         */

    u64 total_bitmap_size = 0;              /* Total bit count for all bitmaps  */
    u64 total_bitmap_entries = 0;           /* Number of bitmaps counted        */

    /* Fuzzing queue (vector)           */
    std::vector<std::shared_ptr<AFLTestcase>> case_queue; 

    /* Top entries for bitmap bytes     */
    std::vector<NullableRef<AFLTestcase>> top_rated 
        = std::vector<NullableRef<AFLTestcase>>(AFLOption::MAP_SIZE);

    /* Extra tokens to fuzz with        */
    std::vector<AFLDictData> extras;

    /* Automatically selected extras    */
    std::vector<AFLDictData> a_extras;

private:
    bool should_construct_auto_dict;
};
