set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/netty/uptime)

add_executable(netty_uptime_client client.cpp)
target_link_libraries(netty_uptime_client blink)
