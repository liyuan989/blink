set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/fastcgi)

add_executable(FastCgiTest FastCgiTest.cpp FastCgi.cpp)
target_link_libraries(FastCgiTest blink)
