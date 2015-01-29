set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/simple/discard)

add_executable(simple_discard_client client.cpp)
target_link_libraries(simple_discard_client blink)

add_executable(simple_discard_server server.cpp DiscardServer.cpp)
target_link_libraries(simple_discard_server blink)
