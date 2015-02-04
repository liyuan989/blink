set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/shorturl)

add_executable(ShortUrl ShortUrl.cpp)
target_link_libraries(ShortUrl blink)
