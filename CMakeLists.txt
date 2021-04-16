cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(qbus)

option(CMAKE_EXPORT_COMPILE_COMMANDS                            ON)
option(QBUS_USE_CLANG "use clang"                               OFF)
option(QBUS_EXAMPLES_ENABLED "the examples are enabled"         ON)
option(QBUS_TEST_ENABLED "the tests are enabled"                ON)

if (QBUS_USE_CLANG)
    set(CMAKE_CXX_COMPILER clang++)
    set(CMAKE_C_COMPILER clang)
endif (QBUS_USE_CLANG)

if (CMAKE_EXPORT_COMPILE_COMMANDS)
   add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON) 
endif (CMAKE_EXPORT_COMPILE_COMMANDS)

find_package(Boost COMPONENTS system thread unit_test_framework REQUIRED)
if (Boost_FOUND)
    add_definitions(-DQBUS_BOOST_ENABLED)
    include_directories(${Boost_INCLUDE_DIRS})
endif (Boost_FOUND)

set(CMAKE_CXX_FLAGS "-Wall -Werror")
add_subdirectory(qbus)
if (QBUS_EXAMPLES_ENABLED)
    add_subdirectory(examples)
endif (QBUS_EXAMPLES_ENABLED)
if (Boost_FOUND AND QBUS_TEST_ENABLED)
    enable_testing()
    add_subdirectory(tests)
endif (Boost_FOUND AND QBUS_TEST_ENABLED)
