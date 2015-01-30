set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/simple/echo)

add_executable(simple_echo_client client.cpp EchoClient.cpp)
target_link_libraries(simple_echo_client blink)

add_executable(simple_echo_server server.cpp EchoServer.cpp)
target_link_libraries(simple_echo_server blink)
