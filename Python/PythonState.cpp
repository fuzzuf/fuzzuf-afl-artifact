#include "Python/PythonState.hpp"

PythonState::PythonState(const PythonSetting &setting) 
    : setting( setting ),
      input_set() {}

PythonState::~PythonState() {}
