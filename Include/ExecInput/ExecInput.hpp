#pragma once

#include <memory>

#include "Utils/Common.hpp"

class ExecInput {
public:     
    static constexpr u64 INVALID_INPUT_ID = UINT64_MAX;

    // This class is the base class
    virtual ~ExecInput();

    // disable copies
    ExecInput(const ExecInput&) = delete;
    ExecInput& operator=(const ExecInput&) = delete;

    // allow moves
    ExecInput(ExecInput&&);
    ExecInput& operator=(ExecInput&&);

    virtual void LoadIfNotLoaded(void) = 0;
    virtual void Load(void) = 0; // including reload
    virtual void Unload(void) = 0;
    virtual void Save(void) = 0;
    virtual void OverwriteKeepingLoaded(const u8* buf, u32 len) = 0;
    virtual void OverwriteKeepingLoaded(std::unique_ptr<u8[]>&& buf, u32 len) = 0;
    virtual void OverwriteThenUnload(const u8* buf, u32 len) = 0;
    virtual void OverwriteThenUnload(std::unique_ptr<u8[]>&& buf, u32 len) = 0;

    u8* GetBuf() const; 
    u32 GetLen() const;
    u64 GetID()  const;

protected:
    u64 id;
    // FIXME: using shared instead of unique, only for ease of custom deletion.
    // Probably this is a bad practice.
    std::shared_ptr<u8[]> buf;
    u32 len;

    // ExecInput instances can be created only in ExecInputSet
    // (i.e. it's the factory of ExecInput)
    ExecInput();
    ExecInput(const u8*, u32);
    ExecInput(std::unique_ptr<u8[]>&&, u32);

private:
    static u64 id_counter;
};
