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
                         blink::string* command,
                         blink::string* topic,
                         blink::string* content);

#endif
