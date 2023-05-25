#ifndef ESP32
#include <ESP8266WiFi.h>
#else
#include "WiFi.h"
#endif
#include <algorithm>
#ifdef ESP32
#include <SPIFFS.h>
#endif

#include "utils.h"
#include "defines.h"
#include "ShellyManager.h"
#include "HardwareController.h"
#include "shelly_1plus/BleManager.h"

void ShellyManager::loop() {
    adapter.update();
    {
        #ifdef SHELLY_1_PLUS
        Lockguard l(this->stateMutex);
        #endif
        // remove pending props
        this->pending_updated_props.clear();
    }
    DomoUpdater::getInstance().loop();

    if(ShellyManager::shouldReboot) {
        ESP.restart();
        delay(1000);
        #ifndef ESP32
        ESP.reset();
        #else
        ESP.restart();
        #endif
    }

}

void ShellyManager::update_action_handler(const String& firmware_url, const String& firmware_version) {
            if(firmware_version != DomoUpdater::getInstance().currentFwVersion) {
                LogUDP("UPDATE needed");
                DomoUpdater::getInstance().setFwToUpdate(firmware_url);
            }else {
                LogUDP("FW already updated");
            }
}

void ShellyManager::updateState(const String updated_properties[], uint16_t changed_props_num){

    status.clear();

    // serialize status
    this->serialized_shelly_state.clear();

    status["mac_address"] = this->macAddress;

    status["topic_name"] = this->getTopicName();

    status["ip_address"] = this->ip_address;

    status["gateway"] = this->gateway;

    status["ap_mac_address"] = this->ap_mac_address;

    status["wifi_ssid"] = this->wifi_ssid;

    status["wifi_password"] = this->wifi_password;

    status["fw_version"] = this->fwVersion;

    status["rssi"] = this->rssi;

    status["mode"] = this->mode;

    #ifndef SHELLY_EM
    status["input1"] = this->input1;
    #endif

    #if defined(SHELLY_25)  || defined(SHELLY_DIMMER) || defined(SHELLY_2PM_PLUS)
    status["input2"] = this->input2;
    #endif

    #if defined(SHELLY_1PM)  || defined(SHELLY_25) || defined(SHELLY_DIMMER) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_2PM_PLUS)
    status["power1"] = this->power1;
    status["energy1"] = this->energy1;
    #endif

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    status["power2"] = this->power2;
    status["energy2"] = this->energy2;
    #endif

    if (this->mode == ShellyOutputMode::RELAY) {
        status["output1"] = this->output1;
    }

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    if (this->mode == ShellyOutputMode::RELAY) {
        status["output2"] = this->output2;
    }
    #endif

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    if(this->mode == ShellyOutputMode::SHUTTER) {
        status["shutter_status"] = this->shutterStatus;
        status["inverted"] = this->inverted;
    }
    #endif

    #if defined(SHELLY_DIMMER)
    if(this->mode == ShellyOutputMode::DIMMER) {
        status["dimmer_status"] = this->dimmerStatus;
    }
    #endif

    #if defined(SHELLY_RGBW)
    status["rgbw_status"] = this->rgbwStatus.string_representation;
    #endif

    #if defined(SHELLY_1_PLUS)
    status["beacon_adv"] = this->beaconAdv;
    status["valve_operation"] = this->valveOperation;
    #endif


    status["mcu_temperature"] = this->mcuTemperature;

    #if defined(SHELLY_1) || defined(SHELLY_1PM)
    status["ambient_temperature"] = this->temperature;
    #endif

    #if defined(SHELLY_EM)
    status["power_data"] = this->powerData.to_json();
    #endif

    auto props = status.createNestedArray("updated_properties");

    // appending pending updated props
    Serial.println("PENDING PROPS: ");
    Serial.flush();

    for(int i=0; i<pending_updated_props.size(); i++) {
        Serial.println(i);
        Serial.flush();
        props.add(pending_updated_props[i]);
    }

    Serial.println("END PENDING PROPS: ");
    Serial.flush();

    // appending new props
    for (int i=0;i<changed_props_num; i++){
        props.add(updated_properties[i]);
        pending_updated_props.push_back(updated_properties[i]);
    }

    #ifndef ESP32
    auto now_heap = system_get_free_heap_size();

    Serial.println("HEAP:" + String(now_heap));

    Serial.flush();
    #endif

    int ret = serializeJson(status, this->serialized_shelly_state);

    Serial.println("RET");
    Serial.println(ret);
    Serial.flush();

    if (ret <= 0 ){
        Serial.println("Error while SERIALIZING");
        Serial.flush();
    }

    //LogUDP("New state: " + this->serialized_shelly_state);
    //Serial.flush();

    //Serial.println("Setting prop value");
    //Serial.flush();

    ThingPropertyValue value;
    value.string = &this->serialized_shelly_state;
    #ifndef SHELLY_1_PLUS
    this->shellyStatusProp.setValue(value);
    #else
    {

        if (!BleManager::getInstance().operation_in_progress) {
            this->shellyStatusProp.setValue(value);
        }
    }
    #endif

    //Serial.println("prop value set");
    //Serial.flush();


    if (updated_properties != nullptr) {
        if (updated_properties[0] == "beacon_adv" || updated_properties[0] == "valve_operation") return;
    }

    //Serial.println("Persisting");
    //Serial.flush();
    this->persistRitenitiveOnFlash();

}

