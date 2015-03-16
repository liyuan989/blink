set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/protobuf/rpcbalancer)

add_executable(rpc_balancer balancer.cpp)
set_target_properties(rpc_balancer PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(rpc_balancer blink_protorpc)

add_executable(rpc_balancer_raw balancer_raw.cpp)
set_target_properties(rpc_balancer_raw PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
target_link_libraries(rpc_balancer_raw blink_protorpc)
