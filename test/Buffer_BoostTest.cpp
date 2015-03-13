#include <blink/Buffer.h>

#include <fcntl.h>

#include <stdio.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE BufferTest
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

using namespace blink;

BOOST_AUTO_TEST_CASE(testAppendReset)
{
    Buffer buf;
    BOOST_CHECK_EQUAL(buf.readableSize(), 0);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    const string str(200, 'a');
    buf.append(str);
    BOOST_CHECK_EQUAL(buf.readableSize(), str.size());
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - str.size());
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    const string str2 = buf.resetToString(50);
    BOOST_CHECK_EQUAL(str2.size(), 50);
    BOOST_CHECK_EQUAL(buf.readableSize(), str.size() - str2.size());
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - str.size());
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + str2.size());

    buf.append(str);
    BOOST_CHECK_EQUAL(buf.readableSize(), 2 * str.size() - str2.size());
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 2 * str.size());
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + str2.size());

    const string str3 = buf.resetAllToString();
    BOOST_CHECK_EQUAL(str3.size(), 350);
    BOOST_CHECK_EQUAL(buf.readableSize(), 0);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);
}

BOOST_AUTO_TEST_CASE(testBufferGrow)
{
    Buffer buf;
    buf.append(string(400, 'b'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 400);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 400);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(50);
    BOOST_CHECK_EQUAL(buf.readableSize(), 350);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 400);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + 50);

    buf.append(string(1000, 'c'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 1350);
    BOOST_CHECK_EQUAL(buf.writeableSize(), 0);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + 50);

    buf.resetAll();
    BOOST_CHECK_EQUAL(buf.readableSize(), 0);
    BOOST_CHECK_EQUAL(buf.writeableSize(), 1400);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);
}

BOOST_AUTO_TEST_CASE(testBufferInsideGrow)
{
    Buffer buf;
    buf.append(string(800, 'd'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 800);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 800);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(500);
    BOOST_CHECK_EQUAL(buf.readableSize(), 300);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 800);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + 500);

    buf.append(string(300, 'e'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 600);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 600);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);
}

BOOST_AUTO_TEST_CASE(testBufferShrink)
{
    Buffer buf;
    buf.append(string(2000, 'f'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 2000);
    BOOST_CHECK_EQUAL(buf.writeableSize(), 0);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(1500);
    BOOST_CHECK_EQUAL(buf.readableSize(), 500);
    BOOST_CHECK_EQUAL(buf.writeableSize(), 0);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize + 1500);

    buf.shrink(0);
    BOOST_CHECK_EQUAL(buf.readableSize(), 500);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 500);
    BOOST_CHECK_EQUAL(buf.resetAllToString(), string(500, 'f'));
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);
}

BOOST_AUTO_TEST_CASE(testBufferPrepend)
{
    Buffer buf;
    buf.append(string(200, 'g'));
    BOOST_CHECK_EQUAL(buf.readableSize(), 200);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 200);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize);

    int x = 0;
    buf.prepend(&x, sizeof(x));
    BOOST_CHECK_EQUAL(buf.readableSize(), 200 + sizeof(x));
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize - 200);
    BOOST_CHECK_EQUAL(buf.prependableSize(), Buffer::kPrependSize - sizeof(x));
}

BOOST_AUTO_TEST_CASE(testBufferReadInteger)
{
    Buffer buf;
    buf.append("HTTP");

    BOOST_CHECK_EQUAL(buf.readableSize(), 4);
    BOOST_CHECK_EQUAL(buf.peekInt8(), 'H');
    int top16 = buf.peekInt16();
    BOOST_CHECK_EQUAL(top16, 256 * 'H' + 'T');  // little endian
    BOOST_CHECK_EQUAL(buf.peekInt32(), top16 * 65536 + 'T' * 256 + 'P'); // little endian

    BOOST_CHECK_EQUAL(buf.readInt8(), 'H');
    BOOST_CHECK_EQUAL(buf.readInt16(), 256 * 'T' + 'T');
    BOOST_CHECK_EQUAL(buf.readInt8(), 'P');
    BOOST_CHECK_EQUAL(buf.readableSize(), 0);
    BOOST_CHECK_EQUAL(buf.writeableSize(), Buffer::kBufferSize);

    buf.appendInt8(-1);
    buf.appendInt16(-1);
    buf.appendInt32(-1);
    buf.appendInt64(-1);
    BOOST_CHECK_EQUAL(buf.readableSize(), 15);
    BOOST_CHECK_EQUAL(buf.readInt8(), -1);
    BOOST_CHECK_EQUAL(buf.readInt16(), -1);
    BOOST_CHECK_EQUAL(buf.readInt32(), -1);
    BOOST_CHECK_EQUAL(buf.readInt64(), -1);
}

BOOST_AUTO_TEST_CASE(testBufferFindEOL)
{
    Buffer buf;
    buf.append(string(100000, 'k'));
    const char* null = NULL;
    BOOST_CHECK_EQUAL(buf.findEndOfLine(), null);
    BOOST_CHECK_EQUAL(buf.findEndOfLine(buf.peek() + 90000), null);
}

void testFile()
{
    Buffer buffer;
    int fd1 = open("1.txt", O_RDONLY);
    int err1 = 0;
    buffer.readData(fd1, &err1);
    string s1 = buffer.resetAllToString();
    printf("%s\n", s1.c_str());
    printf("%d\n", buffer.bufferCapacity());
    printf("%d\n", buffer.writeableSize());
    printf("%d\n", buffer.readableSize());
}

