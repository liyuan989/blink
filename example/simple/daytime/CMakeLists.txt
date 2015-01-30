set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/simple/daytime)

add_executable(simple_daytime_client client.cpp)
target_link_libraries(simple_daytime_client blink)

add_executable(simple_daytime_server server.cpp DaytimeServer.cpp)
target_link_libraries(simple_daytime_server blink)