void ShellyManager::setRSSI(int32_t rssi) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->rssi=rssi;
    String updated_props[1] = {"rssi"};
    this->updateState(updated_props, 1);
}

void ShellyManager::setMCUTemperature(float mcu_temperature){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->mcuTemperature = mcu_temperature;
    String updated_props[1] = {"mcu_temperature"};
    this->updateState(updated_props, 1);
}


void ShellyManager::setFwVersion(const String& version){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->fwVersion = version;
    String updated_props[1] = {"fw_version"};
    this->updateState(updated_props, 1);
}


void ShellyManager::setNetworkInfo(const String &ip, const String &gw, const String &apMacAddress,
                                   const String& wifiSSID, const String& wifiPassword) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->ip_address = ip;
    this->gateway = gw;
    this->ap_mac_address = apMacAddress;
    this->wifi_ssid = wifiSSID;
    this->wifi_password = wifiPassword;

    String updated_props[5] = {"ip_address", "gateway", "ap_mac_address", "wifi_ssid", "wifi_password"};

    this->updateState(updated_props, 5);
}

void ShellyManager::inputChanged(int inputNumber, bool value) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    if (inputNumber == 1) {
        if (value == this->input1)
            return;
        this->input1 = value;
        String updated_props[1] = {"input1"};
        this->updateState(updated_props, 1);
    } else {
        if (inputNumber == 2) {
            if (value == this->input2)
                return;
            this->input2 = value;
            String updated_props[1] = {"input2"};
            this->updateState(updated_props, 1);
        }
        else
            return;

    }

}


void ShellyManager::shutterStatusChanged(ShutterStatus newStatus){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->shutterStatus = newStatus;

    String updated_props[1] = {"shutter_status"};
    this->updateState(updated_props, 1);
}

void ShellyManager::updateValveOperation(const String& valveOperation){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->valveOperation = valveOperation;
    String updated_props[1] = {"valve_operation"};
    this->updateState(updated_props, 1);
}

void ShellyManager::updateBeaconAdv(const String& adv){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    Serial.println("updateBeaconAdv");
    Serial.flush();

    this->beaconAdv = adv;
    String updated_props[1] = {"beacon_adv"};
    this->updateState(updated_props, 1);
}


void ShellyManager::dimmerStatusChanged(uint32_t newStatus){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->dimmerStatus = newStatus;

    String updated_props[1] = {"dimmer_status"};
    this->updateState(updated_props, 1);

}


void ShellyManager::RGBWStatusChanged(uint32_t r, uint32_t g, uint32_t b, uint32_t w){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->rgbwStatus.setRGBW(r, g, b,w);

    String updated_props[1] = {"rgbw_status"};
    this->updateState(updated_props, 1);

}

void ShellyManager::rChannelStatusChanged(uint32_t newStatus){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    LogUDP("R channel status changed");

    this->rgbwStatus.setR(newStatus);

    String updated_props[1] = {"rgbw_status"};
    this->updateState(updated_props, 1);

}

void ShellyManager::gChannelStatusChanged(uint32_t newStatus){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->rgbwStatus.setG(newStatus);
    String updated_props[1] = {"rgbw_status"};
    this->updateState(updated_props, 1);
}

