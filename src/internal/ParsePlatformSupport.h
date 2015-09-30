#ifndef __PARSE_PLATFORM_SUPPORT_H__
#define __PARSE_PLATFORM_SUPPORT_H__

#include <Arduino.h>
#include "ConnectionClient.h"

class ParsePlatformSupport {
public:
    static int read(ConnectionClient* client, char* buf, int len);
};

#endif //__PARSE_PLATFORM_SUPPORT_H__
