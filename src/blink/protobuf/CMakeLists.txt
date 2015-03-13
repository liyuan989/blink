add_library(blink_protobuf_codec ProtobufCodecLite.cpp)
target_link_libraries(blink_protobuf_codec blink protobuf z)

install(TARGETS blink_protobuf_codec DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/protobuf)
