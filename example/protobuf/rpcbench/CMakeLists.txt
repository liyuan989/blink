set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/protobuf/rpcbench)

add_custom_command(OUTPUT echo.pb.h echo.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/echo.proto
 DEPENDS echo.proto
 VERBATIM)

set_source_files_properties(echo.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")

add_executable(protobuf_rpcbench_client client.cpp echo.pb.cc)
set_target_properties(protobuf_rpcbench_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_rpcbench_client blink_protorpc protobuf)

add_executable(protobuf_rpcbench_server server.cpp echo.pb.cc)
set_target_properties(protobuf_rpcbench_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_rpcbench_server blink_protorpc protobuf)
