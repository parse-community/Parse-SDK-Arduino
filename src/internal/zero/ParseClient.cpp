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

#if defined (ARDUINO_SAMD_ZERO)

#include "../ParseClient.h"
#include "../../external/FlashStorage/FlashStorage.h"
#include <sys/time.h>

// Set DEBUG to true to see serial debug output for the main stages
// of the Parse client.
const bool DEBUG = false;
const char* PARSE_API = "api.parse.com";
const char* USER_AGENT = "arduino-zero.v.1.0.1";
const char* CLIENT_VERSION = "1.0.3";
const char* PARSE_PUSH = "push.parse.com";
const unsigned short SSL_PORT = 443;

struct KeysInternalStorage {
  bool assigned;
  char installationId[37];
  char sessionToken[41];
  char lastPushTime[41];
};

// Reserve a portion of flash memory to store a "KeysInternalStorage" variable
// and call it "parse_flash_store".
FlashStorage(parse_flash_store, KeysInternalStorage);

/*
 * !!! IMPORTANT !!!
 * This function does not conform to RFC 4122, even though it returns
 * a string formatted as an UUID. Do not use this to generate proper UUID!
 */
static String createNewInstallationId(void) {
  static bool randInitialized = false;
  char buff[40];

  if (!randInitialized) {
     struct timeval tm;
     gettimeofday(&tm, NULL);
     // Terrible choice for initialization, prone to collision
     srand((unsigned)tm.tv_sec + (unsigned)tm.tv_usec);
     randInitialized = true;
  }

  snprintf(buff, sizeof(buff),
    "%1x%1x%1x%1x%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x-%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x%1x",
    rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16,
    rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16, rand()%16
  );
  return String(buff);
}

static void sendAndEchoToSerial(WiFiClient& client, const char *line) {
  client.print(line);
  if (Serial && DEBUG)
    Serial.print(line);
}

ParseClient::ParseClient() {
  memset(applicationId, 0, sizeof(applicationId));
  memset(clientKey, 0, sizeof(clientKey));
  memset(installationId, 0, sizeof(installationId));
  memset(sessionToken, 0, sizeof(sessionToken));
  memset(lastPushTime, 0, sizeof(lastPushTime));
  lastHeartbeat = 0;
  dataIsDirty = false;
}

ParseClient::~ParseClient() {
  end();
}

void ParseClient::begin(const char *applicationId, const char *clientKey) {
  if (Serial && DEBUG) {
    Serial.print("begin(");
    Serial.print(applicationId ? applicationId : "NULL");
    Serial.print(", ");
    Serial.print(clientKey ? clientKey : "NULL");
    Serial.println(")");
  }

  if(applicationId) {
    strncpy(this->applicationId, applicationId, sizeof(this->applicationId));
  }
  if (clientKey) {
    strncpy(this->clientKey, clientKey, sizeof(this->clientKey));
  }
  restoreKeys();
}

void ParseClient::setInstallationId(const char *installationId) {
  if (installationId) {
    if (strcmp(this->installationId, installationId))
      dataIsDirty = true;
    strncpy(this->installationId, installationId, sizeof(this->installationId));
  } else {
    if (this->installationId[0])
      dataIsDirty = true;
    strncpy(this->installationId, "", sizeof(this->installationId));
  }
}

const char* ParseClient::getInstallationId() {
  if (!strlen(installationId)) {
    setInstallationId(createNewInstallationId().c_str());
    char buff[40];

    if (Serial && DEBUG) {
      Serial.print("creating new installationId:");
      Serial.println(installationId);
    }

    char content[120];
    snprintf(content, sizeof(content), "{\"installationId\": \"%s\", \"deviceType\": \"embedded\", \"parseVersion\": \"1.0.0\"}", installationId);

    ParseResponse response = sendRequest("POST", "/1/installations", content, "");
    if (Serial && DEBUG) {
      Serial.print("response:");
      Serial.println(response.getJSONBody());
    }
  }
  saveKeys();
  return installationId;
}

