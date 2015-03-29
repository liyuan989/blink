set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/ace/ttcp)

add_executable(ttcp_blink main.cpp common.cpp ttcp.cpp)
target_link_libraries(ttcp_blink blink boost_program_options)

add_executable(ttcp_blocking main.cpp common.cpp ttcp_blocking.cpp)
target_link_libraries(ttcp_blocking blink boost_program_options)
