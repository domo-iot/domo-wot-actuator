//
// Created by domenico on 9/5/22.
//

#include "logging.h"


void LogUDP(const String& s) {

    #if defined(SHELLY_DIMMER)
    static WiFiUDP Udp;

    if (WiFi.isConnected()) {
        Udp.beginPacket(IPAddress(10, 0, 1, 248), 4389);
        Udp.write((s+"\n").c_str());
        Udp.endPacket();
    }
    #else
    Serial.println(s);
    Serial.flush();
    #endif

}