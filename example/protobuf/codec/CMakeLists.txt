set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/protobuf/codec)

add_custom_command(OUTPUT query.pb.h query.pb.cc
 COMMAND protoc
 ARGS -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=. ${CMAKE_CURRENT_SOURCE_DIR}/query.proto
 DEPENDS query.proto
 VERBATIM)

set_source_files_properties(query.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion -Wno-shadow")

add_executable(codec_test codec_test.cpp codec.cpp query.pb.cc)
set_target_properties(codec_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(codec_test blink protobuf z)

add_executable(codec_dispatcher_test dispatcher_test.cpp query.pb.cc)
set_target_properties(codec_dispatcher_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(codec_dispatcher_test blink protobuf)

add_executable(codec_dispatcher_lite_test dispatcher_lite_test.cpp query.pb.cc)
set_target_properties(codec_dispatcher_lite_test PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(codec_dispatcher_lite_test blink protobuf)

add_executable(codec_client client.cpp codec.cpp query.pb.cc)
set_target_properties(codec_client PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(codec_client blink protobuf z)

add_executable(codec_server server.cpp codec.cpp query.pb.cc)
set_target_properties(codec_server PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(codec_server blink protobuf z)
