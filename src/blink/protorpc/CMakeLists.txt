add_custom_command(OUTPUT rpc.pb.h rpc.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/rpc.proto
 DEPENDS rpc.proto
 VERBATIM )

include_directories(${PROJECT_BINARY_DIR}/src)
set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_library(blink_protorpc_wire rpc.pb.cc RpcCodec.cpp)
set_target_properties(blink_protorpc_wire PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")

add_library(blink_protorpc RpcServer.cpp RpcChannel.cpp)
set_target_properties(blink_protorpc PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(blink_protorpc blink_protorpc_wire blink_protobuf blink protobuf z)

if(TCMALLOC_LIBRARY AND TCMALLOC_INCLUDE_DIR)
    target_link_libraries(blink_protorpc tcmalloc_and_profiler)
endif()

install(TARGETS blink_protorpc_wire DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/protorpc)

