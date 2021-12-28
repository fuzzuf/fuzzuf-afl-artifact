#pragma once

#include <memory>

#include "Utils/Common.hpp"
#include "Utils/Filesystem.hpp"
#include "ExecInput/ExecInput.hpp"
#include "ExecInput/ExecInputSet.hpp"

class OnDiskExecInput : public ExecInput {
public:     
    ~OnDiskExecInput() {}

    // disable copies
    OnDiskExecInput(const OnDiskExecInput&) = delete;
    OnDiskExecInput& operator=(const OnDiskExecInput&) = delete;

    // allow moves
    OnDiskExecInput(OnDiskExecInput&&);
    OnDiskExecInput& operator=(OnDiskExecInput&&);

    void ReallocBufIfLack(u32 new_len);

    void LoadIfNotLoaded(void);
    void Load(void);
    void Unload(void);
    void Save(void);
    void OverwriteKeepingLoaded(const u8* buf, u32 len);
    void OverwriteKeepingLoaded(std::unique_ptr<u8[]>&& buf, u32 len);
    void OverwriteThenUnload(const u8* buf, u32 len);
    void OverwriteThenUnload(std::unique_ptr<u8[]>&& buf, u32 len);

    void LoadByMmap(void);
    bool Link(const fs::path& dest_path);
    void Copy(const fs::path& dest_path);
    bool LinkAndRefer(const fs::path& new_path);
    void CopyAndRefer(const fs::path& new_path);

    const fs::path& GetPath(void) const;

private:
    
    // ExecInput instances can be created only in ExecInputSet
    // (i.e. it's the factory of ExecInput)
    friend class ExecInputSet;
    OnDiskExecInput(const fs::path&, bool hardlinked=false);

    fs::path path;
    bool hardlinked;
};
