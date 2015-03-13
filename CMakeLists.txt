cmake_minimum_required(VERSION 2.6)

project(blink)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug")
endif()

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -finline-limit=1000")

set(CMAKE_CXX_FLAGS
-g
-Wconversion
-Wall
-Werror
-Wextra
-Wshadow
-Wno-unused-parameter
-Wold-style-cast
-Woverloaded-virtual
-Wpointer-arith
-Wwrite-strings
-Wreorder
-march=native
-D_FILE_OFFSET_BITS=64
-DCHECK_PTHREAD_RETURN_VALUE
-rdynamic
)

if(CMAKE_BUILD_BITS EQUAL 32)
    list(APPEND CMAKE_CXX_FLAGS "-m32")
endif()

string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if(${CMAKE_BUILD_TYPE} STREQUAL "debug")
    set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/debug)
elseif(${CMAKE_BUILD_TYPE} STREQUAL "release")
    set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib/release)
else()
    message(FATAL_ERROR "Wrong build option! usage: -DCMAKE_BUILD_TYPE=Debug/Release")
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    message(STATUS "found Boost")
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
if(BOOSTTEST_LIBRARY)
    message(STATUS "found boost_unit_test_framework")
endif()

find_library(BOOSTPO_LIBRARY NAMES boost_program_options)
if(BOOSTPO_LIBRARY)
    message(STATUS "found boost_program_options")
endif()

find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)
find_path(TCMALLOC_INCLUDE_DIR gperftools/heap-profiler.h)
if(TCMALLOC_LIBRARY AND TCMALLOC_INCLUDE_DIR)
    message(STATUS "found tcmalloc")
endif()

find_package(GTest)
if(GTEST_FOUND)
    message(STATUS "found Gtest")
    include_directories(${GTEST_INCLUDE_DIRS})
endif()

find_package(Protobuf)
if(PROTOBUF_FOUND)
    message(STATUS "found Protobuf")
    include_directories(${PROTOBUF_INCLUDE_DIRS})
endif()

find_package(ZLIB)
if(ZLIB_FOUND)
    message(STATUS "found Zlib")
    include_directories(${ZLIB_INCLUDE_DIRS})
endif()

message(STATUS "CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
message(STATUS "CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
message(STATUS "LIBRARY_OUTPUT_PATH = " ${LIBRARY_OUTPUT_PATH})

#SET(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(src)

if(NOT CMAKE_BUILD_NO_TEST)
    add_subdirectory(test)
endif()

if(NOT CMAKE_BUILD_NO_EXAMPLES)
    include_directories(${CMAKE_SOURCE_DIR})
    add_subdirectory(example)
endif()