void ShellyManager::bChannelStatusChanged(uint32_t newStatus) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->rgbwStatus.setB(newStatus);
    String updated_props[1] = {"rgbw_status"};
    this->updateState(updated_props, 1);
}

void ShellyManager::wChannelStatusChanged(uint32_t newStatus){
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->rgbwStatus.setW(newStatus);
    String updated_props[1] = {"rgbw_status"};
    this->updateState(updated_props, 1);
}

void ShellyManager::outputChanged(int outputNumber, bool value) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    Serial.println("OutputChanged ShellyManager");
    Serial.flush();

    if (outputNumber == 1) {

        if (value == this->output1)
            return;

        this->output1 = value;
        String updated_props[1] = {"output1"};
        this->updateState(updated_props, 1);

    } else {

        if (outputNumber == 2) {

            if (value == this->output2)
                return;

            this->output2 = value;
            String updated_props[1] = {"output2"};
            this->updateState(updated_props, 1);
        }
        else
            return;

    }


}


ShellyManager::ShellyManager():
                                macAddress(ShellyManager::get_mac_address()),
                                deviceName(ShellyManager::getDeviceModel() + "-" + this->macAddress),
                                #ifndef ESP32
                                adapter(deviceName, WiFi.localIP(), 443),
                                #else
                                //adapter(deviceName, WiFi.gatewayIP(), 443),
                                adapter(deviceName, WiFi.gatewayIP(), 443),
                                #endif
                                shelly(deviceName.c_str(), "Shelly", this->shellyTypes),
                                shellyStatusProp("status", "shelly_status", STRING, ""),
                                shelly_action("shelly_action",
                                              "shelly_action",
                                              "shelly_action",
                                              "shelly_action",
                                              &shelly_action_input_obj,
                                              &ShellyManager::shelly_action_generator
                                ){

    get_mode();

    LogUDP("Add device");

    adapter.addDevice(&shelly);

    // properties

    shelly.addProperty(&shellyStatusProp);

    LogUDP("Add property");

    // actions
    add_shelly_action();

    LogUDP("Add action");

    Serial.flush();
    adapter.begin();

}



String ShellyManager::get_mac_address() {
    auto macAddress = WiFi.macAddress();

    macAddress.toLowerCase();

    macAddress.replace(":", "");

    return macAddress;
}


void ShellyManager::add_shelly_action() {

    this->shelly_action_input_obj = shelly_action_json.to<JsonObject>();

    shelly_action_input_obj["type"] = "object";

    JsonObject command =
            shelly_action_input_obj.createNestedObject("action");

    JsonObject action_name =
            command.createNestedObject("action_name");

    action_name["type"] = "string";

    JsonObject action_payload =
            command.createNestedObject("action_payload");

    action_payload["type"] = "string";

    shelly.addAction(&shelly_action);
}



void ShellyManager::reportAsyncEnergyAndPower(float power, float energy) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    power = deadzone(power, POWER_MEASUREMENT_DEADZONE_W);
    energy = deadzone(energy, ENERGY_MEASUREMENT_DEADZONE_W);
    this->energy1 = energy;
    this->power1 = power;
    String updated_props[2] = {"power1", "energy1"};
    this->updateState(updated_props, 2);

}

void ShellyManager::reportPeriodicEnergyAndPower25(float power1, float energy1, float power2, float energy2) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    auto pow1 = deadzone(power1, POWER_MEASUREMENT_DEADZONE_W);
    auto ene1 = deadzone(energy1, ENERGY_MEASUREMENT_DEADZONE_W);

    auto pow2 = deadzone(power2, POWER_MEASUREMENT_DEADZONE_W);
    auto ene2 = deadzone(energy2, ENERGY_MEASUREMENT_DEADZONE_W);

    this->energy1 = ene1;
    this->power1 = pow1;

    this->energy2 = ene2;
    this->power2 = pow2;

    String updated_props[4] = {"power1", "energy1", "power2", "energy2"};
    this->updateState(updated_props, 4);

}


