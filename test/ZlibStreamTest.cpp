#include <blink/ZlibStream.h>
#include <blink/Log.h>

#include <gtest/gtest.h>

using namespace blink;

TEST(testZlib, OutputStream)
{
    Buffer output;
    {
        ZlibOutputStream stream(&output);
        EXPECT_EQ(0, output.readableSize());
    }
    EXPECT_EQ(8, output.readableSize());
}

TEST(testZlib, OutputStream1)
{
    Buffer output;
    ZlibOutputStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    stream.finish();
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

TEST(testZlib, OutputStream2)
{
    Buffer output;
    ZlibOutputStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    EXPECT_TRUE(stream.write("012345678901234567890123456789"));
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

TEST(testZlib, OutputStream3)
{
    Buffer output;
    ZlibOutputStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    for (int i = 0; i < 1024 * 1024; ++i)
    {
        EXPECT_TRUE(stream.write("012345678901234567890123456789"));
    }
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

TEST(testZlib, OutputStream4)
{
    Buffer output;
    ZlibOutputStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    string input;
    for (int i = 0; i < 32768; ++i)
    {
        input += "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-"[rand() % 64];
    }
    for (int i = 0; i < 10; ++i)
    {
        EXPECT_TRUE(stream.write(input));
    }
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

TEST(testZlib, OutputStream5)
{
    Buffer output;
    ZlibOutputStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    string input(1024 * 1024, '_');
    for (int i = 0; i < 64; ++i)
    {
        EXPECT_TRUE(stream.write(input));
    }
    printf("Buffer_size: %d\n", stream.internalOutputBufferSize());
    LOG_INFO << "total_in: " << stream.inputBytes();
    LOG_INFO << "total_out: " << stream.outputBytes();
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

int main(int argc, char* argv[])
{
    testing::GTEST_FLAG(color) = "yes";
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
