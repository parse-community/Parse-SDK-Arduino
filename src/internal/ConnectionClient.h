#ifndef __CONNECTION_CLIENT_H__
#define __CONNECTION_CLIENT_H__


#include <Arduino.h>

#ifdef ARDUINO_SAMD_ZERO

#include <WiFi101.h>
typedef WiFiClient ConnectionClient;

#elif ARDUINO_AVR_YUN

#include <Bridge.h>
typedef Process ConnectionClient;

#elif ARDUINO_ARCH_ESP8266

#include <WiFiClientSecure.h>
typedef WiFiClientSecure ConnectionClient;

#endif

#endif //__CONNECTION_CLIENT_H__
