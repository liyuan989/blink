set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/asio/timer)

add_executable(timer2 timer2.cpp)
target_link_libraries(timer2 blink)

add_executable(timer3 timer3.cpp)
target_link_libraries(timer3 blink)

add_executable(timer4 timer4.cpp)
target_link_libraries(timer4 blink)

add_executable(timer5 timer5.cpp)
target_link_libraries(timer5 blink)

add_executable(timer6 timer6.cpp)
target_link_libraries(timer6 blink)
