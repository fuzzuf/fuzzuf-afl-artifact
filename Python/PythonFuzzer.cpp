#include <cstddef>
#include "Python/PythonFuzzer.hpp"
#include "Options.hpp"
#include "Utils/Common.hpp"
#include "Utils/Workspace.hpp"
#include "Python/PythonState.hpp"
#include "Python/PythonHierarFlowRoutines.hpp"
#include "Executor/NativeLinuxExecutor.hpp"
#include "Logger/Logger.hpp"

// NOTE: 流石に雑すぎる
void PythonFuzzer::ExecuteInitialSeeds(const fs::path &in_dir) {
    struct dirent **namelist;

    DEBUG("Scanning %s\n", in_dir.c_str());

    int dirnum = Util::ScanDirAlpha(in_dir.string(), &namelist);
    if (dirnum < 0) {
      ERROR("Unable to open '%s'", in_dir.c_str());
    }

    for (int i = 0; i < dirnum; i++) {
        struct stat st;

        std::string fn = Util::StrPrintf("%s/%s", 
                                      in_dir.c_str(), namelist[i]->d_name);

        free(namelist[i]); /* not tracked */

        if (lstat(fn.c_str(), &st) != 0 || access(fn.c_str(), R_OK) != 0) {
            ERROR("Unable to access '%s'", fn.c_str());
        }

        /* This also takes care of `.` and `..` */

        if (!S_ISREG(st.st_mode) || !st.st_size) {
            continue;
        }

        if (st.st_size > AFLOption::MAX_FILE) {
            ERROR("Test case '%s' is too big\n", fn.c_str());
        }

        DEBUG("Attempting dry run with '%s'...\n", fn.c_str());

        auto buf = std::make_unique<u8[]>(st.st_size);
        int fd = Util::OpenFile(fn, O_RDONLY);
        Util::ReadFile(fd, buf.get(), st.st_size);
        Util::CloseFile(fd);
        add_seed(buf.get(), st.st_size);
    }

    free(namelist);
}

PythonFuzzer::PythonFuzzer(
    const std::vector<std::string> &argv,
    const std::string &in_dir,
    const std::string &out_dir,
    u32 exec_timelimit_ms,
    u32 exec_memlimit,
    bool forksrv,
    bool need_afl_cov,
    bool need_bb_cov
) :
    setting( argv, 
             in_dir, 
             out_dir, 
             exec_timelimit_ms, 
             exec_memlimit,
             forksrv, 
             need_afl_cov,
             need_bb_cov
    ),
    state( new PythonState(setting) )
    // Executor and FuzzingPrimitive will be initialized inside the function
{
    Util::set_segv_handler::get();

    // Executor needs the directory specified by "out_dir" to be already set up
    // so we need to create the directory first, and then initialize Executor
    SetupDirs(setting.out_dir.string());

    executor.reset(new NativeLinuxExecutor(
                          setting.argv, 
                          setting.exec_timelimit_ms,
                          setting.exec_memlimit,
                          setting.forksrv,
                          setting.out_dir / AFLOption::DEFAULT_OUTFILE,
                          setting.need_afl_cov,
                          setting.need_bb_cov,
                          -1
    ));

    BuildFuzzFlow();

    ExecuteInitialSeeds(setting.in_dir);
}

PythonFuzzer::~PythonFuzzer() {}

// do not call non aync-signal-safe functions inside because this function can be called during signal handling
void PythonFuzzer::ReceiveStopSignal(void) {
    executor->ReceiveStopSignal();
}

void PythonFuzzer::Reset(void) {
    // すべてを確保した逆順で開放し、out_dirを初期化し、再度すべて確保し直す

    executor.reset();

    Util::DeleteFileOrDirectory(setting.out_dir.string());
    SetupDirs(setting.out_dir.string());

    state.reset(new PythonState(setting));

    executor.reset(new NativeLinuxExecutor(
                          setting.argv, 
                          setting.exec_timelimit_ms,
                          setting.exec_memlimit,
                          setting.forksrv,
                          setting.out_dir / AFLOption::DEFAULT_OUTFILE,
                          setting.need_afl_cov,
                          setting.need_bb_cov,
                          NativeLinuxExecutor::CPUID_DO_NOT_BIND
    ));

    BuildFuzzFlow();    

    ExecuteInitialSeeds(setting.in_dir);
}

void PythonFuzzer::Release(void) {
    executor.reset();
    state.reset();

    // move HierarFlowNodes to local variables to call destructor
    { auto _ = std::move(bit_flip); }
    { auto _ = std::move(byte_flip); }
    { auto _ = std::move(havoc); }
    { auto _ = std::move(add); }
    { auto _ = std::move(sub); }
    { auto _ = std::move(interest); }
    { auto _ = std::move(add_seed); }

    Util::DeleteFileOrDirectory(setting.out_dir.string());
}

