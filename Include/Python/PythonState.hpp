#pragma once

#include <memory>
#include <unordered_map>

#include "ExecInput/ExecInputSet.hpp"
#include "Mutator/Mutator.hpp"
#include "Feedback/PersistentMemoryFeedback.hpp"
#include "Python/PythonSetting.hpp"
#include "Python/PythonTestcase.hpp"

struct PythonState {
    explicit PythonState(const PythonSetting &setting);
    ~PythonState();

    const PythonSetting &setting;
    ExecInputSet input_set;
    std::unordered_map<u64, std::unique_ptr<PythonTestcase>> test_set;
    std::unique_ptr<Mutator> mutator;
};
