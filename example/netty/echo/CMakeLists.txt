set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/netty/echo)

add_executable(netty_echo_client client.cpp)
target_link_libraries(netty_echo_client blink)

add_executable(netty_echo_server server.cpp)
target_link_libraries(netty_echo_server blink)

add_executable(netty_echo_server2 server2.cpp)
target_link_libraries(netty_echo_server2 blink)
