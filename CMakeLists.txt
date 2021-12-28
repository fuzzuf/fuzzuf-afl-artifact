cmake_minimum_required(VERSION 3.10)
set(
  CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/"
  ${CMAKE_MODULE_PATH}
)

PROJECT(fuzzuf)

execute_process(
  COMMAND git rev-parse HEAD
  OUTPUT_VARIABLE PATTR_COMMIT
)
execute_process(
  COMMAND whoami
  OUTPUT_VARIABLE PATTR_USER
)
set( PATTR_BRANCH "unknown" CACHE STRING "The name of branch the package used" )
set( PATTR_VERSION_MAJOR 0 )
set( PATTR_VERSION_MINOR 1 )
set( PATTR_VERSION_PATCH 0 )
set( PATTR_VERSION_RELEASE 1 CACHE STRING "Release number of package" )
math( EXPR PATTR_VERSION_RELEASE_CALCULATED "${PATTR_VERSION_RELEASE}" )
set( PATTR_DESCRIPTION_SUMMARY "Extensible fuzzing framework" )
set( PATTR_HOMEPAGE_URL "https://github.com/fuzzuf/fuzzuf-afl-artifact" )
set( PATTR_LICENSE "Proprietary" )
set( PATTR_VENDOR "Ricerca Security" )
set( PATTR_DIST "ricsec" )
set( PATTR_MAINTAINER "Ricerca Security <fuzzuf-dev@ricsec.co.jp>" )
set( PATTR_COMPRESSIN_TYPE "xz" )

include(CheckIncludeFiles)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)
find_package(PkgConfig)
enable_testing()
try_compile(
  HAS_CXX_STD_BIT
  ${CMAKE_BINARY_DIR}/check
  SOURCES ${CMAKE_SOURCE_DIR}/check/has_bit.cpp
)
try_compile(
  HAS_CXX_STD_FILESYSTEM
  ${CMAKE_BINARY_DIR}/check
  SOURCES ${CMAKE_SOURCE_DIR}/check/has_filesystem.cpp
)
if( HAS_CXX_STD_FILESYSTEM )
  find_package( Boost 1.53.0 COMPONENTS system unit_test_framework program_options REQUIRED )
else()
  find_package( Boost 1.53.0 COMPONENTS filesystem system unit_test_framework program_options REQUIRED )
endif()
set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package( Threads REQUIRED )
find_package( Python3x REQUIRED )
find_package( Pybind11 REQUIRED )
if( ${CMAKE_VERSION} VERSION_LESS 3.17.0 )
  execute_process(
    COMMAND python3-config --extension-suffix
    OUTPUT_VARIABLE Python3_SOABI
  )
  string( REGEX REPLACE "^\\." "" Python3_SOABI "${Python3_SOABI}" )
  string( REGEX REPLACE "\\.(so|dylib|dll)\n$" "" Python3_SOABI "${Python3_SOABI}" )
endif()
option( ENABLE_DOXYGEN "generate documents" OFF )
if( ENABLE_DOXYGEN )
  find_package( Doxygen OPTIONAL_COMPONENTS dot mscgen dia )
endif()
option( ENABLE_FLC "enable flc" OFF )
if( ENABLE_FLC )
  pkg_check_modules( FLC flc REQUIRED )
else()
  set( FLC_FOUND FALSE )
endif()
find_package( NlohmannJSON REQUIRED )

set( ADDITIONAL_COMPILE_FLAGS )

