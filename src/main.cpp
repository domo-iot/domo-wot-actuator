#if not defined(ESP32)
#include "FS.h"
#endif

#include <Arduino.h>
#include "HardwareController.h"
#include "defines.h"
#include "shelly_dimmer/STM32Updater.h"
#include "logging.h"
#ifdef ESP32
#include "SPIFFS.h"
#endif
#ifdef SHELLY_1_PLUS
#include "shelly_1plus/BleManager.h"
#endif



struct WiFiConf {
    String ssid;
    String password;
};

#ifdef SHELLY_DIMMER
void checkStm32Update() {

    STM32Updater updater{};

    if (!updater.updateSTM32()) {
        ESP.restart();
        delay(3000);
        ESP.reset();
    }

}
#endif

WiFiConf getWiFiConf(){

    WiFiConf c;
    c.ssid = DEFAULT_WIFI_SSID;
    c.password = DEFAULT_WIFI_PASSWORD;

    auto file = SPIFFS.open(WIFI_CONFIG_FILE, "r");

    if (!file) {
        LogUDP("WIFI_CONFIG_FILE: file not found");
        return c;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [&file](void *) {
        LogUDP(F("WIFI_CONFIG_FILE: closing"));
        file.close();
    });


    size_t size = file.size();
    if (!size) {
        LogUDP("WIFI_CONFIG_FILE: file size=0");
        return c;
    }

    DynamicJsonDocument loaded_json{512};

    LogUDP("WIFI_CONFIG_FILE: parsing");

    auto res = deserializeJson(loaded_json, file);

    if (res != DeserializationError::Ok) {
        LogUDP("WIFI_CONFIG_FILE:  bad Json");
        return c;
    }

    if (loaded_json.containsKey("wifi")) {
        auto wifi = loaded_json["wifi"];
        if (wifi.containsKey("wifi_ssid") && wifi.containsKey("wifi_password")){
            c.ssid = wifi["wifi_ssid"].as<String>();
            c.password = wifi["wifi_password"].as<String>();
        }
    }

    return c;
}

const int ledPin = 0; // manually configure LED pin

ShellyManager *shellyManagerInstance{nullptr};
HardwareController *hardwareControllerInstance{nullptr};
DomoUpdater *updaterControllerInstance{nullptr};
bool ShellyManager::shouldReboot = false;
String ShellyManager::action_in_progress = "";

#ifdef SHELLY_1_PLUS
BleManager* bleManagerInstance {nullptr};
#endif

#ifdef SHELLY_2PM_PLUS
uint8_t HardwareController::INPUT_1_PIN = 2;
#endif

void updateNetworkStatus() {

    #ifndef ESP32
    if (WiFi.getMode() != WiFiMode::WIFI_STA || !WiFi.isConnected()) {
        return;
    }
    #else
    if (WiFi.getMode() != WIFI_STA || !WiFi.isConnected()) {
        return;
    }
    #endif


    auto bssid = WiFi.BSSIDstr();

    bssid.toLowerCase();

    bssid.replace(":", "");

    shellyManagerInstance->setNetworkInfo(WiFi.localIP().toString(), WiFi.gatewayIP().toString(),
                                          bssid, WiFi.SSID(), WiFi.psk());
    shellyManagerInstance->setRSSI(WiFi.RSSI());
    shellyManagerInstance->setMCUTemperature(HardwareController::getInstance().getMCUTemperature());

}


void setup(void) {

    #ifndef ESP32
    if (SPIFFS.begin()){
    #else
    if(SPIFFS.begin(true)){
    #endif
        LogUDP(F("LittleFS init.\n"));
    } else{
        LogUDP(F("Failed LittleFS init.\n"));
        ESP.restart();
    }

    WiFiConf c = getWiFiConf();

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    Serial.begin(115200);
    LogUDP("");
    Serial.print("Connecting to \"");
    Serial.print(c.ssid);
    LogUDP("\"");
#if defined(ESP8266) || defined(ESP32)
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(c.ssid.c_str(), c.password.c_str());
    LogUDP("");

    // Wait for connection
    bool blink = true;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        digitalWrite(ledPin, blink ? LOW : HIGH); // active low led
        blink = !blink;
    }

    digitalWrite(ledPin, HIGH); // active low led
    LogUDP("");
    Serial.print("Connected to ");
    LogUDP(c.ssid);
    Serial.print("IP address: ");
    LogUDP(WiFi.localIP().toString());

    shellyManagerInstance = &ShellyManager::getInstance();
    hardwareControllerInstance = &HardwareController::getInstance();
    hardwareControllerInstance->init();

    updaterControllerInstance = &DomoUpdater::getInstance();

    #ifndef ESP32
    LogUDP("HTTP server started");
    #endif
    #ifdef SHELLY_1_PLUS
    LogUDP("WebSocket client started");
    bleManagerInstance = &BleManager::getInstance();
    bleManagerInstance->init();
    #endif
    LogUDP(WiFi.localIP().toString());

    LogUDP("Device id: ");

    LogUDP(shellyManagerInstance->getDeviceName());

    updateNetworkStatus();

    #ifdef SHELLY_DIMMER
    checkStm32Update();
    #endif
}


inline void heapCheck() {

    #ifndef ESP32
    auto now_heap = system_get_free_heap_size();

    Serial.println("HEAP:" + String(now_heap));

    Serial.flush();
    #endif


}


void loop() {

    static DomoTimer printConnectedTimer{};

    if(!WiFi.isConnected()) {
       #ifndef SHELLY_DIMMER
        if(printConnectedTimer.elapsed(1000)){
            Serial.println("Not connected");
        }
       #endif
    } else {

        if(printConnectedTimer.elapsed(1000)){
            Serial.println("Connected");
        }
    }



    static DomoTimer networkTimer{};

    if (networkTimer.elapsed(60000)) {
        updateNetworkStatus();
        heapCheck();
    }




    hardwareControllerInstance->loop();

    shellyManagerInstance->loop();

    #ifdef SHELLY_1_PLUS
    bleManagerInstance->loop();
    #endif

}