void ShellyManager::reportPeriodicEnergyAndPower1(float power1, float energy1) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    auto power = deadzone(power1, POWER_MEASUREMENT_DEADZONE_W);
    auto energy = deadzone(energy1, ENERGY_MEASUREMENT_DEADZONE_W);
    this->energy1 = energy;
    this->power1 = power;
    String updated_props[2] = {"power1", "energy1"};
    this->updateState(updated_props, 2);

}

#if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
void ShellyManager::reportPeriodicEnergyAndPower2(float power2, float energy2) {

    auto power = deadzone(power2, POWER_MEASUREMENT_DEADZONE_W);
    auto energy = deadzone(energy2, ENERGY_MEASUREMENT_DEADZONE_W);
    this->energy2 = energy;
    this->power2 = power;
    String updated_props[2] = {"power2", "energy2"};
    this->updateState(updated_props, 2);

}
#endif

void ShellyManager::setTemperature(float temperature) {
    #ifdef SHELLY_1_PLUS
    Lockguard l(this->stateMutex);
    #endif
    this->temperature = temperature;
    String updated_props[1] = {"ambient_temperature"};
    this->updateState(updated_props, 1);

}

const int32_t &ShellyManager::getRssi() const {
    return rssi;
}

const String &ShellyManager::getApMacAddress() const {
    return ap_mac_address;
}



const String &ShellyManager::getDeviceName() const {
    return deviceName;
}


bool ShellyManager::persistRitenitiveOnFlash() {

    Serial.println("opening file");
    Serial.flush();

    this->ritenitiveFile = SPIFFS.open(RITENITIVE_FILE, "w");

    Serial.println("file open");
    Serial.flush();

    if (!this->ritenitiveFile) {
        LogUDP(F("Error while saving LittleFS (unable to open file)"));
        return false;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [this](void *) {
        this->ritenitiveFile.close();
    });


    Serial.println("creating d");
    Serial.flush();
    DynamicJsonDocument d{512};

    Serial.println("d created");
    Serial.flush();

    d["output1"] = this->output1;
    d["output2"] = this->output2;
    d["shutter_status"] = this->shutterStatus;
    d["dimmer"] = this->dimmerStatus;
    d["led_dimmer1"] = this->rgbwStatus.r;
    d["led_dimmer2"] = this->rgbwStatus.g;
    d["led_dimmer3"] = this->rgbwStatus.b;
    d["led_dimmer4"] = this->rgbwStatus.w;


    Serial.println("d compiled");
    Serial.flush();

    String data_dump;
    int ret = serializeJsonPretty(d, data_dump);

    Serial.println("RET PERSIST:");
    Serial.println(ret);


    #ifndef ESP32
    this->ritenitiveFile.write(data_dump.c_str());
    #else
    this->ritenitiveFile.write((const uint8_t*)  data_dump.c_str(), data_dump.length());
    #endif

    this->ritenitiveFile.flush();

    return true;

}

void ShellyManager::change_mode_action_handler(const ShellyOutputMode& mode, bool inverted) {


       #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_DIMMER) || defined(SHELLY_RGBW) || defined(SHELLY_EM)
       if (mode == ShellyOutputMode::SHUTTER) return;
       #endif

       #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_RGBW) || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
       if (mode == ShellyOutputMode::DIMMER) return;
       #endif

       #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_DIMMER) || defined(SHELLY_25) || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
       if (mode == ShellyOutputMode::RGBW || mode == ShellyOutputMode::LED_DIMMER) return;
       #endif

       #if defined(SHELLY_DIMMER) || defined(SHELLY_RGBW)
       if (mode == ShellyOutputMode::RELAY) return;
       #endif

        auto hw_conf_file = SPIFFS.open(HW_CONFIG_FILE, "w");


        if (!hw_conf_file) {
            LogUDP(F("Error while saving LittleFS (unable to open file)"));
            return;
        }

        std::shared_ptr<void> defer_file_closing(nullptr, [&](void *) {
            LogUDP(F("HW_CONFIG_FILE: closing"));
            hw_conf_file.close();
        });

        DynamicJsonDocument d{512};

        d.createNestedObject("mode");
        d["mode"] = mode;

        #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
        d.createNestedObject("inverted");
        d["inverted"] = inverted;
        #endif

        String data_dump;
        serializeJsonPretty(d, data_dump);

        LogUDP("Saving:" + data_dump);

        #ifndef ESP32
        hw_conf_file.write(data_dump.c_str());
        #else
        hw_conf_file.write((const uint8_t*) data_dump.c_str(), data_dump.length());
        #endif

        hw_conf_file.flush();

        LogUDP("Ask for Reboot ....");
        delay(2000);

        ShellyManager::shouldReboot = true;

}



