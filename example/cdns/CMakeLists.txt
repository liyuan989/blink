set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/cdns)

add_executable(dns dns.cpp Resolver.cpp)
target_link_libraries(dns blink cares)
