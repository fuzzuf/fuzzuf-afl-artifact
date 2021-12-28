#include <iostream>
#include <string>
#include <pybind11/pybind11.h>
int main() {
  std::cout << PYBIND11_VERSION_MAJOR << '.' << PYBIND11_VERSION_MINOR << '.' << PYBIND11_VERSION_PATCH << std::flush;
}

