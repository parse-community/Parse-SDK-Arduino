/*
 *  Copyright (c) 2015, Parse, LLC. All rights reserved.
 *
 *  You are hereby granted a non-exclusive, worldwide, royalty-free license to use,
 *  copy, modify, and distribute this software in source code or binary form for use
 *  in connection with the web services and APIs provided by Parse.
 *
 *  As with any software that integrates with the Parse platform, your use of
 *  this software is subject to the Parse Terms of Service
 *  [https://www.parse.com/about/terms]. This copyright notice shall be
 *  included in all copies or substantial portions of the software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#if defined (ARDUINO_ARCH_ESP8266)

#include "../ParsePush.h"
#include "../ParseClient.h"
#include "../ParseUtils.h"

ParsePush::ParsePush(WiFiClientSecure* pushClient) : ParseResponse(pushClient) {
    memset(lookahead, 0, sizeof(lookahead));
}

void ParsePush::close() {
    freeBuffer();
}

void ParsePush::setLookahead(const char *read_data) {
    strncpy(lookahead, read_data, sizeof(lookahead));
}

void ParsePush::read() {
    if (buf == NULL) {
        bufSize = BUFSIZE;
        buf = new char[bufSize];
    }

    if (p == bufSize - 1) {
        return;
    }

    if (p == 0) {
        memset(buf, 0, bufSize);
        for (; lookahead[p]; ++p)
            buf[p] = lookahead[p];
        lookahead[0] = 0;
    }

    char c;
    while (client->connected()) {
        if (client->available()) {
            c = client->read();
            if (c == '\n') c = '\0';
            if (p < bufSize - 1) {
                *(buf + p) = c;
                p++;
            }
            if (c == '\0') break;
        }
    }
    p = 0;
    char time[41];
    if (ParseUtils::getStringFromJSON(buf, "time", time, sizeof(time))) {
      Parse.saveLastPushTime(time);
    }
}

#endif // ARDUINO_ARCH_ESP8266

