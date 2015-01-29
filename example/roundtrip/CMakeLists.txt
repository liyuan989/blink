set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/roundtrip)

add_executable(Roundtrip Roundtrip.cpp)
target_link_libraries(Roundtrip blink)

add_executable(RoundtripUdp RoundtripUdp.cpp)
target_link_libraries(RoundtripUdp blink)
