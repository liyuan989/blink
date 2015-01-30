set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/multiplexer)

add_executable(Demux Demux.cpp)
target_link_libraries(Demux blink)

add_executable(MultiplexServer MultiplexServer.cpp)
target_link_libraries(MultiplexServer blink)

add_executable(MultiplexServer_simple MultiplexServer_simple.cpp)
target_link_libraries(MultiplexServer_simple blink)
