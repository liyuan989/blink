set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/filetransfer)

add_executable(Download Download.cpp)
target_link_libraries(Download blink)

add_executable(Download2 Download2.cpp)
target_link_libraries(Download2 blink)

add_executable(Download3 Download3.cpp)
target_link_libraries(Download3 blink)
