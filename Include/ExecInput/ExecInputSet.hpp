#pragma once

#include <memory>

#include "Utils/Common.hpp"
#include "ExecInput/ExecInput.hpp"
#include "ExecInput/OnDiskExecInput.hpp"
#include "ExecInput/OnMemoryExecInput.hpp"

class ExecInput;
class OnDiskExecInput;
class OnMemoryExecInput;

// TODO: maybe it would be more convenient 
// if we provide OnDiskExecInputSet, OnMemoryExecInputSet, ...

class ExecInputSet {
public:
    template<class Derived, class... Args>
    std::shared_ptr<Derived> CreateInput(Args&&... args) {
        std::shared_ptr<Derived> new_input(new Derived(args...));
        elems[new_input->GetID()] = new_input;
        return new_input;
    }

    template<class... Args>
    std::shared_ptr<OnDiskExecInput> CreateOnDisk(Args&&... args) {
        return CreateInput<OnDiskExecInput>(args...);
    }

    template<class... Args>
    std::shared_ptr<OnMemoryExecInput> CreateOnMemory(Args&&... args) {
        return CreateInput<OnMemoryExecInput>(args...);
    }

    ExecInputSet();
    ~ExecInputSet();

    size_t size(void);
    NullableRef<ExecInput> get_ref(u64 id);
    std::shared_ptr<ExecInput> get_shared(u64 id);
    void erase(u64 id);

    std::vector<u64> get_ids(void);

private:
    // key: input->id, val: input
    std::unordered_map<u64, std::shared_ptr<ExecInput>> elems;
};
