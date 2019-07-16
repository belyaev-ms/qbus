################################################################################
# Coverage
################################################################################
set(DMUX_COVERAGE_ENABLED yes)
if (DMUX_COVERAGE_ENABLED)
    include(../cmake/coverage.cmake)
else (DMUX_COVERAGE_ENABLED)
    set(CMAKE_CXX_FLAGS "-g3")
endif (DMUX_COVERAGE_ENABLED)

################################################################################
# Unit-tests
################################################################################
add_definitions(-DDMUX_TEST_ENABLED)
#macros for add an tool
macro(dmux_add_tool tool)
    add_executable(${tool} ${tool}.cpp)
    target_link_libraries(${tool} dmux)
endmacro(dmux_add_tool)
#macros for add an unit-test
macro(dmux_add_test test)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} 
        ${Boost_FILESYSTEM_LIBRARY} 
        ${Boost_SYSTEM_LIBRARY} 
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
        ${Boost_THREAD_LIBRARY} 
        pthread
        dmux
    )
    add_test(${test} ${test})
endmacro(dmux_add_test)
include_directories(..)
dmux_add_tool(test_server)
dmux_add_tool(test_client)
dmux_add_test(queue_test)
dmux_add_test(shared_queue_test)
dmux_add_test(connector_test)
dmux_add_test(ipc_test)
if (DMUX_COVERAGE_ENABLED)
    dmux_coverage(coverage)
endif (DMUX_COVERAGE_ENABLED)

################################################################################
# Static code analysis
################################################################################
add_custom_target(
    cppcheck
    COMMAND cppcheck
    --enable=all
    --template=gcc
    --project=${CMAKE_BINARY_DIR}/compile_commands.json
)

add_custom_target(
    clang-tidy
    COMMAND run-clang-tidy-3.8.py
)