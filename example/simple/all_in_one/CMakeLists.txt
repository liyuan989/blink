set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/simple/all_in_one)

add_executable(all_in_one
main.cpp
 ../chargen/ChargenServer.cpp
 ../daytime/DaytimeServer.cpp
 ../discard/DiscardServer.cpp
 ../echo/EchoServer.cpp
 ../time/TimeServer.cpp
 )
target_link_libraries(all_in_one blink)
