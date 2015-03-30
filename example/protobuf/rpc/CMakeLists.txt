set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/protobuf/rpc)

add_custom_command(OUTPUT sudoku.pb.h sudoku.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/sudoku.proto
 DEPENDS sudoku.proto
 VERBATIM)

set_source_files_properties(sudoku.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")

add_executable(protobuf_rpc_client client.cpp sudoku.pb.cc)
set_target_properties(protobuf_rpc_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_rpc_client blink_protorpc protobuf)

add_executable(protobuf_rpc_server server.cpp sudoku.pb.cc)
set_target_properties(protobuf_rpc_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_rpc_server blink_protorpc protobuf)
