#include "Algorithms/AFL/AFLState.hpp"

#include <unistd.h>
#include <sys/ioctl.h>

#include "Utils/Common.hpp"
#include "Algorithms/AFL/AFLMacro.hpp"
#include "Algorithms/AFL/AFLUtil.hpp"
#include "Algorithms/AFL/AFLFuzzer.hpp"
#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"
#include "Executor/NativeLinuxExecutor.hpp"

// FIXME: check if we are initializing all the members that need to be initialized
AFLState::AFLState(const AFLSetting &setting, NativeLinuxExecutor &executor) 
    : setting( setting ), 
      executor( executor ),
      input_set(),
      rand_fd( Util::OpenFile("/dev/urandom", O_RDONLY | O_CLOEXEC) ),
      should_construct_auto_dict(false)
{
    if (in_bitmap.empty()) virgin_bits.assign(AFLOption::MAP_SIZE, 255);
    else {
        ReadBitmap(in_bitmap);
    }

    /* Gnuplot output file. */

    auto plot_fn = setting.out_dir / "plot_data";
    plot_file = fopen(plot_fn.c_str(), "w");
    if (!plot_file) PFATAL("Unable to create '%s'", plot_fn.c_str());

    fprintf(plot_file, "# unix_time, cycles_done, cur_path, paths_total, "
                       "pending_total, pending_favs, map_size, unique_crashes, "
                       "unique_hangs, max_depth, execs_per_sec\n");
}

AFLState::~AFLState() {
    if (rand_fd != -1) {
        Util::CloseFile(rand_fd);
        rand_fd = -1;
    }

    fclose(plot_file);
}

InplaceMemoryFeedback AFLState::RunExecutorWithClassifyCounts(
    const u8* buf, 
    u32 len, 
    ExitStatusFeedback &exit_status,
    u32 tmout
) {
    total_execs++;

    if (tmout == 0) { 
        executor.Run(buf, len);
    } else {
        executor.Run(buf, len, tmout);
    }

    auto inp_feed = executor.GetAFLFeedback();
    exit_status = executor.GetExitStatusFeedback();

    if constexpr (sizeof(size_t) == 8) {
        inp_feed.ModifyMemoryWithFunc(
            [](u8* trace_bits, u32 map_size) {
                AFLFuzzer::ClassifyCounts<u64>((u64*)trace_bits, map_size);
            }
        );
    } else {
        inp_feed.ModifyMemoryWithFunc(
            [](u8* trace_bits, u32 map_size) {
                AFLFuzzer::ClassifyCounts<u32>((u32*)trace_bits, map_size);
            }
        );
    }

    return InplaceMemoryFeedback(std::move(inp_feed));
}

/* Get the number of runnable processes, with some simple smoothing. */

static double GetRunnableProcesses(void) {
    // FIXME: static variable
    static double res = 0;

#if defined(__APPLE__) || defined(__FreeBSD__) || defined (__OpenBSD__)

    /* I don't see any portable sysctl or so that would quickly give us the
       number of runnable processes; the 1-minute load average can be a
       semi-decent approximation, though. */

    if (getloadavg(&res, 1) != 1) return 0;

#else

    /* On Linux, /proc/stat is probably the best way; load averages are
       computed in funny ways and sometimes don't reflect extremely short-lived
       processes well. */

    FILE* f = fopen("/proc/stat", "r");
    char tmp[1024];
    u32 val = 0;

    if (!f) return 0;

    while (fgets(tmp, sizeof(tmp), f)) {
        if (!strncmp(tmp, "procs_running ", 14) ||
            !strncmp(tmp, "procs_blocked ", 14)) val += atoi(tmp + 14);
      }
 
    fclose(f);

    if (!res) {
        res = val;
    } else {
        res = res * (1.0 - 1.0 / AFLOption::AVG_SMOOTHING) +
              ((double)val) * (1.0 / AFLOption::AVG_SMOOTHING);
    }

#endif /* ^(__APPLE__ || __FreeBSD__ || __OpenBSD__) */

    return res;
}

