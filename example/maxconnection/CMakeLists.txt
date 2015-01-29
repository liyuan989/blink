set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/maxconnection)

add_executable(MaxConnection MaxConnection.cpp)
target_link_libraries(MaxConnection blink)
