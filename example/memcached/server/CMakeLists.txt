set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/memcached/server)

if(BOOSTPO_LIBRARY)
    add_executable(memcached_server server.cpp MemcacheServer.cpp Session.cpp Item.cpp)
    target_link_libraries(memcached_server blink_inspect boost_program_options)
endif()

add_executable(footprint_test footprint_test.cpp MemcacheServer.cpp Session.cpp Item.cpp)
target_link_libraries(footprint_test blink_inspect)

if(TCMALLOC_LIBRARY AND TCMALLOC_INCLUDE_DIR)
    set_target_properties(footprint_test PROPERTIES COMPILE_FLAGS "-DHAVE_TCMALLOC")
    if(BOOSTPO_LIBRARY)
        set_target_properties(memcached_server PROPERTIES COMPILE_FLAHS "-DHAVE_TCMALLOC")
    endif()
endif()
