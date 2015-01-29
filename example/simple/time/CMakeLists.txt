set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/simple/time)

add_executable(simple_time_client client.cpp)
target_link_libraries(simple_time_client blink)

add_executable(simple_time_server server.cpp TimeServer.cpp)
target_link_libraries(simple_time_server blink)
