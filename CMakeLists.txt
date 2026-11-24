# <CMakeLists.txt> -*- coding: utf-8 -*-
#
# Project shochu
# Copyright (C) 2025-2026 neige68
#

cmake_minimum_required(VERSION 3.28)
set(CMAKE_VERBOSE_MAKEFILE ON)
# static runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(shochu CXX)

# Boost (set CMAKE_PREFIX_PATH when it is not on C:)
# Boost.Log is linked by auto-link; Boost.JSON / Boost.Test are used header-only.
find_package(Boost 1.88 REQUIRED COMPONENTS log)
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# /MP: multi process compile, /W3: warning level 3
# /wd4819: C4819 raised by non-ASCII bytes in some Boost headers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /W3 /wd4819")

add_compile_definitions(NOMINMAX _WIN32_WINNT=0x0602) # Windows 8 or later

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# pure functions (no network): shared with the unit test
add_library(shochu_parse STATIC parse.cpp)

add_executable(shochu
    start.cpp service.cpp config.cpp log.cpp http.cpp mydns.cpp cloudflare.cpp
    resource.rc)
target_precompile_headers(shochu PRIVATE pch.h)
target_link_libraries(shochu shochu_parse winhttp version)

# unit test
enable_testing()
add_executable(test_shochu test/test_shochu.cpp)
target_link_libraries(test_shochu shochu_parse)
add_test(NAME test_shochu COMMAND test_shochu)

# end of <CMakeLists.txt>
