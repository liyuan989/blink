set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/netty/dicard)

add_executable(client client.cpp)
target_link_libraries(client blink)

add_executable(server server.cpp)
target_link_libraries(server blink)
