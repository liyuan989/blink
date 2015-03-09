set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/memcached/server)

if(BOOSTPO_LIBRARY)
    add_executable(memcached_server server.cpp MemcacheServer.cpp Session.cpp Item.cpp)
    target_link_libraries(memcached_server blink_inspect boost_program_options)
endif()
