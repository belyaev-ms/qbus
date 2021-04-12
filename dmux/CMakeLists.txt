################################################################################
# Library the QBUS
################################################################################
include_directories(..)
add_library(dmux STATIC
    common.cpp
    locker.cpp
    queue.cpp
    message.cpp
    connector.cpp
    memory.cpp
)
target_link_libraries(dmux 
    ${Boost_SYSTEM_LIBRARY}
    pthread
)
if (NOT APPLE)
    target_link_libraries(dmux rt)
endif (NOT APPLE)