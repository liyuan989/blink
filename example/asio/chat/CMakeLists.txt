set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/asio/chat)

add_executable(ChatClient ChatClient.cpp)
target_link_libraries(ChatClient blink)

add_executable(ChatClientLoadTest ChatClientLoadTest.cpp)
target_link_libraries(ChatClientLoadTest blink)

add_executable(ChatServer ChatServer.cpp)
target_link_libraries(ChatServer blink)

add_executable(ChatServerEfficient ChatServerEfficient.cpp)
target_link_libraries(ChatServerEfficient blink)

add_executable(ChatServerHighPerformance ChatServerHighPerformance.cpp)
target_link_libraries(ChatServerHighPerformance blink)

add_executable(ChatServerThreaded ChatServerThreaded.cpp)
target_link_libraries(ChatServerThreaded blink)