void ShellyManager::pulse_action_handler(const int& port, const int& duration_ms, const bool& pulse_down){

    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::RELAY) return;

    PulseAction act;
            act.toExecute = true;
            act.durationMs = duration_ms;
            act.finalState = pulse_down;

            if (port == 1) {
                HardwareController::getInstance().pulseAction1 = act;
                HardwareController::getInstance().pulseAction1.timeout.start();
            } else if (port == 2 ){
                HardwareController::getInstance().pulseAction2 = act;
                HardwareController::getInstance().pulseAction2.timeout.start();
            }

            HardwareController::getInstance().setRelay(port, !pulse_down);

}


void ShellyManager::change_wifi_action_handler(const String& wifi_ssid, const String& wifi_password){

            auto conf_file = SPIFFS.open(WIFI_CONFIG_FILE, "w");

            if (!conf_file) {
                LogUDP(F("Error while saving LittleFS (unable to open file)"));
                return;
            }

            std::shared_ptr<void> defer_file_closing(nullptr, [&](void *) {
                LogUDP(F("WIFI_CONFIG_FILE: closing"));
                conf_file.close();
            });

            DynamicJsonDocument d{512};

            d.createNestedObject("wifi");
            d["wifi"]["wifi_ssid"] = wifi_ssid;
            d["wifi"]["wifi_password"] = wifi_password;

            String data_dump;
            serializeJsonPretty(d, data_dump);

            #ifndef ESP32
            conf_file.write(data_dump.c_str());
            #else
            conf_file.write((const uint8_t*)  data_dump.c_str(), data_dump.length());
            #endif

            conf_file.flush();

            LogUDP("Ask for Reboot ....");

    ShellyManager::shouldReboot = true;


}


