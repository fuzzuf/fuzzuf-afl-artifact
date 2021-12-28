#pragma once

#include <vector>
#include <array>
#include <string>
#include <memory>
#include "Utils/Common.hpp"
#include "Fuzzer/Fuzzer.hpp"
#include "Algorithms/AFL/AFLSetting.hpp"
#include "Algorithms/AFL/AFLState.hpp"
#include "Algorithms/AFL/CountClasses.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

#include "Executor/NativeLinuxExecutor.hpp"

class AFLFuzzer : public Fuzzer {
public:
    explicit AFLFuzzer(
        // 参照渡しした変数は内部でコピーするので、ライフタイムの心配はありません。AFLSetting クラスを参照
        const std::vector<std::string> &argv,
        const std::string &in_dir,
        const std::string &out_dir,
        u32 exec_timelimit_ms,
        u32 exec_memlimit,
        bool forksrv
    );

    ~AFLFuzzer();

    static const CountClasses count_class;

    template<class UInt>
    static void ClassifyCounts(UInt *mem, u32 map_size);

    void BuildFuzzFlow(void);
    void OneLoop(void);
  
    void ReceiveStopSignal(void);

private:
    AFLSetting setting;
    
    // We need std::unique_ptr because we have to make the construction of these variables "delayed"
    // For example, NativeLinuxExecutor doesn't have the default constructor NativeLinuxExecutor()
    // nor operator=(). So we have no choice but to delay those constructors 
    std::unique_ptr<AFLState> state;
    std::unique_ptr<NativeLinuxExecutor> executor;
    HierarFlowNode<void(void), bool(std::shared_ptr<AFLTestcase>)> fuzz_loop;
};

// static method
template<class UInt>
void AFLFuzzer::ClassifyCounts(UInt *mem, u32 map_size) {
    constexpr unsigned int width = sizeof(UInt);
    static_assert(width == 4 || width == 8);

    int wlog;
    if constexpr (width == 4) {
      wlog = 2;
    } else if constexpr (width == 8) {
      wlog = 3;
    }

    u32 i = map_size >> wlog;
    while (i--) {
        /* Optimize for sparse bitmaps. */

        if (unlikely(*mem)) {
            u16* mem16 = (u16*)mem;

            for (unsigned int j=0; j*sizeof(u16) < width; j++) {
                mem16[j] = count_class.lookup16[mem16[j]];
            }
        }

        mem++;
    }
}