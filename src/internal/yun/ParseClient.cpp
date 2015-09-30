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


#if defined (ARDUINO_AVR_YUN)

#include "../ParseClient.h"
#include "../ParseUtils.h"
#include "../ParsePlatformSupport.h"

ParseClient::ParseClient() {
  applicationId[0] = '\0';
  clientKey[0] = '\0';
  installationId[0] = '\0';
  sessionToken[0] = '\0';
}

ParseClient::~ParseClient() {
  end();
}

void ParseClient::begin(const char *applicationId, const char *clientKey) {
  Console.println("begin");

  // send the info to linux through Bridge
  if(applicationId) {
    Bridge.put("appId", applicationId);
  }
  if (clientKey) {
    Bridge.put("clientKey", clientKey);
  }
}

void ParseClient::setInstallationId(const char *installationId) {
  strncpy(this->installationId, installationId, sizeof(this->installationId));
  if (installationId) {
    Bridge.put("installationId", installationId);
  } else {
    Bridge.put("installationId", "");
  }
}

const char* ParseClient::getInstallationId() {
  if (this->installationId[0] == '\0') {
    client.begin("parse_request");
    client.addParameter("-i");
    client.run();
    ParsePlatformSupport::read(&client, this->installationId, sizeof(installationId));
  }
  return this->installationId;
}

void ParseClient::setSessionToken(const char *sessionToken) {
  if ((sessionToken != NULL) && (sessionToken[0] != '\0')){
    strncpy(this->sessionToken, sessionToken, sizeof(this->sessionToken));
    Bridge.put("sessionToken", sessionToken);
  } else {
    Bridge.put("sessionToken", "");
    this->sessionToken[0] = '\0';
  }
}

void ParseClient::clearSessionToken() {
  setSessionToken(NULL);
}

const char* ParseClient::getSessionToken() {
  if (!this->sessionToken) {
    char *buf = new char[33];

    client.begin("parse_request");
    client.addParameter("-s");
    client.run();
    ParsePlatformSupport::read(&client, buf, 33);
    strncpy(this->sessionToken, buf, sizeof(this->sessionToken));
  }
  return this->sessionToken;
}

ParseResponse ParseClient::sendRequest(const char* httpVerb, const char* httpPath, const char* requestBody, const char* urlParams) {
  return sendRequest(String(httpVerb), String(httpPath), String(requestBody), String(urlParams));
}

ParseResponse ParseClient::sendRequest(const String& httpVerb, const String& httpPath, const String& requestBody, const String& urlParams) {
  while(client.available()) {
    client.read();                // flush out the buffer in case there is any leftover data there
  }
  client.begin("parse_request");  // start a process that launch the "parse_request" command

  if(ParseUtils::isSanitizedString(httpVerb)
  && ParseUtils::isSanitizedString(httpPath)
  && ParseUtils::isSanitizedString(requestBody)
  && ParseUtils::isSanitizedString(urlParams)) {
    client.addParameter("-v");
    client.addParameter(httpVerb);
    client.addParameter("-e");
    client.addParameter(httpPath);
    if (requestBody != "") {
      client.addParameter("-d");
      client.addParameter(requestBody);
    }
    if (urlParams != "") {
      client.addParameter("-p");
      client.addParameter(urlParams);
      client.runAsynchronously();
    } else {
      client.run(); // Run the process and wait for its termination
    }
  }

  ParseResponse response(&client);
  return response;
}

bool ParseClient::startPushService() {
  pushClient.begin("parse_push");  // start a process that launch the "parse_request" command
  pushClient.runAsynchronously();

  while(1) {
    if(pushClient.available()) {
      // read the push starting result
      char c = pushClient.read();
      while(pushClient.available()) {
        pushClient.read();
      }
      if (c == 's') {
        pushClient.write('n'); // send a signal that ready to consume next available push
        return true;
      } else {
        return false;
      }
    }
  }
}

ParsePush ParseClient::nextPush() {
  ParsePush push(&pushClient);
  return push;
}

bool ParseClient::pushAvailable() {
  pushClient.write('n'); // send a signal that ready to consume next available push
  if(pushClient.available()) {
    return 1;
  } else {
    return 0;
  }
}

void ParseClient::stopPushService() {
  pushClient.close();
}

void ParseClient::end() {
  if(installationId[0] != '\0') {
    installationId[0] = '\0';
  }
  if(sessionToken[0] != '\0') {
    sessionToken[0] = '\0';
  }

  stopPushService();
}

ParseClient Parse;

#endif
