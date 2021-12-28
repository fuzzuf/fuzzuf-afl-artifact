#pragma once

#include <memory>
#include "Python/PythonState.hpp"

#include "Executor/NativeLinuxExecutor.hpp"

#include "Feedback/InplaceMemoryFeedback.hpp"
#include "Feedback/ExitStatusFeedback.hpp"

#include "HierarFlow/HierarFlowRoutine.hpp"
#include "HierarFlow/HierarFlowNode.hpp"
#include "HierarFlow/HierarFlowIntermediates.hpp"

namespace pyfuzz {
namespace pipeline {

using PyMutOutputType = u64(const u8*, u32);
using PyUpdInputType = u64(const u8*, 
                           u32, 
                           ExitStatusFeedback, 
                           InplaceMemoryFeedback&, 
                           InplaceMemoryFeedback&);

struct PyExecutePUT
    : public HierarFlowRoutine<
        PyMutOutputType,
        PyUpdInputType
      > {
public:
    PyExecutePUT(NativeLinuxExecutor &executor);

    NullableRef<HierarFlowCallee<PyMutOutputType>> operator()(const u8*, u32);

private:
    NativeLinuxExecutor &executor;
};

struct PyUpdate
    : public HierarFlowRoutine<
        PyUpdInputType,
        void(void)
      > {
public:
    PyUpdate(PythonState &state);

    NullableRef<HierarFlowCallee<PyUpdInputType>> operator()(
        const u8*,
        u32,
        ExitStatusFeedback,
        InplaceMemoryFeedback&,
        InplaceMemoryFeedback&
    );

private:
    PythonState &state;
};

struct PyBitFlip
    : public HierarFlowRoutine<
        void(u32,u32),
        PyMutOutputType
      > {
public:
    PyBitFlip(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,u32)>> operator()(u32, u32);

private:
    PythonState &state;
};

struct PyByteFlip
    : public HierarFlowRoutine<
        void(u32,u32),
        PyMutOutputType
      > {
public:
    PyByteFlip(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,u32)>> operator()(u32, u32);

private:
    PythonState &state;
};

struct PyHavoc
    : public HierarFlowRoutine<
        void(u32),
        PyMutOutputType
      > {
public:
    PyHavoc(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32)>> operator()(u32);

private:
    PythonState &state;
};

struct PyAdd
    : public HierarFlowRoutine<
        void(u32,int,int,bool),
        PyMutOutputType
      > {
public:
    PyAdd(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,int,int,bool)>> operator()(
        u32, int, int, bool
    );

private:
    PythonState &state;
};

struct PySub
    : public HierarFlowRoutine<
        void(u32,int,int,bool),
        PyMutOutputType
      > {
public:
    PySub(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,int,int,bool)>> operator()(
        u32, int, int, bool
    );

private:
    PythonState &state;
};


struct PyInterest
    : public HierarFlowRoutine<
        void(u32,int,u32,bool),
        PyMutOutputType
      > {
public:
    PyInterest(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,int,u32,bool)>> operator()(
        u32, int, u32, bool
    );

private:
    PythonState &state;
};

struct PyOverwrite
    : public HierarFlowRoutine<
        void(u32,char),
        PyMutOutputType
      > {
public:
    PyOverwrite(PythonState &state);

    NullableRef<HierarFlowCallee<void(u32,char)>>operator()(u32, char);

private:
    PythonState &state;
};

} // namespace pipeline
} // namespace pyfuzz
