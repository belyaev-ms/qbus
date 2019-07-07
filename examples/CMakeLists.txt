################################################################################
# Examples
################################################################################
include_directories(..)
add_executable(server server.cpp)
target_link_libraries(server dmux)
add_executable(client client.cpp)
target_link_libraries(client dmux)
add_executable(multi_server multi_server.cpp)
target_link_libraries(multi_server dmux)
add_executable(multi_client multi_client.cpp)
target_link_libraries(multi_client dmux)