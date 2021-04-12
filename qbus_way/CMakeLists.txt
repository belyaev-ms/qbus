################################################################################
# Library the QBUS
################################################################################
include_directories(..)
add_library(qbus_way STATIC
    queue.cpp
    message.cpp
)
target_link_libraries(qbus_way 
    ${Boost_SYSTEM_LIBRARY}
    pthread
)
if (NOT APPLE)
    target_link_libraries(qbus_way rt)
endif (NOT APPLE)