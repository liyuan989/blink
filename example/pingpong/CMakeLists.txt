set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/pingpong)

add_executable(pingpong_server server.cpp)
target_link_libraries(pingpong_server blink)

add_executable(pingpong_client client.cpp)
target_link_libraries(pingpong_client blink)

add_executable(BenchMark BenchMark.cpp)
target_link_libraries(BenchMark blink)
