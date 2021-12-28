#pragma once

#include <memory>
#include <functional>

class Fuzzer {
public:
    virtual ~Fuzzer() {}

    virtual void OneLoop(void) {};

    // do not call non aync-signal-safe functions inside because this function can be called during signal handling
    virtual void ReceiveStopSignal(void) = 0;
};
