set(http_SRCS
 HttpContext.cpp
 HttpRequest.cpp
 HttpResponse.cpp
 HttpServer.cpp
 )

add_library(blink_http ${http_SRCS})
target_link_libraries(blink_http blink)

install(TARGETS blink_http DESTINATION lib)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/http)
