#include "Exception.h"

#include <stdio.h>

void test()
{
    throw blink::Exception("test for Exception");
}

int main(int argc, char const *argv[])
{
    try
    {
        test();
    }
    catch (const blink::Exception& e)
    {
        printf("reason: %s\n", e.what());
        printf("stacktrace:\n%s\n", e.stackTrace());
    }
    return 0;
}