void ParseClient::setSessionToken(const char *sessionToken) {
  if ((sessionToken != NULL) && (strlen(sessionToken) > 0 )) {
    if (strcmp(this->sessionToken, sessionToken))
      dataIsDirty = true;
    strncpy(this->sessionToken, sessionToken, sizeof(this->sessionToken));
    if (Serial && DEBUG) {
      Serial.print("setting the session for installation:");
      Serial.println(installationId);
    }
    getInstallationId();
    ParseResponse response = sendRequest("GET", "/1/sessions/me", "", "");
    String installation = response.getString("installationId");
    if (!installation.length() && strlen(installationId)) {
      sendRequest("PUT", "/1/sessions/me", "{}\r\n", "");
    }
  } else {
    if (sessionToken[0])
      dataIsDirty = true;
   this->sessionToken[0] = 0;
  }
}

void ParseClient::clearSessionToken() {
  setSessionToken(NULL);
}

const char* ParseClient::getSessionToken() {
  if (!strlen(sessionToken))
    return NULL;
  return sessionToken;
}

ParseResponse ParseClient::sendRequest(const char* httpVerb, const char* httpPath, const char* requestBody, const char* urlParams) {
  return sendRequest(String(httpVerb), String(httpPath), String(requestBody), String(urlParams));
}

ParseResponse ParseClient::sendRequest(const String& httpVerb, const String& httpPath, const String& requestBody, const String& urlParams) {
  char buff[91] = {0};
  client.stop();

  saveKeys();

  if (Serial && DEBUG) {
    Serial.print("sendRequest(\"");
    Serial.print(httpVerb.c_str());
    Serial.print("\", \"");
    Serial.print(httpPath.c_str());
    Serial.print("\", \"");
    Serial.print(requestBody.c_str());
    Serial.print("\", \"");
    Serial.print(urlParams.c_str());
    Serial.println("\")");
  }

  int retry = 3;
  bool connected;

  while(!(connected = client.connectSSL(PARSE_API, SSL_PORT)) && retry--);

  if (connected) {
    if (Serial && DEBUG) {
      Serial.println("connected to server");
      Serial.println(applicationId);
      Serial.println(clientKey);
      Serial.println(installationId);
    }
    if (urlParams.length() > 0) {
        snprintf(buff, sizeof(buff) - 1, "%s %s?%s HTTP/1.1\r\n", httpVerb.c_str(), httpPath.c_str(), urlParams.c_str());
    } else {
        snprintf(buff, sizeof(buff) - 1, "%s %s HTTP/1.1\r\n", httpVerb.c_str(), httpPath.c_str());
    }
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "Host: %s\r\n",  PARSE_API);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Client-Version: %s\r\n", CLIENT_VERSION);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Application-Id: %s\r\n", applicationId);
    sendAndEchoToSerial(client, buff);
    snprintf(buff, sizeof(buff) - 1, "X-Parse-Client-Key: %s\r\n", clientKey);
    sendAndEchoToSerial(client, buff);

    if (strlen(installationId) > 0) {
      snprintf(buff, sizeof(buff) - 1, "X-Parse-Installation-Id: %s\r\n", installationId);
      sendAndEchoToSerial(client, buff);
    }
    if (strlen(sessionToken) > 0) {
      snprintf(buff, sizeof(buff) - 1, "X-Parse-Session-Token: %s\r\n", sessionToken);
      sendAndEchoToSerial(client, buff);
    }
    if (requestBody.length() > 0 && httpVerb != "GET") {
      requestBody;
      sendAndEchoToSerial(client, "Content-Type: application/json; charset=utf-8\r\n");
    } else if (urlParams.length() > 0) {
      sendAndEchoToSerial(client, "Content-Type: html/text\r\n");
    }
    if (requestBody.length() > 0 && httpVerb != "GET") {
      snprintf(buff, sizeof(buff) - 1, "Content-Length: %d\r\n", requestBody.length());
      sendAndEchoToSerial(client, buff);
    }
    sendAndEchoToSerial(client, "Connection: close\r\n");
    sendAndEchoToSerial(client, "\r\n");
    if (requestBody.length() > 0) {
      sendAndEchoToSerial(client, requestBody.c_str());
    }
  } else {
    if (Serial && DEBUG)
      Serial.println("failed to connect to server");
  }
  ParseResponse response(&client);
  return response;
}

