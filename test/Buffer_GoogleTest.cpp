#include <blink/Buffer.h>

#include <gtest/gtest.h>

#include <fcntl.h>

#include <stdio.h>

using namespace blink;

TEST(BufferTest, testAppendReset)
{
    Buffer buf;
    EXPECT_EQ(buf.readableSize(), 0);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    const string str(200, 'a');
    buf.append(str);
    EXPECT_EQ(buf.readableSize(), str.size());
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - str.size());
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    const string str2 = buf.resetToString(50);
    EXPECT_EQ(str2.size(), 50);
    EXPECT_EQ(buf.readableSize(), str.size() - str2.size());
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - str.size());
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + str2.size());

    buf.append(str);
    EXPECT_EQ(buf.readableSize(), 2 * str.size() - str2.size());
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 2 * str.size());
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + str2.size());

    const string str3 = buf.resetAllToString();
    EXPECT_EQ(str3.size(), 350);
    EXPECT_EQ(buf.readableSize(), 0);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);
}

TEST(BufferTest, testBufferGrow)
{
    Buffer buf;
    buf.append(string(400, 'b'));
    EXPECT_EQ(buf.readableSize(), 400);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 400);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(50);
    EXPECT_EQ(buf.readableSize(), 350);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 400);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + 50);

    buf.append(string(1000, 'c'));
    EXPECT_EQ(buf.readableSize(), 1350);
    EXPECT_EQ(buf.writeableSize(), 0);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + 50);

    buf.resetAll();
    EXPECT_EQ(buf.readableSize(), 0);
    EXPECT_EQ(buf.writeableSize(), 1400);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);
}

TEST(BufferTest, testBufferInsideGrow)
{
    Buffer buf;
    buf.append(string(800, 'd'));
    EXPECT_EQ(buf.readableSize(), 800);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 800);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(500);
    EXPECT_EQ(buf.readableSize(), 300);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 800);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + 500);

    buf.append(string(300, 'e'));
    EXPECT_EQ(buf.readableSize(), 600);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 600);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);
}

TEST(BufferTest, testBufferShrink)
{
    Buffer buf;
    buf.append(string(2000, 'f'));
    EXPECT_EQ(buf.readableSize(), 2000);
    EXPECT_EQ(buf.writeableSize(), 0);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    buf.reset(1500);
    EXPECT_EQ(buf.readableSize(), 500);
    EXPECT_EQ(buf.writeableSize(), 0);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize + 1500);

    buf.shrink(0);
    EXPECT_EQ(buf.readableSize(), 500);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 500);
    EXPECT_EQ(buf.resetAllToString(), string(500, 'f'));
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);
}

TEST(BufferTest, testBufferPrepend)
{
    Buffer buf;
    buf.append(string(200, 'g'));
    EXPECT_EQ(buf.readableSize(), 200);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 200);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize);

    int x = 0;
    buf.prepend(&x, sizeof(x));
    EXPECT_EQ(buf.readableSize(), 200 + sizeof(x));
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize - 200);
    EXPECT_EQ(buf.prependableSize(), Buffer::kPrependSize - sizeof(x));
}

TEST(BufferTest, testBufferReadInteger)
{
    Buffer buf;
    buf.append("HTTP");

    EXPECT_EQ(buf.readableSize(), 4);
    EXPECT_EQ(buf.peekInt8(), 'H');
    int top16 = buf.peekInt16();
    EXPECT_EQ(top16, 256 * 'T' + 'H');  // little endian
    EXPECT_EQ(buf.peekInt32(), top16 + 65536 * 'T' + 256 * 65536 * 'P'); // little endian

    EXPECT_EQ(buf.readInt8(), 'H');
    EXPECT_EQ(buf.readInt16(), 256 * 'T' + 'T');
    EXPECT_EQ(buf.readInt8(), 'P');
    EXPECT_EQ(buf.readableSize(), 0);
    EXPECT_EQ(buf.writeableSize(), Buffer::kBufferSize);

    buf.appendInt8(-1);
    buf.appendInt16(-1);
    buf.appendInt32(-1);
    buf.appendInt64(-1);
    EXPECT_EQ(buf.readableSize(), 15);
    EXPECT_EQ(buf.readInt8(), -1);
    EXPECT_EQ(buf.readInt16(), -1);
    EXPECT_EQ(buf.readInt32(), -1);
    EXPECT_EQ(buf.readInt64(), -1);
}

TEST(BufferTest, testBufferFindEOL)
{
    Buffer buf;
    buf.append(string(100000, 'k'));
    const char* null = NULL;
    EXPECT_EQ(buf.findEndOfLine(), null);
    EXPECT_EQ(buf.findEndOfLine(buf.peek() + 90000), null);
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

int main(int argc, char* argv[])
{
    testing::GTEST_FLAG(color) = "yes";
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

