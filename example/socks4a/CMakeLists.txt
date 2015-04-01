set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/socks4a)

add_executable(tcprelay tcprelay.cpp)
target_link_libraries(tcprelay blink)

add_executable(socks4a socks4a.cpp)
target_link_libraries(socks4a blink)

add_executable(balancer balancer.cpp)
target_link_libraries(balancer blink)
