set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/simple/chargen)

add_executable(simple_chargen_client client.cpp ChargenClient.cpp)
target_link_libraries(simple_chargen_client blink)

add_executable(simple_chargen_server server.cpp ChargenServer.cpp)
target_link_libraries(simple_chargen_server blink)
