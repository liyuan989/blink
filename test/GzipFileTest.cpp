#include <blink/GzipFile.h>
#include <blink/Log.h>

#include <errno.h>

using namespace blink;

int main(int argc, char* argv[])
{
    const char* filename = "GzipFileTest.gz";
    unlink(filename);
    const char data[] = "123456789012345678901234567890123456789012345678901234567890";

    {
        printf("testing openForAppend\n");
        GzipFile writer = GzipFile::openForAppend(filename);
        if (writer.valid())
        {
            LOG_INFO << "tell " << writer.tell();
            LOG_INFO << "wrote " << writer.write(data);
            LOG_INFO << "tell " << writer.tell();
        }
    }

    {
        printf("testing openForRead\n");
        GzipFile reader(GzipFile::openForRead(filename));
        if (reader.valid())
        {
            char buf[256];
            LOG_INFO << "tell " <<reader.tell();
            int nread = reader.read(buf, sizeof(buf));
            printf("read %d\n", nread);
            if (nread >= 0)
            {
                buf[nread] = '\0';
                printf("data: %s\n", buf);
            }
            LOG_INFO << "tell " << reader.tell();
            if (strncmp(buf, data, strlen(data)) != 0)
            {
                printf("failed!\n");
                abort();
            }
            else
            {
                printf("passed!\n");
            }
        }
    }

    {
        printf("testing openForWriteExclusive\n");
        GzipFile writer = GzipFile::openForWriteExclusive(filename);
        if (writer.valid() || errno != EEXIST)
        {
            printf("failed!\n");
        }
        else
        {
            printf("passed!\n");
        }
    }

    return 0;
}