void ShellyManager::shelly_action_handler(const JsonVariant &input) {

    LogUDP("SHELLY_ACTION");

    #ifndef ESP32
    auto now_heap = system_get_free_heap_size();

    Serial.println("HEAP:" + String(now_heap));

    Serial.flush();
    #endif


    String out;
    serializeJson(input, out);
    Serial.println(out);


    JsonObject inputObj = input.as<JsonObject>();

    if (inputObj.containsKey("action")){
        auto action = inputObj["action"];
        if (action.containsKey("action_name") && action.containsKey("action_payload")){

            String action_name = action["action_name"].as<String>();
            String action_payload = action["action_payload"].as<String>();


            if (
                    action_name != "get_status_update" && action_name != "set_output" && action_name != "set_dimmer" && action_name != "pulse_action" &&
                    action_name != "change_wifi" && action_name != "change_mode" && action_name != "update_action" &&
                    action_name != "set_shutter" && action_name != "set_rgbw" && action_name != "set_led_dimmer"
                    && action_name != "control_radiator_valve"
            )
                return;


            DynamicJsonDocument action_payload_obj {1024};

            auto ret = deserializeJson(action_payload_obj, action_payload);

            if (ret != DeserializationError::Code::Ok){
                 LogUDP("Error parsing action_payload");
                 Serial.flush();
                 return;
            }

            LogUDP("action_name: " + action_name + " action_payload " + action_payload);

            if (action_name == "get_status_update") {
                ShellyManager::getInstance().updateState(nullptr, 0);
            }


                if (action_name == "set_output") {
                if(action_payload_obj.containsKey("output_number") && action_payload_obj.containsKey("value")) {
                    int port = action_payload_obj["output_number"];
                    bool newValue = action_payload_obj["value"];
                    ShellyManager::set_output_action_handler(port, newValue);
                }
            }

            if (action_name == "set_dimmer") {
                if(action_payload_obj.containsKey("dim_value")) {
                    int dim_value = action_payload_obj["dim_value"];
                    ShellyManager::dim_action_handler(dim_value);
                }
            }

            if (action_name == "pulse_action") {
                if(action_payload_obj.containsKey("output_number") &&
                        action_payload_obj.containsKey("duration_ms") &&
                        action_payload_obj.containsKey("pulse_down")
                        ) {
                    int port = action_payload_obj["output_number"];
                    int duration_ms = action_payload_obj["duration_ms"];
                    bool pulse_down = action_payload_obj["pulse_down"];

                    ShellyManager::pulse_action_handler(port, duration_ms, pulse_down);
                }
            }

            if (action_name == "change_wifi") {
                if(action_payload_obj.containsKey("wifi_ssid") &&
                   action_payload_obj.containsKey("wifi_password")
                        ) {
                    String wifi_ssid = action_payload_obj["wifi_ssid"];
                    String wifi_password = action_payload_obj["wifi_password"];
                    ShellyManager::change_wifi_action_handler(wifi_ssid, wifi_password);
                }
            }

            if (action_name == "change_mode") {
                if(action_payload_obj.containsKey("mode")
                        ) {
                    ShellyOutputMode mode = action_payload_obj["mode"].as<ShellyOutputMode>();

                    if(action_payload_obj.containsKey("inverted")
                            ) {
                        bool inverted = action_payload_obj["inverted"].as<bool>();
                        ShellyManager::change_mode_action_handler(mode, inverted);
                    } else {
                        ShellyManager::change_mode_action_handler(mode);
                    }

                }



            }

            if (action_name == "update_action") {
                if(action_payload_obj.containsKey("firmware_url") &&
                   action_payload_obj.containsKey("firmware_version")
                        ) {
                    String firmware_url = action_payload_obj["firmware_url"];
                    String firmware_version = action_payload_obj["firmware_version"];
                    ShellyManager::update_action_handler(firmware_url, firmware_version);
                }
            }

            if (action_name == "set_shutter") {
                if(action_payload_obj.containsKey("shutter_command")
                        ) {
                    ShutterCommand command = action_payload_obj["shutter_command"].as<ShutterCommand>();
                    ShellyManager::shutter_action_handler(command);
                }
            }

            if (action_name == "set_rgbw") {
                if(action_payload_obj.containsKey("rgbw_status")
                        ) {
                    int r_value = action_payload_obj["rgbw_status"]["r_value"].as<int>();
                    int g_value = action_payload_obj["rgbw_status"]["g_value"].as<int>();
                    int b_value = action_payload_obj["rgbw_status"]["b_value"].as<int>();
                    int w_value = action_payload_obj["rgbw_status"]["w_value"].as<int>();

                    RGBWStatus s;
                    s.setRGBW(r_value,g_value,b_value,w_value);
                    ShellyManager::rgbw_action_handler(s);
                }
            }

            if (action_name == "set_led_dimmer") {
                if(action_payload_obj.containsKey("led_dimmer_status")
                        ) {
                    String channel = action_payload_obj["led_dimmer_status"]["channel"].as<String>();
                    int value = action_payload_obj["led_dimmer_status"]["value"].as<int>();
                    ShellyManager::led_dimmer_action_handler(channel, value);
                }
            }


            if (action_name == "control_radiator_valve") {
                if(action_payload_obj.containsKey("mac_address") && action_payload_obj.containsKey("value")) {
                    bool newValue = action_payload_obj["value"];
                    String mac = action_payload_obj["mac_address"].as<String>();
                    #ifdef SHELLY_1_PLUS
                    BleManager::getInstance().controlRadiator(mac, newValue);
                    #endif
                }
            }



        }
    }





}


void ShellyManager::get_mode(){

    #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25)  || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
    this->mode = ShellyOutputMode::RELAY;
    #endif

    #if defined(SHELLY_DIMMER)
    this->mode = ShellyOutputMode::DIMMER;
    #endif

    #if defined(SHELLY_RGBW)
    this->mode = ShellyOutputMode::LED_DIMMER;
    #endif


    auto file = SPIFFS.open(HW_CONFIG_FILE, "r");

    if (!file) {
        LogUDP("HW_CONFIG_FILE: file not found");
        return;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [&file](void *) {
        LogUDP(F("HW_CONFIG_FILE: closing"));
        file.close();
    });


    size_t size = file.size();
    if (!size) {
        LogUDP("HW_CONFIG_FILE: file size=0");
        return;
    }

    DynamicJsonDocument loaded_json{512};

    LogUDP("HW_CONFIG_FILE: parsing");

    auto res = deserializeJson(loaded_json, file);

    if (res != DeserializationError::Ok) {
        LogUDP("HW_CONFIG_FILE:  bad Json");
        return;
    }

    if (loaded_json.containsKey("mode")) {
        this->mode = loaded_json["mode"].as<ShellyOutputMode>();
        LogUDP("Reading:" + String(mode));
    }

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    if (loaded_json.containsKey("inverted")) {
        this->inverted = loaded_json["inverted"].as<bool>();
    }
    #endif

    return;

}


