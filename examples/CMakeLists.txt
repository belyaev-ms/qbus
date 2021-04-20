################################################################################
# Examples
################################################################################
include_directories(..)
add_executable(producer producer.cpp)
target_link_libraries(producer qbus)
add_executable(consumer consumer.cpp)
target_link_libraries(consumer qbus)
add_executable(multi_producer multi_producer.cpp)
target_link_libraries(multi_producer qbus)
add_executable(multi_consumer multi_consumer.cpp)
target_link_libraries(multi_consumer qbus)