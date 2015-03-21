set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/wordcount)

add_executable(wordcount_sender hash.cpp)
target_link_libraries(wordcount_sender blink)

add_executable(wordcount_receiver receiver.cpp)
target_link_libraries(wordcount_receiver blink)
