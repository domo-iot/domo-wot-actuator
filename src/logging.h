//
// Created by domenico on 9/5/22.
//

#ifndef SHELLY_WEBTHINGS_LOGGING_H
#define SHELLY_WEBTHINGS_LOGGING_H


#include <WString.h>
#include <WiFiUdp.h>
#ifndef ESP32
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

void LogUDP(const String& s);

#endif //SHELLY_WEBTHINGS_LOGGING_H
