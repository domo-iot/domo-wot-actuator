#include "secutils.h"

#ifndef ESP32
bool getDomoSecMaterial(String &serverCert, String &serverKey,  String& authUser,  String& authPassword){

    auto file = SPIFFS.open(SEC_CONFIG_FILE, "r");

    if (!file) {
        LogUDP("SEC_CONFIG_FILE: file not found");
        return false;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [&file](void *) {
        LogUDP(F("SEC_CONFIG_FILE: closing"));
        file.close();
    });

    size_t size = file.size();
    if (!size) {
        LogUDP("SEC_CONFIG_FILE: file size=0");
        return false;
    }

    DynamicJsonDocument loaded_json{3500};

    LogUDP("SEC_CONFIG_FILE: parsing");

    auto res = deserializeJson(loaded_json, file);

    if (res != DeserializationError::Ok) {
        LogUDP("SEC_CONFIG_FILE:  bad Json");
        return false;
    }

    if (loaded_json.containsKey("serverCert") && loaded_json.containsKey("serverKey")
        && loaded_json.containsKey("authUser") && loaded_json.containsKey("authPassword")) {

        serverCert = loaded_json["serverCert"].as<String>();
        serverKey = loaded_json["serverKey"].as<String>();
        authUser = loaded_json["authUser"].as<String>();
        authPassword = loaded_json["authPassword"].as<String>();

        return true;

    }

    return false;
}

#else

bool getDomoClientSecMaterial(String &caCert, String& authUser,  String& authPassword){

    auto file = SPIFFS.open(SEC_CONFIG_FILE, "r");

    if (!file) {
        LogUDP("SEC_CONFIG_FILE: file not found");
        return false;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [&file](void *) {
        LogUDP(F("SEC_CONFIG_FILE: closing"));
        file.close();
    });

    size_t size = file.size();
    if (!size) {
        LogUDP("SEC_CONFIG_FILE: file size=0");
        return false;
    }

    DynamicJsonDocument loaded_json{3500};

    LogUDP("SEC_CONFIG_FILE: parsing");

    auto res = deserializeJson(loaded_json, file);

    if (res != DeserializationError::Ok) {
        LogUDP("SEC_CONFIG_FILE:  bad Json");
        return false;
    }

    if (loaded_json.containsKey("caCert") && loaded_json.containsKey("authUser") && loaded_json.containsKey("authPassword")) {

        caCert = loaded_json["caCert"].as<String>();
        authUser = loaded_json["authUser"].as<String>();
        authPassword = loaded_json["authPassword"].as<String>();

        return true;

    }

    return false;
}

#endif