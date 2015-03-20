#include <blink/ZlibStream.h>
#include <blink/Log.h>

#include <gtest/gtest.h>

using namespace blink;

TEST(testZlib, Stream)
{
    Buffer output;
    {
        ZlibCompressStream stream(&output);
        EXPECT_EQ(0, output.readableSize());
    }
    EXPECT_EQ(8, output.readableSize());

    Buffer output_raw;
    {
        ZlibDecompressStream stream_decompress(&output_raw);
        EXPECT_EQ(0, output_raw.readableSize());
    }
    EXPECT_EQ(0, output_raw.readableSize());
}

TEST(testZlib, Stream1)
{
    Buffer output;
    ZlibCompressStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    stream.finish();
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());

    Buffer output_raw;
    ZlibDecompressStream stream_decompress(&output_raw);
    EXPECT_EQ(Z_OK, stream_decompress.ZlibErrorCode());
    stream_decompress.finish();
    EXPECT_EQ(Z_STREAM_END, stream_decompress.ZlibErrorCode());
}

TEST(testZlib, Stream2)
{
    Buffer output;
    ZlibCompressStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    EXPECT_TRUE(stream.write("012345678901234567890123456789"));
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());

    Buffer output_raw;
    ZlibDecompressStream stream_decompress(&output_raw);
    EXPECT_EQ(Z_OK, stream_decompress.ZlibErrorCode());
    EXPECT_TRUE(stream_decompress.write(&output));
    stream_decompress.finish();
    EXPECT_STREQ("012345678901234567890123456789", output_raw.toString().c_str());
    printf("total: %zd bytes\n", output_raw.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream_decompress.ZlibErrorCode());
}

TEST(testZlib, Stream3)
{
    Buffer output;
    ZlibCompressStream stream(&output);
    EXPECT_EQ(Z_OK, stream.zlibErrorCode());
    for (int i = 0; i < 1024 * 1024; ++i)
    {
        EXPECT_TRUE(stream.write("012345678901234567890123456789"));
    }
    stream.finish();
    printf("total: %zd bytes\n", output.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream.zlibErrorCode());
}

TEST(testZlib, Stream4)
{
    Buffer output;
    ZlibCompressStream stream(&output);
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

    Buffer output_raw;
    ZlibDecompressStream stream_decompress(&output_raw);
    EXPECT_EQ(Z_OK, stream_decompress.ZlibErrorCode());
    stream_decompress.write(&output);
    stream_decompress.finish();
    string expect;
    expect.reserve(input.size() * 10);
    for (int i = 0; i < 10; ++i)
    {
        expect += input;
    }
    EXPECT_EQ(expect, output_raw.toString());
    printf("total: %zd bytes\n", output_raw.readableSize());
    EXPECT_EQ(Z_STREAM_END, stream_decompress.ZlibErrorCode());
}

TEST(testZlib, Stream5)
{
    Buffer output;
    ZlibCompressStream stream(&output);
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
