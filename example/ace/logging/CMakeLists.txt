set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/ace/logging)

include_directories(${CMAKE_BINARY_DIR})

add_custom_command(OUTPUT logrecord.pb.h logrecord.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/logrecord.proto
 DEPENDS logrecord.proto
 VERBATIM)

add_executable(logging_client client.cpp logrecord.pb.cc)
set_target_properties(logging_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow -Wno-conversion")
target_link_libraries(logging_client blink_protobuf)

add_executable(logging_server server.cpp logrecord.pb.cc)
set_target_properties(logging_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow -Wno-conversion")
target_link_libraries(logging_server blink_protobuf)
