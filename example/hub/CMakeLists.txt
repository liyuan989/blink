set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/hub)

add_executable(Pub Pub.cpp PubSubClient.cpp Codec.cpp)
target_link_libraries(Pub blink)

add_executable(Sub Sub.cpp PubSubClient.cpp Codec.cpp)
target_link_libraries(Sub blink)

add_executable(PubSubServer PubSubServer.cpp Codec.cpp)
target_link_libraries(PubSubServer blink)
