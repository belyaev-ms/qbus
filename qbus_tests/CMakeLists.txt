################################################################################
# Coverage
################################################################################
#set(QBUS_COVERAGE_ENABLED yes)
if (QBUS_COVERAGE_ENABLED)
    include(../cmake/coverage.cmake)
else (QBUS_COVERAGE_ENABLED)
    set(CMAKE_CXX_FLAGS "-g3")
endif (QBUS_COVERAGE_ENABLED)

################################################################################
# Unit-tests
################################################################################
add_definitions(-DQBUS_TEST_ENABLED)
#macros for add an tool
macro(qbus_add_tool tool)
    add_executable(${tool} ${tool}.cpp)
    target_link_libraries(${tool} qbus)
endmacro(qbus_add_tool)
#macros for add an unit-test
macro(qbus_add_test test)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} 
        ${Boost_FILESYSTEM_LIBRARY} 
        ${Boost_SYSTEM_LIBRARY} 
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_THREAD_LIBRARY} 
        pthread
        qbus
    )
    add_test(${test} ${test})
endmacro(qbus_add_test)
include_directories(..)
qbus_add_test(qbus_queue_test)
if (QBUS_COVERAGE_ENABLED)
    qbus_coverage(coverage)
endif (QBUS_COVERAGE_ENABLED)

################################################################################
# Static code analysis
################################################################################
#add_custom_target(
#    cppcheck
#    COMMAND cppcheck
#    --enable=all
#    --template=gcc
#    --project=${CMAKE_BINARY_DIR}/compile_commands.json
#)
#
#add_custom_target(
#    clang-tidy
#    COMMAND run-clang-tidy-3.8.py
#)