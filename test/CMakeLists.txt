set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/test)

add_executable(AcceptorTest AcceptorTest.cpp)
target_link_libraries(AcceptorTest blink)

add_executable(AsyncLogTest AsyncLogTest.cpp)
target_link_libraries(AsyncLogTest blink)

add_executable(AtomicTest AtomicTest.cpp)
target_link_libraries(AtomicTest blink)

add_executable(BlockingQueueTest BlockingQueueTest.cpp)
target_link_libraries(BlockingQueueTest blink)

add_executable(BoundedBlockingQueueTest BoundedBlockingQueueTest.cpp)
target_link_libraries(BoundedBlockingQueueTest blink)

if(BOOSTTEST_LIBRARY)
    add_executable(Buffer_BoostTest Buffer_BoostTest.cpp)
    target_link_libraries(Buffer_BoostTest blink boost_unit_test_framework)
endif()

if(GTEST_FOUND)
    add_executable(Buffer_GoogleTest Buffer_GoogleTest.cpp)
    target_link_libraries(Buffer_GoogleTest blink gtest)
endif()

add_executable(CountDownLatchTest CountDownLatchTest.cpp)
target_link_libraries(CountDownLatchTest blink)

add_executable(DateTest DateTest.cpp)
target_link_libraries(DateTest blink)

add_executable(echoclient_test echoclient_test.cpp)
target_link_libraries(echoclient_test blink)

add_executable(echoserver_test echoserver_test.cpp)
target_link_libraries(echoserver_test blink)

add_executable(EventLoopTest EventLoopTest.cpp)
target_link_libraries(EventLoopTest blink)

add_executable(EventLoopThreadTest EventLoopThreadTest.cpp)
target_link_libraries(EventLoopThreadTest blink)

add_executable(EventLoopThreadPoolTest EventLoopThreadPoolTest.cpp)
target_link_libraries(EventLoopThreadPoolTest blink)

add_executable(ExceptionTest ExceptionTest.cpp)
target_link_libraries(ExceptionTest blink)

add_executable(FileToolTest FileToolTest.cpp)
target_link_libraries(FileToolTest blink)

if(GTEST_FOUND)
    add_executable(HttpRequest_unittest HttpRequest_unittest.cpp)
    target_link_libraries(HttpRequest_unittest blink_http gtest)
endif()

add_executable(HttpServerTest HttpServerTest.cpp)
target_link_libraries(HttpServerTest blink_http)

add_executable(InetAddressTest InetAddressTest.cpp)
target_link_libraries(InetAddressTest blink)

add_executable(InspectorTest InspectorTest.cpp)
target_link_libraries(InspectorTest blink_inspect)

add_executable(LogFileTest LogFileTest.cpp)
target_link_libraries(LogFileTest blink)

add_executable(LogStreamTest LogStreamTest.cpp)
target_link_libraries(LogStreamTest blink)

add_executable(LogTest LogTest.cpp)
target_link_libraries(LogTest blink)

add_executable(ProcessInfoTest ProcessInfoTest.cpp)
target_link_libraries(ProcessInfoTest blink)

if(PROTOBUF_FOUND AND ZLIB_FOUND)
    add_executable(RpcCodecTest RpcCodecTest.cpp)
    set_target_properties(RpcCodecTest PROPERTIES COMPILE_FLAGS "-Wno-error=shadow")
    target_link_libraries(RpcCodecTest blink_protorpc_wire blink_protobuf_codec)
endif()

add_executable(SingletonTest SingletonTest.cpp)
target_link_libraries(SingletonTest blink)

if(GTEST_FOUND)
    add_executable(StringPieceTest StringPieceTest.cpp)
    target_link_libraries(StringPieceTest blink ${GTEST_BOTH_LIBRARIES})
endif()

add_executable(TcpClientTest TcpClientTest.cpp)
target_link_libraries(TcpClientTest blink)

add_executable(TcpServerTest TcpServerTest.cpp)
target_link_libraries(TcpServerTest blink)

add_executable(ThreadLocalSingletonTest ThreadLocalSingletonTest.cpp)
target_link_libraries(ThreadLocalSingletonTest blink)

add_executable(ThreadLocalTest ThreadLocalTest.cpp)
target_link_libraries(ThreadLocalTest blink)

add_executable(ThreadPoolTest ThreadPoolTest.cpp)
target_link_libraries(ThreadPoolTest blink)

add_executable(ThreadTest ThreadTest.cpp)
target_link_libraries(ThreadTest blink)

add_executable(TimerQueueTest TimerQueueTest.cpp)
target_link_libraries(TimerQueueTest blink)

add_executable(TimestampTest TimestampTest.cpp)
target_link_libraries(TimestampTest blink)

add_executable(TimeZoneTest TimeZoneTest.cpp)
target_link_libraries(TimeZoneTest blink)

add_executable(WeakCallbackTest WeakCallbackTest.cpp)
target_link_libraries(WeakCallbackTest blink)
