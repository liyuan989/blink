include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/src)

add_subdirectory(resolver)
add_subdirectory(rpc)
add_subdirectory(rpcbench)
add_subdirectory(rpcbalancer)

if(PROTOBUF_FOUND AND ZLIB_FOUND)
    add_subdirectory(codec)
else()
    add_subdirectory(codec EXCLUDE_FROM_ALL)
endif()
