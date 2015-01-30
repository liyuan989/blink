set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/idleconnection)

add_executable(EchoServer main.cpp EchoServer.cpp)
target_link_libraries(EchoServer blink)

add_executable(EchoServer_sorted EchoServer_sorted.cpp)
target_link_libraries(EchoServer_sorted blink)
