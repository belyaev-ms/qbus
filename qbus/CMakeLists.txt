################################################################################
# Library the QBUS
################################################################################
include_directories(..)
add_library(qbus STATIC
    common.cpp
    locker.cpp
    queue.cpp
    message.cpp
    connector.cpp
    bus.cpp
    memory.cpp
)
target_link_libraries(qbus 
    ${Boost_SYSTEM_LIBRARY}
    pthread
)
if (NOT APPLE)
    target_link_libraries(qbus rt)
endif (NOT APPLE)