add_library(blink_protobuf ProtobufCodecLite.cpp)
target_link_libraries(blink_protobuf blink protobuf z)

install(TARGETS blink_protobuf DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/protobuf)
