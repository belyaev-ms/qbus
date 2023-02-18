################################################################################
# Logger - server
################################################################################
add_executable(logger logger.cpp)
target_link_libraries(logger qbus)
if (QBUS_ZMQ_ENABLED)
    target_link_libraries(logger zmq)
endif (QBUS_ZMQ_ENABLED)

################################################################################
# Logger - client
################################################################################
add_executable(client client.cpp)
target_link_libraries(client qbus)
if (QBUS_ZMQ_ENABLED)
    target_link_libraries(client zmq)
endif (QBUS_ZMQ_ENABLED)
