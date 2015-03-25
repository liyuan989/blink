set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/hiredis)

add_executable(mredis_client mredis_client.cpp Hiredis.cpp)
target_link_libraries(mredis_client blink hiredis)
