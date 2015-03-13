#include <example/memcached/server/MemcacheServer.h>

#include <blink/inspect/ProcessInspector.h>
#include <blink/EventLoop.h>

#ifdef HAVE_TCMALLOC
#include <gperftools/heap-profiler.h>
#include <gperftools/malloc_extension.h>
#endif

#include <unistd.h>

#include <stdio.h>

using namespace blink;

int main(int argc, char* argv[])
{
#ifdef HAVE_TCMALLOC
    MallocExtension::Initialize();
#endif
    int items = argc > 1 ? atoi(argv[1]) : 10000;
    int key_len = argc > 2 ? atoi(argv[2]) : 10;
    int value_len = argc > 3 ? atoi(argv[3]) : 100;
    EventLoop loop;
    MemcacheServer::Options options;
    MemcacheServer server(&loop, options);

    printf("sizeof(Item) = %zd\npid = %d\nitems = %d\nkey_len = %d\nvalue_len = %d\n",
           sizeof(Item), getpid(), items, key_len, value_len);
    char key[256] = {0};
    string value;
    for (int i = 0; i < items; ++i)
    {
        snprintf(key, sizeof(key), "%0*d", key_len, i);
        value.assign(value_len, "0123456789"[i % 10]);
        ItemPtr item(Item::makeItem(key, 0, 0, value_len + 2, 1));
        item->append(value.data(), value.size());
        item->append("\r\n", 2);
        assert(item->endWithCRLF());
        bool exists = false;
        bool stored = server.storeItem(item, Item::kAdd, &exists);
        assert(stored);
        (void)stored;
        assert(!exists);
    }
    Inspector::ArgList arg;
    printf("==========================\n");
    printf("%s\n", ProcessInspector::overview(HttpRequest::kGet, arg).c_str());
    fflush(stdout);

#ifdef HAVE_TCMALLOC
    char buf[8192];
    MallocExtension::instance()->GetStats(buf, sizeof(buf));
    printf("%s\n", buf);
    HeapProfilerDump("end");

    int blocks = 0;
    size_t total = 0;
    int histogram[kMallocHistogramSize] = {0};
    MallocExtension::instance()->MallocMemoryStats(&blocks, &total, histogram);
    printf("==========================\n");
    printf("blocks = %d\ntotal = %zd\n", blocks, total);
    for (int i = 0; i < kMallocHistogramSize; ++i)
    {
        printf("%d = %d\n", i, histogram[i]);
    }
#endif
    return 0;
}
