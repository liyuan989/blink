set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/twisted/finger)

add_executable(finger1 finger1.cpp)
target_link_libraries(finger1 blink)

add_executable(finger2 finger2.cpp)
target_link_libraries(finger2 blink)

add_executable(finger3 finger3.cpp)
target_link_libraries(finger3 blink)

add_executable(finger4 finger4.cpp)
target_link_libraries(finger4 blink)

add_executable(finger5 finger5.cpp)
target_link_libraries(finger5 blink)

add_executable(finger6 finger6.cpp)
target_link_libraries(finger6 blink)

add_executable(finger7 finger7.cpp)
target_link_libraries(finger7 blink)
