#ifndef DOMO_ESP_SECUTILS_H
#define DOMO_ESP_SECUTILS_H

#include "defines.h"
#include "ArduinoJson.h"
#include "logging.h"
#include <WString.h>

#ifndef ESP32
bool getDomoSecMaterial(String &serverCert, String &serverKey,  String& authUser,  String& authPassword);

#else
#include "SPIFFS.h"
bool getDomoClientSecMaterial(String &caCert, String& authUser,  String& authPassword);
#endif

#endif