set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/zeromq)

add_executable(zeromq_client remote_lat.cpp)
target_link_libraries(zeromq_client blink)

add_executable(zeromq_server local_lat.cpp)
target_link_libraries(zeromq_server blink)
