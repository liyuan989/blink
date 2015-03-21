set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/protobuf/resolver)

add_custom_command(OUTPUT resolver.pb.h resolver.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/resolver.proto
 DEPENDS resolver.proto
 VERBATIM)

add_executable(protobuf_resolver_client client.cpp resolver.pb.cc)
set_target_properties(protobuf_resolver_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_resolver_client blink_protorpc protobuf)

add_executable(protobuf_resolver_server server.cpp resolver.pb.cc)
set_target_properties(protobuf_resolver_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(protobuf_resolver_server blink_protorpc protobuf)