/* Update stats file for unattended monitoring. */

void AFLState::WriteStatsFile(double bitmap_cvg, double stability, double eps) {
   
    auto fn = setting.out_dir / "fuzzer_stats";
    int fd = Util::OpenFile(fn.string(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) PFATAL("Unable to create '%s'", fn.c_str());

    FILE* f = fdopen(fd, "w");
    if (!f) PFATAL("fdopen() failed");

    /* Keep last values in case we're called from another context
       where exec/sec stats and such are not readily available. */

    if (!bitmap_cvg && !stability && !eps) {
        bitmap_cvg = last_bcvg;
        stability  = last_stab;
        eps        = last_eps;
    } else {
        last_bcvg = bitmap_cvg;
        last_stab = stability;
        last_eps  = eps;
    }

    fprintf(f, "start_time        : %llu\n"
               "last_update       : %llu\n"
               "fuzzer_pid        : %u\n"
               "cycles_done       : %llu\n"
               "execs_done        : %llu\n"
               "execs_per_sec     : %0.02f\n"
               "paths_total       : %u\n"
               "paths_favored     : %u\n"
               "paths_found       : %u\n"
               "paths_imported    : %u\n"
               "max_depth         : %u\n"
               "cur_path          : %u\n" /* Must match find_start_position() */
               "pending_favs      : %u\n"
               "pending_total     : %u\n"
               "variable_paths    : %u\n"
               "stability         : %0.02f%%\n"
               "bitmap_cvg        : %0.02f%%\n"
               "unique_crashes    : %llu\n"
               "unique_hangs      : %llu\n"
               "last_path         : %llu\n"
               "last_crash        : %llu\n"
               "last_hang         : %llu\n"
               "execs_since_crash : %llu\n"
               "exec_timeout      : %u\n" /* Must match find_timeout() */
               "afl_banner        : %s\n"
               "afl_version       : " AFL_VERSION "\n"
               "target_mode       : %s%s%s%s%s%s%s\n"
               "command_line      : %s\n"
               "slowest_exec_ms   : %llu\n",
               start_time / 1000, Util::GetCurTimeMs() / 1000, getpid(),
               queue_cycle ? (queue_cycle - 1) : 0, total_execs, eps,
               queued_paths, queued_favored, queued_discovered, queued_imported,
               max_depth, current_entry, pending_favored, pending_not_fuzzed,
               queued_variable, stability, bitmap_cvg, unique_crashes,
               unique_hangs, last_path_time / 1000, last_crash_time / 1000,
               last_hang_time / 1000, total_execs - last_crash_execs,
               setting.exec_timelimit_ms, use_banner.c_str(),
               qemu_mode ? "qemu " : "", setting.dumb_mode ? " dumb " : "",
               no_forkserver ? "no_forksrv " : "", 
               crash_mode != PUTExitReasonType::FAULT_NONE ? "crash " : "",
               persistent_mode ? "persistent " : "", deferred_mode ? "deferred " : "",
               (qemu_mode || setting.dumb_mode || no_forkserver || 
               crash_mode != PUTExitReasonType::FAULT_NONE ||
                persistent_mode || deferred_mode) ? "" : "default",
               orig_cmdline.c_str(), slowest_exec_ms);
               /* ignore errors */

    /* Get rss value from the children
       We must have killed the forkserver process and called waitpid
       before calling getrusage */

    struct rusage usage;

    if (getrusage(RUSAGE_CHILDREN, &usage)) {
        WARNF("getrusage failed");
    } else if (usage.ru_maxrss == 0) {
        fprintf(f, "peak_rss_mb       : not available while afl is running\n");
  } else {
#ifdef __APPLE__
        fprintf(f, "peak_rss_mb       : %zu\n", usage.ru_maxrss >> 20);
#else
        fprintf(f, "peak_rss_mb       : %zu\n", usage.ru_maxrss >> 10);
#endif /* ^__APPLE__ */
    }

    fclose(f);
}

void AFLState::SaveAuto(void) {
    if (!auto_changed) return;
    auto_changed = false;

    u32 lim = std::min<u32>(AFLOption::USE_AUTO_EXTRAS, a_extras.size());
    for (u32 i=0; i<lim; i++) {
        auto fn =   setting.out_dir 
                  / "queue/.state/auto_extras" 
                  / Util::StrPrintf("auto_%06u", i);

        int fd = Util::OpenFile(fn.string(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0) PFATAL("Unable to create '%s'", fn.c_str());

        Util::WriteFile(fd, &a_extras[i].data[0], a_extras[i].data.size());
        Util::CloseFile(fd);
    }
}

/* Write bitmap to file. The bitmap is useful mostly for the secret
   -B option, to focus a separate fuzzing session on a particular
   interesting input without rediscovering all the others. */

void AFLState::WriteBitmap(void) {
    if (!bitmap_changed) return;
    bitmap_changed = false;

    auto fn = setting.out_dir  / "fuzz_bitmap";
    
    int fd = Util::OpenFile(fn.string(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) PFATAL("Unable to create '%s'", fn.c_str());
    
    Util::WriteFile(fd, virgin_bits.data(), AFLOption::MAP_SIZE);
    Util::CloseFile(fd);
}

/* Read bitmap from file. This is for the -B option again. */

void AFLState::ReadBitmap(fs::path fname) {
    int fd = Util::OpenFile(fname.string(), O_RDONLY);
    if (fd < 0) PFATAL("Unable to open '%s'", fname.c_str());

    virgin_bits.resize(AFLOption::MAP_SIZE);
    Util::ReadFile(fd, virgin_bits.data(), AFLOption::MAP_SIZE);
    Util::CloseFile(fd);
}

void AFLState::MaybeUpdatePlotFile(double bitmap_cvg, double eps) {
    
    if (prev_qp == queued_paths && prev_pf == pending_favored &&
        prev_pnf == pending_not_fuzzed && prev_ce == current_entry &&
        prev_qc == queue_cycle && prev_uc == unique_crashes &&
        prev_uh == unique_hangs && prev_md == max_depth) return;

    prev_qp  = queued_paths;
    prev_pf  = pending_favored;
    prev_pnf = pending_not_fuzzed;
    prev_ce  = current_entry;
    prev_qc  = queue_cycle;
    prev_uc  = unique_crashes;
    prev_uh  = unique_hangs;
    prev_md  = max_depth;

    /* Fields in the file:

       unix_time, cycles_done, cur_path, paths_total, paths_not_fuzzed,
       favored_not_fuzzed, unique_crashes, unique_hangs, max_depth,
       execs_per_sec */

    fprintf(plot_file,
            "%llu, %llu, %u, %u, %u, %u, %0.02f%%, %llu, %llu, %u, %0.02f\n",
            Util::GetCurTimeMs() / 1000, queue_cycle - 1, current_entry, queued_paths,
            pending_not_fuzzed, pending_favored, bitmap_cvg, unique_crashes,
            unique_hangs, max_depth, eps); /* ignore errors */

    fflush(plot_file);
}

/* Check terminal dimensions after resize. */

static bool CheckTermSize() {
  struct winsize ws;

  bool term_too_small = false;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)) return term_too_small;

  if (ws.ws_row == 0 && ws.ws_col == 0) return term_too_small;
  if (ws.ws_row < 25 || ws.ws_col < 80) term_too_small = true;

  return term_too_small;
}

void AFLState::ShowStats(void) {
    u64 cur_ms = Util::GetCurTimeMs();

    /* If not enough time has passed since last UI update, bail out. */
    if (cur_ms - last_ms < 1000 / AFLOption::UI_TARGET_HZ) return;

    /* Check if we're past the 10 minute mark. */
    if (cur_ms - start_time > 10 * 60 * 1000) run_over10m = true;

    /* Calculate smoothed exec speed stats. */
    if (last_execs == 0) {
        avg_exec = ((double)total_execs) * 1000 / (cur_ms - start_time);
    } else {
        double cur_avg = ((double)(total_execs - last_execs)) * 1000 /
                     (cur_ms - last_ms);

        /* If there is a dramatic (5x+) jump in speed, reset the indicator
           more quickly. */
        if (cur_avg * 5 < avg_exec || cur_avg / 5 > avg_exec)
            avg_exec = cur_avg;

        avg_exec = avg_exec * (1.0 - 1.0 / AFLOption::AVG_SMOOTHING) +
                   cur_avg  * (1.0 / AFLOption::AVG_SMOOTHING);
    }

    last_ms = cur_ms;
    last_execs = total_execs;

    /* Tell the callers when to contact us (as measured in execs). */
    stats_update_freq = avg_exec / (AFLOption::UI_TARGET_HZ * 10);
    if (!stats_update_freq) stats_update_freq = 1;

    /* Do some bitmap stats. */
    u32 t_bytes = Util::CountNon255Bytes(&virgin_bits[0], virgin_bits.size());
    double t_byte_ratio = ((double)t_bytes * 100) / AFLOption::MAP_SIZE;

    double stab_ratio;
    if (t_bytes)
        stab_ratio = 100 - ((double)var_byte_count) * 100 / t_bytes;
    else 
        stab_ratio = 100;

    /* Roughly every minute, update fuzzer stats and save auto tokens. */
    if (cur_ms - last_stats_ms > AFLOption::STATS_UPDATE_SEC * 1000) {
        last_stats_ms = cur_ms;

        WriteStatsFile(t_byte_ratio, stab_ratio, avg_exec);
        SaveAuto();
        WriteBitmap();
    }

    /* Every now and then, write plot data. */
    if (cur_ms - last_plot_ms > AFLOption::PLOT_UPDATE_SEC * 1000) {
        last_plot_ms = cur_ms;
        MaybeUpdatePlotFile(t_byte_ratio, avg_exec);
    }

    /* Honor AFL_EXIT_WHEN_DONE and AFL_BENCH_UNTIL_CRASH. */
    if (!setting.dumb_mode && cycles_wo_finds > 100 && !pending_not_fuzzed && 
        getenv("AFL_EXIT_WHEN_DONE")) stop_soon = 2;

    if (total_crashes && getenv("AFL_BENCH_UNTIL_CRASH")) stop_soon = 2;

    /* If we're not on TTY, bail out. */
    if (not_on_tty) return;

    /* Compute some mildly useful bitmap stats. */
    u32 t_bits = (AFLOption::MAP_SIZE << 3) - Util::CountBits(&virgin_bits[0], virgin_bits.size()); 

    /* Now, for the visuals... */
    bool term_too_small = false;
    if (clear_screen) {
        SAYF(TERM_CLEAR CURSOR_HIDE);
        clear_screen = false;

        term_too_small = CheckTermSize();
    }

    SAYF(TERM_HOME);

    if (term_too_small) {
        SAYF(cBRI "Your terminal is too small to display the UI.\n"
             "Please resize terminal window to at least 80x25.\n" cRST);

        return;
    }

    // FIXME: use stringstream...?
    // anyways it's weird to mix up sprintf and std::string

    /* Let's start by drawing a centered banner. */
    u32 banner_len =  (crash_mode != PUTExitReasonType::FAULT_NONE ? 24 : 22) 
                    + strlen(AFL_VERSION) + use_banner.size();
    u32 banner_pad = (80 - banner_len) / 2;

    auto fuzzer_name = crash_mode != PUTExitReasonType::FAULT_NONE ?
                          cPIN "fuzzuf peruvian were-rabbit"
                        : cYEL "fuzzuf american fuzzy lop";

    std::string tmp(banner_pad, ' ');
    tmp += Util::StrPrintf("%s " cLCY AFL_VERSION cLGN " (%s)", fuzzer_name, use_banner.c_str());
    SAYF("\n%s\n\n", tmp.c_str());
    
    /* Lord, forgive me this. */
    SAYF(SET_G1 bSTG bLT bH bSTOP cCYA " process timing " bSTG bH30 bH5 bH2 bHB
         bH bSTOP cCYA " overall results " bSTG bH5 bRT "\n");

    std::string col;
    if (setting.dumb_mode) {
        col = cRST;
    } else {
        u64 min_wo_finds = (cur_ms - last_path_time) / 1000 / 60;
        
        if (queue_cycle == 1 || min_wo_finds < 15) {
            /* First queue cycle: don't stop now! */
            col = cMGN;
        } else if (cycles_wo_finds < 25 || min_wo_finds < 30) {
            /* Subsequent cycles, but we're still making finds. */
            col = cYEL;
        } else if (cycles_wo_finds > 100 
                && !pending_not_fuzzed && min_wo_finds > 120) {
            /* No finds for a long time and no test cases to try. */
            col = cLGN;
        } else {
            /* Default: cautiously OK to stop? */
            col = cLBL;
        }
    }

    // From here, we use these a lot...!
    using afl::util::DescribeInteger;
    using afl::util::DescribeFloat;
    using afl::util::DescribeTimeDelta;

    SAYF(bV bSTOP "        run time : " cRST "%-34s " bSTG bV bSTOP
         "  cycles done : %s%-5s  " bSTG bV "\n",
         DescribeTimeDelta(cur_ms, start_time).c_str(), col.c_str(),
         DescribeInteger(queue_cycle - 1).c_str());

    /* We want to warn people about not seeing new paths after a full cycle,
       except when resuming fuzzing or running in non-instrumented mode. */

    if (!setting.dumb_mode && 
          ( last_path_time 
         || resuming_fuzz 
         || queue_cycle == 1 
         || !in_bitmap.empty() 
         || crash_mode != PUTExitReasonType::FAULT_NONE)
       ) {

        SAYF(bV bSTOP "   last new path : " cRST "%-34s ",
             DescribeTimeDelta(cur_ms, last_path_time).c_str());

    } else {
        if (setting.dumb_mode) {
            SAYF(bV bSTOP "   last new path : " cPIN "n/a" cRST 
                 " (non-instrumented mode)        ");
        } else {
            SAYF(bV bSTOP "   last new path : " cRST "none yet " cLRD
                 "(odd, check syntax!)      ");
        }
    }

    SAYF(bSTG bV bSTOP "  total paths : " cRST "%-5s  " bSTG bV "\n",
         DescribeInteger(queued_paths).c_str());

    /* Highlight crashes in red if found, denote going over the KEEP_UNIQUE_CRASH
       limit with a '+' appended to the count. */

    tmp = DescribeInteger(unique_crashes);
    if (unique_crashes >= AFLOption::KEEP_UNIQUE_CRASH) tmp += '+';

    SAYF(bV bSTOP " last uniq crash : " cRST "%-34s " bSTG bV bSTOP
         " uniq crashes : %s%-6s " bSTG bV "\n",
         DescribeTimeDelta(cur_ms, last_crash_time).c_str(), 
         unique_crashes ? cLRD : cRST, tmp.c_str());

    tmp = DescribeInteger(unique_hangs);
    if (unique_hangs >= AFLOption::KEEP_UNIQUE_CRASH) tmp += '+';
    
    SAYF(bV bSTOP "  last uniq hang : " cRST "%-34s " bSTG bV bSTOP 
         "   uniq hangs : " cRST "%-6s " bSTG bV "\n",
         DescribeTimeDelta(cur_ms, last_hang_time).c_str(), tmp.c_str());

    SAYF(bVR bH bSTOP cCYA " cycle progress " bSTG bH20 bHB bH bSTOP cCYA
         " map coverage " bSTG bH bHT bH20 bH2 bH bVL "\n");

    /* This gets funny because we want to print several variable-length variables
       together, but then cram them into a fixed-width field - so we need to
       put them in a temporary buffer first. */

    auto& queue_cur = *case_queue[current_entry];

    tmp = DescribeInteger(current_entry);
    if (!queue_cur.favored) tmp += '*';
    tmp += Util::StrPrintf(" (%0.02f%%)", ((double)current_entry * 100) / queued_paths);

    SAYF(bV bSTOP "  now processing : " cRST "%-17s " bSTG bV bSTOP, tmp.c_str());

    tmp = Util::StrPrintf("%0.02f%% / %0.02f%%", 
                          ((double)queue_cur.bitmap_size) * 100 / AFLOption::MAP_SIZE, t_byte_ratio);

    SAYF("    map density : %s%-21s " bSTG bV "\n", t_byte_ratio > 70 ? cLRD : 
         ((t_bytes < 200 && !setting.dumb_mode) ? cPIN : cRST), tmp.c_str());

    tmp = DescribeInteger(cur_skipped_paths);
    tmp += Util::StrPrintf(" (%0.02f%%)", ((double)cur_skipped_paths * 100) / queued_paths);

    SAYF(bV bSTOP " paths timed out : " cRST "%-17s " bSTG bV, tmp.c_str());

    tmp = Util::StrPrintf("%0.02f bits/tuple", t_bytes ? (((double)t_bits) / t_bytes) : 0);
    
    SAYF(bSTOP " count coverage : " cRST "%-21s " bSTG bV "\n", tmp.c_str());

    SAYF(bVR bH bSTOP cCYA " stage progress " bSTG bH20 bX bH bSTOP cCYA
         " findings in depth " bSTG bH20 bVL "\n");

    tmp = DescribeInteger(queued_favored);
    tmp += Util::StrPrintf(" (%0.02f%%)", ((double)queued_favored) * 100 / queued_paths);
 
    /* Yeah... it's still going on... halp? */

    SAYF(bV bSTOP "  now trying : " cRST "%-21s " bSTG bV bSTOP
         " favored paths : " cRST "%-22s " bSTG bV "\n", stage_name.c_str(), tmp.c_str());

    if (!stage_max) {
        tmp = DescribeInteger(stage_cur) + "/-";
    } else {
        tmp = DescribeInteger(stage_cur) + "/" + DescribeInteger(stage_max);
        tmp += Util::StrPrintf(" (%0.02f%%)", ((double)stage_cur) * 100 / stage_max);
    }

    SAYF(bV bSTOP " stage execs : " cRST "%-21s " bSTG bV bSTOP, tmp.c_str());

    tmp = DescribeInteger(queued_with_cov);
    tmp += Util::StrPrintf(" (%0.02f%%)", ((double)queued_with_cov) * 100 / queued_paths);

    SAYF("  new edges on : " cRST "%-22s " bSTG bV "\n", tmp.c_str());

    tmp = DescribeInteger(total_crashes) + " ("
        + DescribeInteger(unique_crashes);
    if (unique_crashes >= AFLOption::KEEP_UNIQUE_CRASH) tmp += '+';
    tmp += " unique)";

    if (crash_mode != PUTExitReasonType::FAULT_NONE) {
        SAYF(bV bSTOP " total execs : " cRST "%-21s " bSTG bV bSTOP
             "   new crashes : %s%-22s " bSTG bV "\n", 
             DescribeInteger(total_execs).c_str(),
             unique_crashes ? cLRD : cRST, tmp.c_str());
    } else {
        SAYF(bV bSTOP " total execs : " cRST "%-21s " bSTG bV bSTOP
             " total crashes : %s%-22s " bSTG bV "\n", 
             DescribeInteger(total_execs).c_str(),
             unique_crashes ? cLRD : cRST, tmp.c_str());
    }

    /* Show a warning about slow execution. */

    if (avg_exec < 100) {
        tmp = DescribeFloat(avg_exec) + "/sec (";
        if (avg_exec < 20) tmp += "zzzz...";
        else tmp += "slow!";
        tmp += ')';
        
        SAYF(bV bSTOP "  exec speed : " cLRD "%-21s ", tmp.c_str());
    } else {
        tmp = DescribeFloat(avg_exec) + "/sec";

        SAYF(bV bSTOP "  exec speed : " cRST "%-21s ", tmp.c_str());
    }

    tmp = DescribeInteger(total_tmouts) + " (" +
          DescribeInteger(unique_tmouts);
    if (unique_hangs >= AFLOption::KEEP_UNIQUE_HANG) tmp += '+';
    tmp += ')';

    SAYF (bSTG bV bSTOP "  total tmouts : " cRST "%-22s " bSTG bV "\n", tmp.c_str());

    /* Aaaalmost there... hold on! */

    SAYF(bVR bH cCYA bSTOP " fuzzing strategy yields " bSTG bH10 bH bHT bH10
         bH5 bHB bH bSTOP cCYA " path geometry " bSTG bH5 bH2 bH bVL "\n");

    // In original AFL, the following part is unrolled, which is too long.
    // So put them into a loop as many as possible and wish compiler's optimization.

    // First, define the type describing the information output in one line:
    using OnelineInfo = std::tuple<
        const char *, // mut_name.      e.g. "bit flips", "arithmetics"
        const char *, // neighbor_name. e.g. "levels", "pending", "pend fav"
        std::string,  // neighbor_val.  e.g. DI(max_depth), DI(pending_not_fuzzed), DI(pending_favored)
        u8,           // stage1.        e.g. AFLOption::STAGE_FLIP8, AFLOption::STAGE_ARITH8
        u8,           // stage2.        e.g. AFLOption::STAGE_FLIP16, AFLOption::STAGE_ARITH16
        u8            // stage3.        e.g. AFLOption::STAGE_FLIP32, AFLOption::STAGE_ARITH32
    >;

    // Next, define each lines
    OnelineInfo line_infos[] = {
        { "   bit flips", "    levels", DescribeInteger(max_depth), 
          AFLOption::STAGE_FLIP1, AFLOption::STAGE_FLIP2, AFLOption::STAGE_FLIP4 },
        { "  byte flips", "   pending", DescribeInteger(pending_not_fuzzed), 
          AFLOption::STAGE_FLIP8, AFLOption::STAGE_FLIP16, AFLOption::STAGE_FLIP32 },
        { " arithmetics", "  pend fav", DescribeInteger(pending_favored), 
          AFLOption::STAGE_ARITH8, AFLOption::STAGE_ARITH16, AFLOption::STAGE_ARITH32 },
        { "  known ints", " own finds", DescribeInteger(queued_discovered),
          AFLOption::STAGE_INTEREST8, AFLOption::STAGE_INTEREST16, AFLOption::STAGE_INTEREST32 },
        { "  dictionary", "  imported", sync_id.empty() ? "n/a" : DescribeInteger(queued_imported),
          AFLOption::STAGE_EXTRAS_UO, AFLOption::STAGE_EXTRAS_UI, AFLOption::STAGE_EXTRAS_AO }
    }; // Havoc is difficult to put together

    tmp = "n/a, n/a, n/a";
    for (int i=0; i<5; i++) {
        auto [mut_name, neighbor_name, neighbor_val, stage1, stage2, stage3] 
            = line_infos[i];

        if (!skip_deterministic) {
            tmp =        DescribeInteger(stage_finds [stage1]) +
                   '/' + DescribeInteger(stage_cycles[stage1]) +

                  ", " + DescribeInteger(stage_finds [stage2]) +
                   '/' + DescribeInteger(stage_cycles[stage2]) +

                  ", " + DescribeInteger(stage_finds [stage3]) +
                   '/' + DescribeInteger(stage_cycles[stage3]);
        }
 
        SAYF(bV bSTOP "%s : " cRST "%-37s " bSTG bV bSTOP "%s : "
             cRST "%-10s " bSTG bV "\n", 
             mut_name, tmp.c_str(), neighbor_name, neighbor_val.c_str());
    }

    tmp =        DescribeInteger(stage_finds [AFLOption::STAGE_HAVOC]) +
           '/' + DescribeInteger(stage_cycles[AFLOption::STAGE_HAVOC]) +

          ", " + DescribeInteger(stage_finds [AFLOption::STAGE_SPLICE]) +
           '/' + DescribeInteger(stage_cycles[AFLOption::STAGE_SPLICE]);

    SAYF(bV bSTOP "       havoc : " cRST "%-37s " bSTG bV bSTOP, tmp.c_str());

    if (t_bytes) tmp = Util::StrPrintf("%0.02f%%", stab_ratio);
    else tmp = "n/a";

    SAYF(" stability : %s%-10s " bSTG bV "\n", (stab_ratio < 85 && var_byte_count > 40) 
         ? cLRD : ((queued_variable && (!persistent_mode || var_byte_count > 20))
         ? cMGN : cRST), tmp.c_str());

    if (!bytes_trim_out) {
        tmp = "n/a, ";
    } else {
        tmp = Util::StrPrintf("%0.02f%%", 
                ((double)(bytes_trim_in - bytes_trim_out)) * 100 / bytes_trim_in);
        tmp += "/" + DescribeInteger(trim_execs) + ", ";
    }

    if (!blocks_eff_total) {
        tmp += "n/a";
    } else {
        tmp += Util::StrPrintf("%0.02f%%", 
                ((double)(blocks_eff_total - blocks_eff_select)) * 100 / blocks_eff_total);
    }

    SAYF(bV bSTOP "        trim : " cRST "%-37s " bSTG bVR bH20 bH2 bH2 bRB "\n"
         bLB bH30 bH20 bH2 bH bRB bSTOP cRST RESET_G1, tmp.c_str());

    /* Provide some CPU utilization stats. */

    if (executor.cpu_core_count) {
        double cur_runnable = GetRunnableProcesses();
        u32 cur_utilization = cur_runnable * 100 / executor.cpu_core_count;

        std::string cpu_color = cCYA;

        /* If we could still run one or more processes, use green. */

        if (executor.cpu_core_count > 1 && 
            cur_runnable + 1 <= executor.cpu_core_count)
            cpu_color = cLGN;

        /* If we're clearly oversubscribed, use red. */

        if (!no_cpu_meter_red && cur_utilization >= 150) cpu_color = cLRD;

#ifdef HAVE_AFFINITY

        if (executor.binded_cpuid.has_value()) {
            SAYF(SP10 cGRA "[cpu%03d:%s%3u%%" cGRA "]\r" cRST,
                 std::min(executor.binded_cpuid.value(), 999), 
                 cpu_color.c_str(), std::min(cur_utilization, 999u));
        } else {
            SAYF(SP10 cGRA "   [cpu:%s%3u%%" cGRA "]\r" cRST,
                 cpu_color.c_str(), std::min(cur_utilization, 999u));
        }

#else

        SAYF(SP10 cGRA "   [cpu:%s%3u%%" cGRA "]\r" cRST,
             cpu_color.c_str(), std::min(cur_utilization, 999u));

#endif /* ^HAVE_AFFINITY */

    } else SAYF("\r");

    /* Hallelujah! */

    fflush(0);
}

void AFLState::ReceiveStopSignal(void) {
    stop_soon = 1;
}

bool AFLState::ShouldConstructAutoDict(void) {
    return should_construct_auto_dict;
}

void AFLState::SetShouldConstructAutoDict(bool v) {
    should_construct_auto_dict = v;
}