option( ENABLE_ADDRSAN "build with address sanitizer" OFF)
if( ENABLE_ADDRSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=address" )
  if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    list(APPEND ADDITIONAL_COMPILE_FLAGS "-static-libasan" )
  endif()
endif()
option( ENABLE_PCOMPSAN "build with pointer compare sanitizer" OFF)
if( ENABLE_PCOMPSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=pointer-compare" )
endif()
option( ENABLE_PSUBSAN "build with pointer subtract sanitizer" OFF)
if( ENABLE_PSUBSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=pointer-subtract" )
endif()
option( ENABLE_THREADSAN "build with thread sanitizer" OFF)
if( ENABLE_THREADSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=thread" )
endif()
option( ENABLE_LEAKSAN "build with leak sanitizer" OFF)
if( ENABLE_LEAKSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=leak" )
endif()
option( ENABLE_UBSAN "build with undefined behavior sanitizer" OFF)
if( ENABLE_UBSAN )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsanitize=undefined" )
endif()
option( ENABLE_CXX20 "build with -std=c++20" OFF)
if( ENABLE_CXX20 )
  set( CMAKE_CXX_STANDARD "20" )
else()
  set( CMAKE_CXX_STANDARD "17" )
endif()
option( ENABLE_DANGEROUS_STACK_TRACE_OUTPUT "print backtrace on SIGSEGV" ON )
if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
  if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0 )
    option( ENABLE_STATIC_ANALYZER "enable static analyzer" OFF)
    if( ENABLE_STATIC_ANALYZER )
      list(APPEND ADDITIONAL_COMPILE_FLAGS "-fanalyzer")
    endif()
  endif()
endif()
if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
  list(APPEND ADDITIONAL_COMPILE_FLAGS "-fsized-deallocation")
endif()
option( ENABLE_DISPLAY_LAST_STACK_TRACE "parse and display stacktrace.dump at next execution" OFF )
option( ENABLE_HEAVY_TEST "enable heavy test" ON )
set( DEFAULT_RUNLEVEL "Release" CACHE STRING "runlevel" )
string( TOUPPER "${DEFAULT_RUNLEVEL}" DEFAULT_RUNLEVEL_CAP )

string(REPLACE ";" " " ADDITIONAL_COMPILE_FLAGS_STR "${ADDITIONAL_COMPILE_FLAGS}")
string(REPLACE ";" " " ADDITIONAL_LINK_FLAGS_STR "${ADDITIONAL_LINK_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_RELEASE "-march=native -O3 -Wall -Wextra -Werror")
set(CMAKE_C_FLAGS_RELEASE "-march=native -O3 -Wall -Wextra -Werror")

find_program(LSB_RELEASE_EXEC lsb_release)
if( NOT LSB_RELEASE_EXEC STREQUAL "LSB_RELEASE_EXEC-NOTFOUND" )
  execute_process(COMMAND ${LSB_RELEASE_EXEC} -is
    OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
else()
  set( LSB_RELEASE_ID_SHORT "unknown" )
endif()

configure_file(
  "${CMAKE_SOURCE_DIR}/check/config.h.in"
  "${CMAKE_BINARY_DIR}/config.h"
)

if( ${LSB_RELEASE_ID_SHORT} STREQUAL "Debian" )
  set( CPACK_GENERATOR "DEB" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "Ubuntu" )
  set( CPACK_GENERATOR "DEB" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "RedHatEnterpriseServer" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "Fedora" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "CentOS" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "AmazonAMI" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "openSUSE project" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "SUSE LINUX" )
  set( CPACK_GENERATOR "RPM" )
elseif( ${LSB_RELEASE_ID_SHORT} STREQUAL "Scientific" )
  set( CPACK_GENERATOR "RPM" )
else()
  set( CPACK_GENERATOR "TGZ" )
endif()

set( DEB_REQUIREMENTS )
set( RPM_REQUIREMENTS )

list(APPEND RPM_REQUIREMENTS "python3" )

if( "${PATTR_PYENV}" STREQUAL "native" )
  set( CPACK_PACKAGE_NAME "fuzzuf" )
else()
  set( CPACK_PACKAGE_NAME "fuzzuf_python_${PATTR_PYENV}" )
  set( CPACK_DEBIAN_PACKAGE_PROVIDES "fuzzuf" )
  set( CPACK_RPM_PACKAGE_PROVIDES "fuzzuf" )
  set( CPACK_DEBIAN_PACKAGE_CONFLICTS "fuzzuf" )
  set( CPACK_RPM_PACKAGE_CONFLICTS "fuzzuf" )
endif()
set( CPACK_PACKAGE_VENDOR "${PATTR_VENDOR}" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE" )
set( CPACK_PACKAGE_VERSION_MAJOR "${PATTR_VERSION_MAJOR}" )
set( CPACK_PACKAGE_VERSION_MINOR "${PATTR_VERSION_MINOR}" )
set( CPACK_PACKAGE_VERSION_PATCH "${PATTR_VERSION_PATCH}" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PATTR_DESCRIPTION_SUMMARY}" )
set( CPACK_PACKAGE_DESCRIPTION "Generated by: ${PATTR_USER}\nBranch: ${PATTR_BRANCH}\nCommit: ${PATTR_COMMIT}" )
set( CPACK_PACKAGE_HOMEPAGE_URL "${PATTR_HOMEPAGE_URL}" )
set( CPACK_STRIP_FILES FALSE )

set( CPACK_DEBIAN_PACKAGE_RELEASE "${PATTR_VERSION_RELEASE_CALCULATED}" )
set( CPACK_DEBIAN_FILE_NAME DEB-DEFAULT )
set( CPACK_DEBIAN_PACKAGE_MAINTAINER "${PATTR_MAINTAINER}" )
set( CPACK_DEBIAN_COMPRESSION_TYPE "${PATTR_COMPRESSION_TYPE}" )
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )
string(REPLACE ";" "," DEB_REQUIREMENTS_STR "${DEB_REQUIREMENTS}")
set( CPACK_DEBIAN_PACKAGE_DEPENDS "${DEB_REQUIREMENTS_STR}" )
message("deb requirements: ${DEB_REQUIREMENTS_STR}")

set( CPACK_RPM_PACKAGE_RELEASE "${PATTR_VERSION_RELEASE_CALCULATED}" )
set( CPACK_RPM_PACKAGE_RELEASE_DIST "${PATTR_DIST}" )
set( CPACK_RPM_PACKAGE_LICENSE "${PATTR_LICENSE}" )
set( CPACK_RPM_PACKAGE_URL "${PATTR_HOMEPAGE_URL}" )
set( CPACK_RPM_COMPRESSION_TYPE "${PATTR_COMPRESSION_TYPE}" )
string(REPLACE ";" "," RPM_REQUIREMENTS_STR "${RPM_REQUIREMENTS}")
set( CPACK_DEBIAN_PACKAGE_DEPENDS "${RPM_REQUIREMENTS_STR}" )
message("rpm requirements: ${RPM_REQUIREMENTS_STR}")

set(CPACK_SET_DESTDIR true)

include(CPack)

# アルファベット順に並べてください
set(
  FUZZUF_SOURCES
  Algorithms/AFL/AFLDictData.cpp
  Algorithms/AFL/AFLFuzzer.cpp
  Algorithms/AFL/AFLMutationHierarFlowRoutines.cpp
  Algorithms/AFL/AFLMutator.cpp
  Algorithms/AFL/AFLOtherHierarFlowRoutines.cpp
  Algorithms/AFL/AFLSetting.cpp
  Algorithms/AFL/AFLState.cpp
  Algorithms/AFL/AFLTestcase.cpp
  Algorithms/AFL/AFLUpdateHierarFlowRoutines.cpp
  Algorithms/AFL/AFLUtil.cpp
  Algorithms/libFuzzer/Dictionary.cpp
  ExecInput/ExecInput.cpp
  ExecInput/ExecInputSet.cpp
  ExecInput/OnDiskExecInput.cpp
  ExecInput/OnMemoryExecInput.cpp
  Executor/Executor.cpp
  Executor/NativeLinuxExecutor.cpp
  Executor/PinToolExecutor.cpp
  Feedback/BorrowedFdFeedback.cpp
  Feedback/DisposableFdFeedback.cpp
  Feedback/ExitStatusFeedback.cpp
  Feedback/FileFeedback.cpp
  Feedback/InplaceMemoryFeedback.cpp
  Feedback/PersistentMemoryFeedback.cpp
  Feedback/PUTExitReasonType.cpp
  Logger/Logger.cpp
  Logger/LogFileLogger.cpp
  Logger/StdoutLogger.cpp
  Mutator/Mutator.cpp
  Python/ExportWyvern.cpp
  Python/PyFeedback.cpp
  Python/PySeed.cpp
  Python/PythonFuzzer.cpp
  Python/PythonHierarFlowRoutines.cpp
  Python/PythonSetting.cpp
  Python/PythonState.cpp
  Python/PythonTestcase.cpp
  Utils/Common.cpp
  Utils/HexDump.cpp
  Utils/Workspace.cpp
  Utils/MapFile.cpp
  Utils/Which.cpp
  Utils/IsExecutable.cpp
)

add_library(
  fuzzuf
  SHARED
  ${FUZZUF_SOURCES}
)

# アルファベット順に並べてください
set(
  FUZZUF_CLI_SOURCES
  CLI/Fuzzers/AFL/RegisterAFLFuzzer.cpp
  CLI/ParseGlobalOptionsForFuzzer.cpp
  CLI/CLIEntrypoint.cpp
  CLI/FuzzerBuilderRegister.cpp
  CLI/CommandLineArgs.cpp
)

### CLI tool
add_executable(
  fuzzuf-cli
  ${FUZZUF_CLI_SOURCES}
  ${FUZZUF_SOURCES}
)
set_target_properties(fuzzuf-cli PROPERTIES OUTPUT_NAME "fuzzuf")

### Library of CLI for unit tests
add_library(
  fuzzuf-cli-lib
  SHARED
  ${FUZZUF_CLI_SOURCES}
  ${FUZZUF_SOURCES}
)

set( FUZZUF_LIBRARIES )
set( FUZZUF_INCLUDE_DIRS )

list(APPEND FUZZUF_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/Include" )
list(APPEND FUZZUF_INCLUDE_DIRS "${CMAKE_BINARY_DIR}" )
list(APPEND FUZZUF_INCLUDE_DIRS "${Pybind11_INCLUDE_DIRS}" )
list(APPEND FUZZUF_INCLUDE_DIRS "${Python3_INCLUDE_DIRS}" )
list(APPEND FUZZUF_INCLUDE_DIRS "${NLOHMANN_JSON_INCLUDE_DIRS}" )
list(APPEND FUZZUF_LIBRARIES "${Python3_LIBRARIES}")
list(APPEND FUZZUF_LIBRARIES "${${CMAKE_DL_LIBS}}")
list(APPEND FUZZUF_LIBRARIES "${NLOHMANN_JSON_LIBRARIES}" )
if( ${CMAKE_VERSION} VERSION_LESS 3.15.0 )
  list(APPEND FUZZUF_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}" )
else()
  list(APPEND FUZZUF_LIBRARIES Boost::headers )
endif()
if( NOT HAS_CXX_STD_FILESYSTEM )
  list(APPEND FUZZUF_LIBRARIES Boost::filesystem )
endif()
if( FLC_FOUND )
  list(APPEND FUZZUF_INCLUDE_DIRS "${FLC_INCLUDE_DIRS}" )
  list(APPEND FUZZUF_LIBRARIES "${FLC_LIBRARIES}")
endif()
list(APPEND FUZZUF_LIBRARIES Boost::system )
list(APPEND FUZZUF_LIBRARIES Threads::Threads )
list(APPEND FUZZUF_LIBRARIES ${CMAKE_DL_LIBS} )
list(APPEND FUZZUF_LIBRARIES util )


### Configure dependencies
target_include_directories(
  fuzzuf
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
)
target_link_libraries(
  fuzzuf
  ${FUZZUF_LIBRARIES}
)

target_include_directories(
  fuzzuf-cli
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
)
target_link_libraries(
  fuzzuf-cli
  ${FUZZUF_LIBRARIES}
)

target_link_libraries(
  fuzzuf-cli-lib
  ${FUZZUF_LIBRARIES}
)
target_include_directories(
  fuzzuf-cli-lib
  PRIVATE
  ${FUZZUF_INCLUDE_DIRS}
)

### 
set_target_properties( fuzzuf PROPERTIES PREFIX "" )
set_target_properties( fuzzuf PROPERTIES SUFFIX ".${Python3_SOABI}${CMAKE_SHARED_LIBRARY_SUFFIX}" )
set_target_properties(
  fuzzuf
  PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
set_target_properties(
  fuzzuf
  PROPERTIES LINK_FLAGS "${ADDITIONAL_COMPILE_FLAGS_STR}"
)
install( TARGETS fuzzuf LIBRARY DESTINATION "${Python3_SITELIB}" )
if( "${PATTR_PYENV}" STREQUAL "native" )
  if( DOXYGEN_FOUND )
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    if( ${CMAKE_VERSION} VERSION_LESS 3.12.0 )
      file( GLOB_RECURSE FUZZUF_HEADERS RELATIVE ${CMAKE_SOURCE_DIR} "*.hpp" )
      doxygen_add_docs(
        fuzzuf_doc
        ${FUZZUF_SOURCES}
        ${FUZZUF_HEADERS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    elseif( ${CMAKE_VERSION} VERSION_LESS 3.16.0 )
      doxygen_add_docs(
        fuzzuf_doc
        ALL
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    else()
      doxygen_add_docs(
        fuzzuf_doc
        ALL
        USE_STAMP_FILE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    endif()
    if( ${CMAKE_VERSION} VERSION_LESS 3.14.0 )
      install( DIRECTORY ${CMAKE_BINARY_DIR}/html DESTINATION ${CMAKE_INSTALL_DOCDIR} )
    else()
      install( DIRECTORY ${CMAKE_BINARY_DIR}/html TYPE DOC )
    endif()
  endif()
endif()

subdirs( test tools )
