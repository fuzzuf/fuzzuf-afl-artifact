#pragma once

#include <functional>
#include "Fuzzer/Fuzzer.hpp"
#include "CLI/FuzzerArgs.hpp"
#include "CLI/GlobalFuzzerOptions.hpp"

// Used only for CLI
using FuzzerBuilder = std::function<std::unique_ptr<Fuzzer>(FuzzerArgs&, GlobalFuzzerOptions&)>;