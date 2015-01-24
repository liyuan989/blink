#ifndef __EXAMPLE_HUB_CODEC_H__
#define __EXAMPLE_HUB_CODEC_H__

#include "Buffer.h"

enum ParseResult
{
    kError,
    kSuccess,
    kContinue,
};

ParseResult parseMessage(blink::Buffer* buf,
                         std::string* command,
                         std::string* topic,
                         std::string* content);

#endif
