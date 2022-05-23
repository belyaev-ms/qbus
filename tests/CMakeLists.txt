add_definitions(-DQBUS_TEST_ENABLED)
################################################################################
# Coverage
################################################################################
set(QBUS_COVERAGE_ENABLED yes)
if (QBUS_COVERAGE_ENABLED)
    include(../cmake/coverage.cmake)
else (QBUS_COVERAGE_ENABLED)
    set(CMAKE_CXX_FLAGS "-g3")
endif (QBUS_COVERAGE_ENABLED)
################################################################################
# Test library the QBUS
################################################################################
add_library(qbus_test STATIC
    ../qbus/common.cpp
    ../qbus/locker.cpp
    ../qbus/queue.cpp
    ../qbus/message.cpp
    ../qbus/connector.cpp
    ../qbus/bus.cpp
    ../qbus/memory.cpp
)
target_link_libraries(qbus_test 
    ${Boost_SYSTEM_LIBRARY}
    pthread
)
if (NOT APPLE)
    target_link_libraries(qbus_test rt)
endif (NOT APPLE)
################################################################################
# Unit-tests
################################################################################
#macros for add an tool
macro(qbus_add_tool tool)
    add_executable(${tool} ${tool}.cpp)
    target_link_libraries(${tool} qbus_test)
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
        qbus_test
    )
    add_test(${test} ${test})
endmacro(qbus_add_test)
include_directories(..)
qbus_add_tool(test_connector_producer)
qbus_add_tool(test_connector_consumer)
qbus_add_tool(test_bus_producer)
qbus_add_tool(test_bus_consumer)
qbus_add_test(queue_test)
qbus_add_test(shared_queue_test)
qbus_add_test(unreadable_shared_queue_test)
qbus_add_test(connector_test)
qbus_add_test(bus_test)
qbus_add_test(ipc_connector_test_1)
qbus_add_test(ipc_connector_test_2)
qbus_add_test(ipc_connector_test_3)
qbus_add_test(ipc_connector_test_4)
qbus_add_test(ipc_connector_test_5)
qbus_add_test(ipc_bus_test_1)
if (QBUS_COVERAGE_ENABLED)
    qbus_coverage(coverage)
endif (QBUS_COVERAGE_ENABLED)

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