void PythonFuzzer::BuildFuzzFlow() {
    using namespace pyfuzz::pipeline;

    using pipeline::CreateNode;
    using pipeline::WrapToMakeHeadNode;

    auto execute = CreateNode<PyExecutePUT>(*executor);
    auto update = CreateNode<PyUpdate>(*state);
    bit_flip = CreateNode<PyBitFlip>(*state);
    byte_flip = CreateNode<PyByteFlip>(*state);
    havoc = CreateNode<PyHavoc>(*state);
    add = CreateNode<PyAdd>(*state);
    sub = CreateNode<PySub>(*state);
    interest = CreateNode<PyInterest>(*state);
    overwrite = CreateNode<PyOverwrite>(*state);

    bit_flip << execute << update;
    byte_flip << execute.HardLink() << update.HardLink();
    havoc << execute.HardLink() << update.HardLink();
    add << execute.HardLink() << update.HardLink();
    sub << execute.HardLink() << update.HardLink();
    interest << execute.HardLink() << update.HardLink();
    overwrite << execute.HardLink() << update.HardLink();

    auto execute_for_add_seed = execute.HardLink();
    execute_for_add_seed << update.HardLink();
    add_seed = WrapToMakeHeadNode(execute_for_add_seed);
}

u64 PythonFuzzer::FlipBit(u32 pos, u32 len) {
    assert(state->mutator != nullptr);
    bit_flip(pos, len);
    state->mutator.reset();
    return bit_flip->GetResponseValue();
}

u64 PythonFuzzer::FlipByte(u32 pos, u32 len) {
    assert(state->mutator != nullptr);
    byte_flip(pos, len);
    state->mutator.reset();
    return byte_flip->GetResponseValue();
}

u64 PythonFuzzer::Havoc(u32 stacking) {
    assert(state->mutator != nullptr);
    havoc(stacking);
    state->mutator.reset();
    return havoc->GetResponseValue();
}

u64 PythonFuzzer::Add(u32 pos, int val, int bits, bool be) {
    assert(state->mutator != nullptr);
    add(pos, val, bits, be);
    state->mutator.reset();
    return add->GetResponseValue();
}

u64 PythonFuzzer::Sub(u32 pos, int val, int bits, bool be) {
    assert(state->mutator != nullptr);
    sub(pos, val, bits, be);
    state->mutator.reset();
    return sub->GetResponseValue();
}

u64 PythonFuzzer::Interest(u32 pos, int bits, int idx, bool be) {
    assert(state->mutator != nullptr);
    interest(pos, bits, idx, be);
    state->mutator.reset();
    return interest->GetResponseValue();
}

u64 PythonFuzzer::Overwrite(u32 pos, char chr) {
    assert(state->mutator != nullptr);
    overwrite(pos, chr);
    state->mutator.reset();
    return overwrite->GetResponseValue();
}

u64 PythonFuzzer::AddSeed(u32 len, const std::vector<u8>& buf) {
    assert(len == buf.size());
    add_seed(buf.data(), buf.size());
    return add_seed->GetResponseValue();
}

void PythonFuzzer::SelectSeed(u64 seed_id) {
    auto itr = state->test_set.find(seed_id);
    if (itr == state->test_set.end()) ERROR("specified seed ID is not found"); 
    
    state->mutator.reset(new Mutator(*itr->second->input));
}

void PythonFuzzer::RemoveSeed(u64 seed_id) {
    auto itr = state->test_set.find(seed_id);
    if (itr == state->test_set.end()) ERROR("specified seed ID is not found");

    state->input_set.erase(seed_id);
    state->test_set.erase(seed_id);
}

std::vector<u64> PythonFuzzer::GetSeedIDs(void) {
    return state->input_set.get_ids();
}

std::optional<PySeed> PythonFuzzer::GetPySeed(u64 seed_id) {
    auto itr = state->test_set.find(seed_id);
    if (itr == state->test_set.end()) return std::nullopt;

    auto& testcase = *itr->second;
    auto& input = *testcase.input;
    return PySeed(seed_id, 
                  std::vector<u8>(input.GetBuf(), input.GetBuf() + input.GetLen()),
                  testcase.bb_feed.GetTrace(), testcase.afl_feed.GetTrace());
}

std::vector<std::unordered_map<int, u8>> PythonFuzzer::GetAFLTraces(void) {
    std::vector<std::unordered_map<int, u8>> ret;
    for( auto& itr : state->test_set )
        ret.emplace_back(itr.second->afl_feed.GetTrace());
    return ret;
}

std::vector<std::unordered_map<int, u8>> PythonFuzzer::GetBBTraces(void) {
    std::vector<std::unordered_map<int, u8>> ret;
    for( auto& itr : state->test_set )
        ret.emplace_back(itr.second->bb_feed.GetTrace());
    return ret;
}

void PythonFuzzer::SuppressLog() {
  runlevel = RunLevel::MODE_RELEASE;
}

void PythonFuzzer::ShowLog() {
  runlevel = RunLevel::MODE_DEBUG;
}
