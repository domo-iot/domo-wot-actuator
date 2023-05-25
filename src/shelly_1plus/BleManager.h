#ifdef SHELLY_1_PLUS
#ifndef DOMO_ESP_BLEMANAGER_H
#define DOMO_ESP_BLEMANAGER_H

#include "Arduino.h"
#include <NimBLEDevice.h>
#include <sstream>
#include <iomanip>
#include "ble_utils.h"
#include "ShellyManager.h"
#include "HardwareController.h"
#include <base64.h>


enum ValveConnectionState {
    ERROR,
    JUST_STARTED,
    CONNECTING,
    CONNECTED,
    SUBSCRIBING_TO_NOTIFYCHAR,
    SUB_TO_NOTIFYCHAR,
    SEARCH_FOR_PKT_NUMBER,
    GOT_PKT_NUMBER,
    SEARCH_FOR_CURRENT_TEMP,
    GOT_CURRENT_TEMP,
    SETTING_MODE,
    SET_MODE,
    SETTING_TEMP,
    SET_TEMP
};

struct DesiredValveState {
    String mac_address;
    bool open;
};

static void scanEndedCB(NimBLEScanResults results){
    Serial.println("Scan Ended");
}


/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) override {
        auto macAddress = advertisedDevice->getAddress().toString();
        auto rssi = advertisedDevice->getRSSI();

        String encoded = base64::encode(advertisedDevice->getPayload(), advertisedDevice->getPayloadLength());

        std::stringstream ss;

        ss << macAddress << " " << encoded.c_str() << " " << rssi << " " << advertisedDevice->getPayloadLength();
        Serial.println(ss.str().c_str());
        HardwareController::getInstance().setToReport(ss.str().c_str());
    };
};


class BleManager {
public:

    void set_temperature_mode();
    void set_temperature_target(bool open);
    void send_msg_for_current_mode();
    void send_msg_for_pkt_number();
    bool getServiceAndSubscribe();

    bool connectToValve(const NimBLEAddress& macAddress);


    void loop();
    void controlRadiator(const String& macAddress, bool open);

    static BleManager &getInstance() {
        static BleManager instance;
        return instance;
    }

    void init() {


        NimBLEDevice::init("");
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);



        this->pScan = NimBLEDevice::getScan();

        //** create a callback that gets called when advertisers are found *//*
        pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

        //** Set scan interval (how often) and window (how long) in milliseconds *//*
        pScan->setInterval(45);
        pScan->setWindow(15);

        pScan->setActiveScan(true);
        pScan->setDuplicateFilter(false);

        // 0 means infinite scan
        pScan->start(0, scanEndedCB);

        scan_in_progress = true;


    }



    void reset_valve_variables(){
        this->current_response_bytes = nullptr;
        this->current_response_bytes_len = 0;
        this->valve_operation_timer.stop();
        this->got_pkt_number = false;
        this->got_comfort_temp = false;
        this->current_pkt_number = 0x01;
        this->valve_conn_state = ValveConnectionState::JUST_STARTED;
        this->desired_state.mac_address = "";
        if(pClient != nullptr){
            NimBLEDevice::deleteClient(pClient);
            pClient = nullptr;
        }
    }

    bool operation_in_progress = false;
    bool scan_in_progress = false;
    NimBLEScan* pScan;
    bool got_pkt_number = false, got_comfort_temp=false;
    uint8_t current_pkt_number = 0x01;
    uint8_t current_mode;
    uint8_t* current_response_bytes = nullptr;
    uint8_t current_response_bytes_len = 0;

    int current_comfort_temp;

    int attempts = 0;

    int valve_operation_total_attempts = 0;

    ValveConnectionState valve_conn_state = ValveConnectionState::JUST_STARTED;
    DesiredValveState desired_state;
    NimBLEClient* pClient;
    DomoTimer valve_operation_timer;
    NimBLERemoteCharacteristic *pChr;

    #ifdef SHELLY_1_PLUS
    mutable Mutex bleStateMutex{};
    #endif



    BleManager(){
    }

};



/** Notification / Indication receiving handler callback */
static void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){

    #ifdef SHELLY_1_PLUS
    Lockguard l(BleManager::getInstance().bleStateMutex);
    #endif

    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    /** NimBLEAddress and NimBLEUUID have std::string operators */
    str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
    str += ", Value = " + std::string((char*)pData, length);
    Serial.println(str.c_str());

    if(BleManager::getInstance().valve_conn_state == ValveConnectionState::SEARCH_FOR_PKT_NUMBER) {
        if(check_message_pkt_number(BleManager::getInstance().current_response_bytes, BleManager::getInstance().current_response_bytes_len,  pData, length, BleManager::getInstance().current_pkt_number, BleManager::getInstance().current_mode)){
            BleManager::getInstance().got_pkt_number = true;
            BleManager::getInstance().valve_conn_state = GOT_PKT_NUMBER;
        }

    } else{
        if(BleManager::getInstance().valve_conn_state == ValveConnectionState::SEARCH_FOR_CURRENT_TEMP) {
            if (check_message_comfort_temp(BleManager::getInstance().current_response_bytes, BleManager::getInstance().current_response_bytes_len,  pData, length, BleManager::getInstance().current_pkt_number,
                                           BleManager::getInstance().current_comfort_temp)) {
                BleManager::getInstance().got_comfort_temp = true;
                BleManager::getInstance().valve_conn_state = GOT_CURRENT_TEMP;
            }
        }
    }

}




class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) override {
        Serial.println("Connected");
        BleManager::getInstance().valve_conn_state = CONNECTED;
    };

    void onDisconnect(NimBLEClient* pClient) override {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println("Disconnected - Starting scan");
    };

    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) override {
        return true;
    };

};

static ClientCallbacks clientCB;

#endif

#endif