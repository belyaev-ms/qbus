cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(dmux)

option(CMAKE_EXPORT_COMPILE_COMMANDS                            ON)
option(DMUX_USE_CLANG "use clang"                               OFF)
option(DMUX_EXAMPLES_ENABLED "the examples are enabled"         ON)
option(DMUX_TEST_ENABLED "the tests are enabled"                ON)

if (DMUX_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)
endif (DMUX_USE_CLANG)

if (CMAKE_EXPORT_COMPILE_COMMANDS)
   add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON) 
endif (CMAKE_EXPORT_COMPILE_COMMANDS)

find_package(Boost COMPONENTS system thread unit_test_framework REQUIRED)
if (Boost_FOUND)
    add_definitions(-DDMUX_BOOST_ENABLED)
    include_directories(${Boost_INCLUDE_DIRS})
endif (Boost_FOUND)

set(CMAKE_CXX_FLAGS "-Wall -Werror")
add_subdirectory(dmux)
if (DMUX_EXAMPLES_ENABLED)
    add_subdirectory(examples)
endif (DMUX_EXAMPLES_ENABLED)
if (Boost_FOUND AND DMUX_TEST_ENABLED)
    enable_testing()
    add_subdirectory(tests)
endif (Boost_FOUND AND DMUX_TEST_ENABLED)