void ShellyManager::shutter_action_handler(const ShutterCommand& shutter_command) {

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)

    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::SHUTTER) return;

    auto inverted = ShellyManager::getInstance().inverted;

    ShutterRelayCommand sc;

    if (shutter_command == ShutterCommand::OPEN){
        if (!inverted){
            sc = ShutterRelayCommand::COMMAND_OPEN;
        }
        else {
            sc = ShutterRelayCommand::COMMAND_CLOSE;
        }
    } else {
        if (shutter_command == ShutterCommand::CLOSE){
            if(!inverted){
                sc = ShutterRelayCommand::COMMAND_CLOSE;
            }
            else {
                sc = ShutterRelayCommand::COMMAND_OPEN;
            }
        }
        else {
            if (shutter_command == ShutterCommand::STOP) {
                sc = ShutterRelayCommand::COMMAND_STOP;
            }
            else return;
        }
    }

    HardwareController::getInstance().setShutter(sc);

    #endif
}

void ShellyManager::dim_action_handler(const uint32_t& dim_value){

    #ifdef SHELLY_DIMMER
    LogUDP("Received dim_action_command");
    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::DIMMER) return;

    LogUDP("calling setDimmer");

    HardwareController::getInstance().setDimmer(dim_value);
    #endif

}

void ShellyManager::rgbw_action_handler(const RGBWStatus& rgbw){

    #ifdef SHELLY_RGBW
    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::RGBW) return;


    HardwareController::getInstance().setRGBW(rgbw);

    #endif


}



void ShellyManager::led_dimmer_action_handler(const String& channel, const uint32_t& dim_value){

    #ifdef SHELLY_RGBW

    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::LED_DIMMER) return;

    int port_number = 0;

    if (channel == "r") port_number = 1;
    if (channel == "g") port_number = 2;
    if (channel == "b") port_number = 3;
    if (channel == "w") port_number = 4;

    HardwareController::getInstance().setLedDimmer(port_number, dim_value);

    #endif

}


void ShellyManager::set_output_action_handler(const int& port, const bool& newValue){

    auto mode = ShellyManager::getInstance().mode;

    if (mode != ShellyOutputMode::RELAY) return;

    HardwareController::getInstance().setRelay(port, newValue);
}

bool ShellyManager::isOutput1() const {
    return output1;
}

bool ShellyManager::isOutput2() const {
    return output2;
}


#ifdef SHELLY_EM
void ShellyManager::reportPeriodicEnergyAndPower_ShellyEM(PowerData data) {
    this->powerData = data;
    String updated_props[1] = {"power_data"};
    this->updateState(updated_props, 1);
}
#endif

#ifdef SHELLY_RGBW
void ShellyManager::ledDimmerChanged(uint32_t portNumber, int dimmerValue){

    LogUDP("ledDimmerChanged ShellyManager" + String(portNumber) + " " + String(dimmerValue));

    if(portNumber == 1) {
        ShellyManager::getInstance().rChannelStatusChanged(dimmerValue);
    }

    if(portNumber == 2) {
        ShellyManager::getInstance().gChannelStatusChanged(dimmerValue);
    }

    if(portNumber == 3) {
        ShellyManager::getInstance().bChannelStatusChanged(dimmerValue);
    }

    if(portNumber == 4) {
        ShellyManager::getInstance().wChannelStatusChanged(dimmerValue);
    }

}
#endif

#ifdef SHELLY_DIMMER
void ShellyManager::dimmerChanged(uint32_t dimValue) {
    if (dimValue == this->dimmerStatus)
        return;
    this->dimmerStatus = dimValue;
    String updated_props[1] = {"dimmer_status"};
    this->updateState(updated_props, 1);
}
#endif
