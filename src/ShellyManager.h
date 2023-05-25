#ifndef DOMO_ESP_DOMOSTATE_H
#define DOMO_ESP_DOMOSTATE_H
#ifndef ESP32
#include "FS.h"
#endif

#ifdef ESP32
#include "SPIFFS.h"
#endif

#include "DomoUpdater.h"
#include <memory>
#include "utils.h"
#include "logging.h"
#ifndef ESP32
#include <ESP8266WebServerSecure.h>
#include "bearssl/WebSockets4WebServerSecure.h"
#endif
#include "domowebthings/Thing.h"
#include "domowebthings/WebThingAdapter.h"
#include "Mutex.h"

#include<vector>

class ShellyManager {
public:
    bool isOutput1() const;

    bool isOutput2() const;

public:

    static String getDeviceModel(){
      #if defined(SHELLY_1)
        return String("shelly_1");
      #endif

      #if defined(SHELLY_1_PLUS)
        return String("shelly_1plus");
      #endif

      #if defined(SHELLY_1PM_PLUS)
            return String("shelly_1pm_plus");
      #endif
      #if defined(SHELLY_2PM_PLUS)
        return String("shelly_2pm_plus");
      #endif
     #if defined(SHELLY_1PM)
        return String("shelly_1pm");
      #endif
      #if defined(SHELLY_25)
        return String("shelly_25");
      #endif


      #if defined(SHELLY_DIMMER)
        return String("shelly_dimmer");
      #endif

      #if defined(SHELLY_RGBW)
        return String("shelly_rgbw");
      #endif

      #if defined(SHELLY_EM)
        return String("shelly_em");
      #endif

    }


    String getTopicName(){
      #if defined(SHELLY_1)
        return String("shelly_1");
      #endif

      #if defined(SHELLY_1_PLUS)
        return String("shelly_1plus");
      #endif

        #if defined(SHELLY_1PM_PLUS)
                return String("shelly_1pm_plus");
        #endif
#if defined(SHELLY_2PM_PLUS)
        return String("shelly_2pm_plus");
#endif


#if defined(SHELLY_1PM)
        return String("shelly_1pm");
      #endif

      #if defined(SHELLY_25)
        return String("shelly_25");
      #endif

      #if defined(SHELLY_DIMMER)
        return String("shelly_dimmer");
      #endif

      #if defined(SHELLY_RGBW)
        return String("shelly_rgbw");
      #endif

      #if defined(SHELLY_EM)
        return String("shelly_em");
      #endif

    }

    // singleton
    ShellyManager(ShellyManager &&) = delete;

    ShellyManager &operator=(const ShellyManager &) = delete;

    ShellyManager &operator=(ShellyManager &&) = delete;

    static ShellyManager &getInstance() {
        static ShellyManager instance;
        return instance;
    }

    // current actuator output mode
    ShellyOutputMode mode;

    // in shutter mode, indicates if up/down are inverted
    bool inverted {false};

    // indicates that the actuator should reboot
    static bool shouldReboot;

    // indicates if an action is in progress
    static String action_in_progress;

    // generator of webthing actions

    static ThingActionObject *shelly_action_generator(DynamicJsonDocument *input) {


        ThingActionObject *new_thing_action_object = nullptr;
        new_thing_action_object = new ThingActionObject("shelly_action",
                                                            input,
                                                            &ShellyManager::shelly_action_handler,
                                                            nullptr);


        if(ShellyManager::action_in_progress != "") {
            Serial.print("Removing " + ShellyManager::action_in_progress);
            ShellyManager::getInstance().shelly.removeAction(ShellyManager::action_in_progress);
            ShellyManager::action_in_progress = "";
        }


        ShellyManager::action_in_progress = new_thing_action_object->id;

        return new_thing_action_object;

    }


    static void led_dimmer_action_handler(const String& channel, const uint32_t& dim_value);

    static void shutter_action_handler(const ShutterCommand& shutter_command);

    static void dim_action_handler(const uint32_t& dim_value);

