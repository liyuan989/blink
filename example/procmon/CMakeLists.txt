set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/procmon)

add_executable(plot_test plot_test.cpp plot.cpp)
target_link_libraries(plot_test blink gd)

add_executable(procmon procmon.cpp plot.cpp)
target_link_libraries(procmon blink_http gd)