bool ParseClient::startPushService() {
    pushClient.stop();

    saveKeys();

    if (Serial && DEBUG)
        Serial.println("start push");

    int retry = 3;
    bool connected;

    while(!(connected = pushClient.connectSSL(PARSE_PUSH, SSL_PORT)) && retry--);

    if (connected) {
        if (Serial && DEBUG)
            Serial.println("push started");
            char buff[256] = {0};
            snprintf(buff, sizeof(buff),
                "{\"installation_id\":\"%s\", \"oauth_key\":\"%s\", "
                "\"v\":\"e1.0.0\", \"last\":%s%s%s}\r\n{}\r\n",
                installationId,
                applicationId,
                lastPushTime[0] ? "\"" : "",
                lastPushTime[0] ? lastPushTime : "null",
                lastPushTime[0] ? "\"" : "");
            sendAndEchoToSerial(pushClient, buff);
        } else {
        if (Serial && DEBUG)
            Serial.println("failed to connect to push server");
    }
}

ParsePush ParseClient::nextPush() {
  ParsePush push(&pushClient);
  if (pushBuff[0]) {
    push.setLookahead(pushBuff);
    memset(pushBuff, 0, sizeof(pushBuff));
  }
  return push;
}

bool ParseClient::pushAvailable() {
  bool available = (pushClient.available() > 0);
  // send keepalive if connected.
  if (!available && pushClient.connected() && millis() - lastHeartbeat > 1000 * 60 * 12) {
    pushClient.println("{}");
    lastHeartbeat = millis();
  }
  if (pushBuff[0]) {
    available = true;
  } else {
    for (int i = 0; i < sizeof(pushBuff) && (pushClient.available() > 0); ++i) {
      pushBuff[i] = pushClient.read();
    }
    if (!strncmp(pushBuff, "{}", 2)) {
      int i = 2;
      for (; pushBuff[i] == '\r' || pushBuff[i] == '\n'; ++i);
      if (pushBuff[i]) {
        int j = i;;
        for (; pushBuff[i]; ++i)
          pushBuff[i - j] = pushBuff[i];
        pushBuff[i - j] = 0;
      } else {
        memset(pushBuff, 0, sizeof(pushBuff));
        available = (pushClient.available() > 0);
      }
    }
  }
  return available;
}

void ParseClient::stopPushService() {
  pushClient.stop();
}

void ParseClient::end() {
  stopPushService();
}

void ParseClient::saveKeys() {
  if (dataIsDirty) {
    KeysInternalStorage stored_keys = parse_flash_store.read();

    stored_keys.assigned = true;
    strcpy(stored_keys.installationId, installationId);
    strcpy(stored_keys.sessionToken, sessionToken);
    strcpy(stored_keys.lastPushTime, lastPushTime);
    parse_flash_store.write(stored_keys);
    if (Serial && DEBUG) {
      Serial.println("ParseClient::saveKeys() : done.");
    }
    dataIsDirty = false;
  } else {
    if (Serial && DEBUG) {
      Serial.println("ParseClient::saveKeys() : keys are not changed - skipping...");
    }
  }
}

void ParseClient::restoreKeys() {
  KeysInternalStorage stored_keys = parse_flash_store.read();

  if (stored_keys.assigned) {
    strcpy(installationId, stored_keys.installationId);
    strcpy(sessionToken, stored_keys.sessionToken);
    strcpy(lastPushTime, stored_keys.lastPushTime);
    if (Serial && DEBUG) {
      Serial.println("ParseClient::restoreKeys() : done:");
      Serial.println(installationId);
      Serial.println(sessionToken);
      Serial.println(lastPushTime);
    }
  } else {
    if (Serial && DEBUG) {
      Serial.println("ParseClient::restoreKeys() : nothing is stored.");
    }
  }
}

void ParseClient::saveLastPushTime(char *time) {
  dataIsDirty = true;
  strcpy(lastPushTime, time);
  saveKeys();
}


ParseClient Parse;

#endif // ARDUINO_SAMD_ZERO
