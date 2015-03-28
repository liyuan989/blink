set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/ace/logging)

include_directories(${CMAKE_BINARY_DIR})

add_executable(logging_client client.cpp logrecord.pb.cc)
set_target_properties(logging_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(logging_client blink_protobuf)

add_executable(logging_server server.cpp logrecord.pb.cc)
set_target_properties(logging_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(logging_server blink_protobuf)
