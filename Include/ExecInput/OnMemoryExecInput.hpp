#pragma once

#include <memory>

#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"
#include "ExecInput/ExecInputSet.hpp"

class OnMemoryExecInput : public ExecInput {
public:     
    ~OnMemoryExecInput() {}

    // disable copies
    OnMemoryExecInput(const OnMemoryExecInput&) = delete;
    OnMemoryExecInput& operator=(const OnMemoryExecInput&) = delete;

    // allow moves
    OnMemoryExecInput(OnMemoryExecInput&&);
    OnMemoryExecInput& operator=(OnMemoryExecInput&&);

    void LoadIfNotLoaded(void);
    void Load(void);
    void Unload(void);
    void Save(void);
    void OverwriteKeepingLoaded(const u8* buf, u32 len);
    void OverwriteKeepingLoaded(std::unique_ptr<u8[]>&& buf, u32 len);
    void OverwriteThenUnload(const u8* buf, u32 len);
    void OverwriteThenUnload(std::unique_ptr<u8[]>&& buf, u32 len);

    void SaveToFile(const fs::path& path);

private:
    
    // ExecInput instances can be created only in ExecInputSet
    // (i.e. it's the factory of ExecInput)
    friend class ExecInputSet;
    OnMemoryExecInput(const u8* buf, u32 len);
    OnMemoryExecInput(std::unique_ptr<u8[]>&& buf, u32 len);
};
