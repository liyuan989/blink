set(http_SRCS
 HttpContext.cpp
 HttpRequest.cpp
 HttpResponse.cpp
 HttpServer.cpp
 )

add_library(blink_http ${http_SRCS})
target_link_libraries(blink_http blink)