    static void rgbw_action_handler(const RGBWStatus& rgbw);

    static void change_mode_action_handler(const ShellyOutputMode& mode, bool inverted = false);

    static void change_wifi_action_handler(const String& wifi_ssid, const String& wifi_password);

    static void shelly_action_handler(const JsonVariant &input);

    static void pulse_action_handler(const int& port, const int& duration_ms, const bool& pulse_down);

    static void update_action_handler(const String& firmware_url, const String& firmware_version);

    static void set_output_action_handler(const int& port, const bool& newValue);

    void loop();

    virtual ~ShellyManager(){};

    void setFwVersion(const String& version);

    void inputChanged(int inputNumber, bool value);

    void shutterStatusChanged(ShutterStatus newStatus);

    void dimmerStatusChanged(uint32_t newStatus);

    void updateBeaconAdv(const String& adv);

    void updateValveOperation(const String& valveOperation);

    void RGBWStatusChanged(uint32_t r, uint32_t g, uint32_t b, uint32_t w);

    void rChannelStatusChanged(uint32_t newStatus);

    void gChannelStatusChanged(uint32_t newStatus);

    void bChannelStatusChanged(uint32_t newStatus);

    void wChannelStatusChanged(uint32_t newStatus);

    void outputChanged(int outputNumber, bool value);

    void setNetworkInfo(const String &ip, const String &gw, const String &apMacAddress,
                        const String& wifiSSID, const String& wifiPassword);

    void setTemperature(float temperature);

    void reportAsyncEnergyAndPower(float power, float energy);

    void reportPeriodicEnergyAndPower25(float power1, float energy1, float power2, float energy2);


    void reportPeriodicEnergyAndPower1(float power1, float energy1);

#if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    void reportPeriodicEnergyAndPower2(float power2, float energy2);
    #endif

    #ifdef SHELLY_EM
    void reportPeriodicEnergyAndPower_ShellyEM(PowerData data);
    #endif

    #ifdef SHELLY_RGBW
    void ledDimmerChanged(uint32_t portNumber, int dimmerValue);
    #endif

    #ifdef SHELLY_DIMMER
    void dimmerChanged(uint32_t dimValue);
    #endif


    void setRSSI(int32_t rssi);


    void setMCUTemperature(float mcu_temperature);

    bool validTemperature() const {
        return this->temperature > -100;
    }

    const String &getApMacAddress() const;

    const int32_t &getRssi() const;

    const String &getDeviceName() const;


protected:

    #ifdef SHELLY_1_PLUS
    mutable Mutex stateMutex{};
    #endif

    void updateState(const String updated_properties[], uint16_t changed_props_num);

    String serialized_shelly_state = "";

    DynamicJsonDocument status{1024};

    std::vector<String> pending_updated_props;

    const char *shellyTypes[2] = {"shelly", nullptr};

    void get_mode();

    void add_shelly_action();

    File ritenitiveFile{};

    bool persistRitenitiveOnFlash();

    // mac_address
    String macAddress;

    // deviceName
    String deviceName {""};

    // Web Things adapter and device
    WebThingAdapter adapter;
    ThingDevice shelly;
    // Web Things status prop
    ThingProperty shellyStatusProp;

    // WebThings shelly_action
    StaticJsonDocument<256> shelly_action_json;
    ThingAction shelly_action;
    JsonObject shelly_action_input_obj;


    ShellyManager();


    bool input1{false}, input2 {false}, output1{false}, output2{false};
    ShutterStatus shutterStatus;

    uint32_t dimmerStatus;


    String beaconAdv;
    String valveOperation;

    PowerData powerData;

    RGBWStatus rgbwStatus;

    float power1{0.0}, energy1{0.0}, power2{0.0}, energy2{0.0};

    float mcuTemperature {-100};

    float temperature {-100};

    // network info
    String ip_address {}, gateway{}, ap_mac_address{}, wifi_ssid, wifi_password, fwVersion;
    int32_t rssi {};

    // return the ESP8266 module mac_address
    static String get_mac_address();

};




#endif
