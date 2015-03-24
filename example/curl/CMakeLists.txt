set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/curl)

add_executable(mcurl mcurl.cpp Curl.cpp)
target_link_libraries(mcurl blink curl)

add_executable(download download.cpp Curl.cpp)
target_link_libraries(download blink curl)
