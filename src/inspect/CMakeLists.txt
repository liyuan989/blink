set(inspect_SRCS
 Inspector.cpp
 PerformanceInspector.cpp
 ProcessInspector.cpp
 SystemInspector.cpp
 )

add_library(blink_inspect ${inspect_SRCS})
target_link_libraries(blink_inspect blink_http)

if(TCMALLOC_LIBRARY AND TCMALLOC_INCLUDE_DIR)
    set_target_properties(blink_inspect PROPERTIES COMPILE_FLAGS "-DHAVE_TCMALLOC")
    target_link_libraries(blink_inspect tcmalloc_and_profiler)
endif()
