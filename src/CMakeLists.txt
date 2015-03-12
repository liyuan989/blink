INCLUDE(CheckFunctionExists)

check_function_exists(accept4 HAVE_ACCEPT4)
if(NOT HAVE_ACCEPT4)
    set_source_file_target(Socket.cpp PROPERTIES COMPILE_FLAGS "-DNO_ACCEPT4")
endif()

set(SRCS
 Acceptor.cpp
 AsyncLog.cpp
 Buffer.cpp
 Channel.cpp
 Condition.cpp
 Connector.cpp
 CountDownLatch.cpp
 CurrentThread.cpp
 Date.cpp
 EventLoop.cpp
 EventLoopThread.cpp
 EventLoopThreadPool.cpp
 Exception.cpp
 FileTool.cpp
 InetAddress.cpp
 Log.cpp
 LogFile.cpp
 LogStream.cpp
 MutexLock.cpp
 Poller.cpp
 ProcessBase.cpp
 ProcessInfo.cpp
 Rio.cpp
 Socket.cpp
 SocketBase.cpp
 TcpClient.cpp
 TcpConnection.cpp
 TcpServer.cpp
 Thread.cpp
 ThreadBase.cpp
 ThreadPool.cpp
 Timer.cpp
 TimerQueue.cpp
 Timestamp.cpp
 TimeZone.cpp
 )

add_library(blink ${SRCS})
target_link_libraries(blink pthread rt ${Boost_LIBRARIES})

add_subdirectory(http)
add_subdirectory(inspect)

if(PROTOBUF_FOUND AND ZLIB_FOUND)
    add_subdirectory(protobuf)
else()
    add_subdirectory(protobuf EXCLUDE_FROM_ALL)
endif()
