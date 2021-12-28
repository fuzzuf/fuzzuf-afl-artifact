#include "Algorithms/AFL/AFLTestcase.hpp"
#include "ExecInput/OnDiskExecInput.hpp"

AFLTestcase::AFLTestcase(std::shared_ptr<OnDiskExecInput> input)
    : input( input ) {}

AFLTestcase::~AFLTestcase() {}
