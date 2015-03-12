add_library(blink_protobuf_codec ProtobufCodecLite.cpp)
target_link_libraries(blink_protobuf_codec blink ${PROTOBUF_LIBRARIES} ${ZLIB_LIBRARIES})
