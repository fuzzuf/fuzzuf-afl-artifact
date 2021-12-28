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

find_package( Threads REQUIRED )
if( NOT Python3_FOUND )
  execute_process(
    COMMAND pyenv version-name
    OUTPUT_VARIABLE PATTR_PYENV
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ENCODING AUTO
  )
  if( "${PATTR_PYENV}" STREQUAL "system" )
    set( PATTR_PYENV "native" )
  elseif( "${PATTR_PYENV}" STREQUAL "" )
    set( PATTR_PYENV "native" )
  endif()
  string(REGEX REPLACE [[\.[0-9]+$]] [[]] PATTR_PYENV_SHORT "${PATTR_PYENV}")
  if( ${CMAKE_VERSION} VERSION_LESS 3.12.0 )
    if( "${PATTR_PYENV}" STREQUAL "native" )
      find_package( PythonLibs 3 REQUIRED )
    else()
      execute_process(
        COMMAND python3-config --prefix
        OUTPUT_VARIABLE Python3_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ENCODING AUTO
      )
      execute_process(
        COMMAND python3-config --abiflags
        OUTPUT_VARIABLE Python3_ABIFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ENCODING AUTO
      )
      set( PYTHON_LIBRARY_CANDIDATE "${Python3_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}python${PATTR_PYENV_SHORT}${Python3_ABIFLAGS}${CMAKE_SHARED_LIBRARY_SUFFIX}" )
      if( EXISTS "${PYTHON_LIBRARY_CANDIDATE}" )
        set( PYTHON_LIBRARY "${Python3_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}python${PATTR_PYENV_SHORT}${Python3_ABIFLAGS}${CMAKE_SHARED_LIBRARY_SUFFIX}" )
      else()
        set( PYTHON_LIBRARY "${Python3_PREFIX}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}python${PATTR_PYENV_SHORT}${Python3_ABIFLAGS}${CMAKE_STATIC_LIBRARY_SUFFIX}" )
      endif()
      set( PYTHON_INCLUDE_DIR "${Python3_PREFIX}/include/python${PATTR_PYENV_SHORT}${Python3_ABIFLAGS}" )
      find_package( PythonLibs ${PATTR_PYENV_SHORT} )
    endif()
    set( Python3_INCLUDE_DIRS "${PYTHON_INCLUDE_DIR}" )
    set( Python3_LIBRARIES "${PYTHON_LIBRARY}" )
    set( Python3_FOUND True )
  else()
    if( "${PATTR_PYENV}" STREQUAL "native" )
      find_package( Python3 COMPONENTS Development )
    else()
      find_package( Python3 ${PATTR_PYENV_SHORT} EXACT COMPONENTS Development )
    endif()
  endif()
  try_run(
    GET_SITEDIR_EXECUTED
    GET_SITEDIR_COMPILED
    ${CMAKE_BINARY_DIR}/check
    ${CMAKE_SOURCE_DIR}/check/get_sitedir.cpp
    RUN_OUTPUT_VARIABLE Python3_SITELIB
    COMPILE_OUTPUT_VARIABLE Python3_SITELIB_COMPILER_OUTPUT
    CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${Python3_INCLUDE_DIRS}"
    LINK_LIBRARIES ${Python3_LIBRARIES} ${CMAKE_DL_LIBS} util Threads::Threads
  )
  if( GET_SITEDIR_COMPILED )
    message( "Python Site: ${Python3_SITELIB}" )
  else()
    message( FATAL_ERROR "Unable to detect Python site dir: ${Python3_SITELIB_COMPILER_OUTPUT}" )
  endif()
endif()
