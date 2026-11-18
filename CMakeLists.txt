cmake_minimum_required(VERSION 3.20)
project(kvstore VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Sanitizers: -DKV_SANITIZE=address | undefined | thread | address,undefined
# Applied globally (including GoogleTest) so instrumented and uninstrumented
# code never mix.
# ---------------------------------------------------------------------------
set(KV_SANITIZE "" CACHE STRING "Comma-separated sanitizers: address, undefined, thread")

if(KV_SANITIZE)
  string(REPLACE "," ";" _kv_sans "${KV_SANITIZE}")
  foreach(_san IN LISTS _kv_sans)
    if(NOT _san MATCHES "^(address|undefined|thread)$")
      message(FATAL_ERROR "KV_SANITIZE: unknown sanitizer '${_san}'")
    endif()
  endforeach()
  if("thread" IN_LIST _kv_sans AND "address" IN_LIST _kv_sans)
    message(FATAL_ERROR "KV_SANITIZE: 'thread' cannot be combined with 'address'")
  endif()
  string(REPLACE ";" "," _kv_san_flag "${_kv_sans}")
  add_compile_options(-fsanitize=${_kv_san_flag} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=${_kv_san_flag})
  message(STATUS "Sanitizers enabled: ${_kv_san_flag}")
endif()

# Warning flags for *our* targets only (not third-party code).
add_library(kv_warnings INTERFACE)
target_compile_options(kv_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
add_library(kvstore
  src/kvstore.cc
  src/record.cc
  src/status.cc
)
target_include_directories(kvstore
  PUBLIC  ${PROJECT_SOURCE_DIR}/include
  PRIVATE ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(kvstore PRIVATE kv_warnings)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(KV_BUILD_TESTS "Build tests" ON)

if(KV_BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()
  include(GoogleTest)
  add_subdirectory(tests)
endif()
