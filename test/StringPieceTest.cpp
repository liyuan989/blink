#include <blink/StringPiece.h>
#include <blink/Types.h>

#include <gtest/gtest.h>

#include <string>

using namespace blink;

TEST(testStringPiece, CStypeString)
{
    StringPiece piece1("123456789");
    EXPECT_EQ(9, piece1.size());
    EXPECT_FALSE(piece1.empty());
    EXPECT_EQ("123456789", string(piece1.data(), piece1.size()));
    EXPECT_EQ("123456789", piece1.asString());
    EXPECT_EQ('1', piece1[0]);
    EXPECT_EQ('9', piece1[8]);

    StringPiece piece2;
    piece2.set("123456789");
    EXPECT_EQ(9, piece1.size());
    EXPECT_FALSE(piece1.empty());
    EXPECT_EQ("123456789", string(piece2.data(), piece2.size()));
    EXPECT_EQ("123456789", piece2.asString());
    EXPECT_TRUE(piece1 == piece2);
    EXPECT_EQ(0, piece1.compare(piece2));
    EXPECT_TRUE(piece1.startWith(piece2));
    EXPECT_EQ('1', piece2[0]);
    EXPECT_EQ('9', piece2[8]);
    EXPECT_TRUE(piece1.end() == piece2.end());

    piece2.removeSuffix(3);
    EXPECT_EQ(6, piece2.size());
    EXPECT_FALSE(piece2.empty());
    EXPECT_EQ("123456", string(piece2.data(), piece2.size()));
    EXPECT_EQ("123456", piece2.asString());
    EXPECT_TRUE(piece1 > piece2);
    EXPECT_TRUE(piece1.compare(piece2));
    EXPECT_TRUE(piece1.startWith(piece2));
    EXPECT_EQ('1', piece2[0]);
    EXPECT_EQ('6', piece2[5]);
    EXPECT_TRUE(piece1.end() != piece2.end());

    piece2.removePrefix(3);
    EXPECT_EQ(3, piece2.size());
    EXPECT_FALSE(piece2.empty());
    EXPECT_EQ("456", string(piece2.data(), piece2.size()));
    EXPECT_EQ("456", piece2.asString());
    EXPECT_TRUE(piece1 < piece2);
    EXPECT_TRUE(piece1.compare(piece2));
    EXPECT_FALSE(piece1.startWith(piece2));
    EXPECT_EQ('4', piece2[0]);
    EXPECT_EQ('6', piece2[2]);
    EXPECT_TRUE(piece1.end() != piece2.end());

    string s;
    piece2.copyToString(&s);
    EXPECT_EQ(*s.data(), *piece2.data());
    EXPECT_EQ(s, piece2.asString());

    piece2.clear();
    EXPECT_EQ(0, piece2.size());
    EXPECT_TRUE(piece2.empty());
    EXPECT_EQ(NULL, piece2.data());
}

TEST(testStringPiece, String)
{
    string base = "123456789";
    StringPiece piece1(base);
    EXPECT_EQ(9, piece1.size());
    EXPECT_FALSE(piece1.empty());
    EXPECT_EQ("123456789", string(piece1.data(), piece1.size()));
    EXPECT_EQ("123456789", piece1.asString());
    EXPECT_EQ('1', piece1[0]);
    EXPECT_EQ('9', piece1[8]);

    StringPiece piece2;
    piece2.set(base.c_str(), base.size());
    EXPECT_EQ(9, piece1.size());
    EXPECT_FALSE(piece1.empty());
    EXPECT_EQ("123456789", string(piece2.data(), piece2.size()));
    EXPECT_EQ("123456789", piece2.asString());
    EXPECT_TRUE(piece1 == piece2);
    EXPECT_EQ(0, piece1.compare(piece2));
    EXPECT_TRUE(piece1.startWith(piece2));
    EXPECT_EQ('1', piece2[0]);
    EXPECT_EQ('9', piece2[8]);
    EXPECT_TRUE(piece1.end() == piece2.end());

    piece2.removeSuffix(3);
    EXPECT_EQ(6, piece2.size());
    EXPECT_FALSE(piece2.empty());
    EXPECT_EQ("123456", string(piece2.data(), piece2.size()));
    EXPECT_EQ("123456", piece2.asString());
    EXPECT_TRUE(piece1 > piece2);
    EXPECT_TRUE(piece1.compare(piece2));
    EXPECT_TRUE(piece1.startWith(piece2));
    EXPECT_EQ('1', piece2[0]);
    EXPECT_EQ('6', piece2[5]);
    EXPECT_TRUE(piece1.end() != piece2.end());

    piece2.removePrefix(3);
    EXPECT_EQ(3, piece2.size());
    EXPECT_FALSE(piece2.empty());
    EXPECT_EQ("456", string(piece2.data(), piece2.size()));
    EXPECT_EQ("456", piece2.asString());
    EXPECT_TRUE(piece1 < piece2);
    EXPECT_TRUE(piece1.compare(piece2));
    EXPECT_FALSE(piece1.startWith(piece2));
    EXPECT_EQ('4', piece2[0]);
    EXPECT_EQ('6', piece2[2]);
    EXPECT_TRUE(piece1.end() != piece2.end());

    string s;
    piece2.copyToString(&s);
    EXPECT_EQ(*s.data(), *piece2.data());
    EXPECT_EQ(s, piece2.asString());

    piece2.clear();
    EXPECT_EQ(0, piece2.size());
    EXPECT_TRUE(piece2.empty());
    EXPECT_EQ(NULL, piece2.data());
}

TEST(testStringArg, All)
{
    StringArg arg1("123456789");
    EXPECT_EQ("123456789", arg1.c_str());

    StringArg arg2(string("123456789"));
    EXPECT_EQ(string("123456789"), arg2.c_str());

    StringArg arg3(std::string("123456789"));
    EXPECT_EQ(std::string("123456789"), arg3.c_str());
}

int main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
