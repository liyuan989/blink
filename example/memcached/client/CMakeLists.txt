set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/memcached/client)

if(BOOSTPO_LIBRARY)
    add_executable(memcached_client client.cpp)
    target_link_libraries(memcached_client blink boost_program_options)
endif()
