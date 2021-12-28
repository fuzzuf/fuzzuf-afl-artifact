# Copyright 2021 Naomasa Matsubayashi
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

find_package( Python3x )
if(NOT Pybind11_ROOT)
  find_path( Pybind11_INCLUDE_DIRS pybind11/pybind11.h PATHS ${Python3_SITELIB}/pybind11/include )
else()
  find_path( Pybind11_INCLUDE_DIRS pybind11/pybind11.h NO_DEFAULT_PATH PATHS ${Pybind11_ROOT}/include )
endif()
try_run(
  GET_PYBIND11_VERSION_EXECUTED
  GET_PYBIND11_VERSION_COMPILED
  ${CMAKE_BINARY_DIR}/check
  ${CMAKE_SOURCE_DIR}/check/get_pybind11_version.cpp
  RUN_OUTPUT_VARIABLE Pybind11_VERSION
  CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Pybind11_INCLUDE_DIRS};${Python3_INCLUDE_DIRS}"
  LINK_LIBRARIES ${Python3_LIBRARIES}
)
if( GET_PYBIND11_VERSION_EXECUTED AND Pybind11_INCLUDE_DIRS )
  set( Pybind11_FOUND TRUE )
else()
  set( Pybind11_FOUND FALSE )
  set( Pybind11_INCLUDE_DIR )
endif()
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Pybind11
  VERSION_VAR Pybind11_VERSION
  REQUIRED_VARS Pybind11_INCLUDE_DIRS
)
