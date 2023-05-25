//
// Created by domenico on 28/10/22.
//

#ifndef BLE_UTILS_H
#define BLE_UTILS_H

#include <cstdint>
#include "NimBLERemoteCharacteristic.h"

static uint8_t calculate_checksum(unsigned char* bytes, int len){

    int sum = 0;
    for(int i=3; i<len; i++){
        if(bytes[i] != 0x55) {
            Serial.println(bytes[i]);
            Serial.flush();
            sum += bytes[i];
        }
    }

    return sum & 0xFF;
}


static bool send_message_for_current_mode(NimBLERemoteCharacteristic* pChar, uint8_t packet_number) {

    unsigned char msg[7] = {0xAA, 0xAA, 0x07, 0x0C, 0x00, 0x00, packet_number};

    int len_msg = sizeof(msg) / sizeof(unsigned char);

    unsigned char checksum = calculate_checksum(msg, len_msg);

    unsigned char message[9];

    for (int i = 0; i < len_msg; i++) {
        message[i] = msg[i];
    }

    message[len_msg] = checksum;

    message[8] = '\0';


    if(pChar->writeValue(message, 8, false)){
        return true;
    }

    return false;

}



static bool send_message_for_pkt_number(NimBLERemoteCharacteristic* pChar, uint8_t packet_number) {

    unsigned char msg[7] = {0xAA, 0xAA, 0x07, 0x01, 0x00, 0x00, packet_number};

    int len_msg = sizeof(msg) / sizeof(unsigned char);

    unsigned char checksum = calculate_checksum(msg, len_msg);

    unsigned char message[9];

    for (int i = 0; i < len_msg; i++) {
        message[i] = msg[i];
    }

    message[len_msg] = checksum;

    message[8] = '\0';

    if(pChar->writeValue(message, 8,false)){
        return true;
    }

    return false;

}



static bool check_message_pkt_number(uint8_t*& current_response_bytes, uint8_t& current_response_bytes_len,
                                     uint8_t* bytes, size_t len, uint8_t& current_packet_number,
                                     uint8_t& current_mode) {



    char charServiceData[100];

    for (int i=0;i<len;i++) {
        sprintf(&charServiceData[i*2], "%02x", bytes[i]);
    }

    std::stringstream ss;
    ss << charServiceData;

    Serial.println("Received");
    Serial.println(ss.str().c_str());
    Serial.flush();

    if(current_response_bytes == nullptr && len < 2) {
        return false;
    }

    if(current_response_bytes == nullptr){
        Serial.println("HERE");
        if(bytes[0] == 170 && bytes[1] == 170){
            Serial.println("EMPTY AND AA AA found");
            Serial.println(len);
            Serial.flush();
            current_response_bytes = new uint8_t[len];
            memcpy(current_response_bytes, bytes, len);
            current_response_bytes_len = len;
            for(int i=3; i<current_response_bytes_len; i++) {
                if(current_response_bytes[i] == 0x55) {
                    return false;
                }
            }
            Serial.println("0x55 not found");
            Serial.flush();
        }
    } else {
        uint8_t* cur2 = new uint8_t[current_response_bytes_len + len];
        memcpy(cur2, current_response_bytes, current_response_bytes_len);
        memcpy(cur2 + current_response_bytes_len, bytes, len);
        delete [] current_response_bytes;
        current_response_bytes = cur2;
        current_response_bytes_len = current_response_bytes_len + len;
    }


    Serial.println("HERE 2");
    Serial.flush();

    uint8_t received_checksum = current_response_bytes[current_response_bytes_len - 1];

    Serial.println("RECEIVED CHECKSUM");
    Serial.println(received_checksum);
    Serial.flush();

    uint8_t expected_checksum = calculate_checksum(current_response_bytes, current_response_bytes_len - 1);

    Serial.println("EXPECTED CHECKSUM");
    Serial.println(expected_checksum);
    Serial.flush();


    if (current_response_bytes[3] != 255 && current_response_bytes[4] != 255 && expected_checksum == received_checksum) {

        Serial.println("Checksum ok");
        Serial.flush();

        if (current_response_bytes[3] == 0x01 && current_response_bytes[4] == 0x00 && current_response_bytes[5] == 0x00) {
            Serial.println("Response to pkt number");
            Serial.flush();

            uint8_t current_valve_packet_number = current_response_bytes[6];
            uint8_t current_valve_mode = current_response_bytes[len - 2];

            Serial.println("valve_pkt_number: ");
            Serial.println(current_valve_packet_number);

            Serial.println("valve_mode: ");
            Serial.println(current_valve_mode);
            Serial.flush();

            current_packet_number = current_valve_packet_number;
            current_mode = current_valve_mode;

            delete [] current_response_bytes;
            current_response_bytes = nullptr;
            current_response_bytes_len = 0;
            return true;
        }

    }

    current_response_bytes = nullptr;
    current_response_bytes_len = 0;
    return false;
}

static bool check_message_comfort_temp(uint8_t*& current_response_bytes, uint8_t& current_response_bytes_len, uint8_t* bytes, size_t len, uint8_t& current_packet_number, int& current_comfort_temp) {



    char charServiceData[100];

    for (int i=0;i<len;i++) {
        sprintf(&charServiceData[i*2], "%02x", bytes[i]);
    }

    std::stringstream ss;
    ss << charServiceData;

    Serial.println("Received");
    Serial.println(ss.str().c_str());
    Serial.flush();

    Serial.println("len");
    Serial.println(len);
    Serial.flush();

    if(current_response_bytes == nullptr && len < 2) {
        return false;
    }

    if(current_response_bytes == nullptr){
        if(bytes[0] == 170 && bytes[1] == 170){
            Serial.println("EMPTY AND AA AA found");
            Serial.println(len);
            Serial.flush();

            current_response_bytes = new uint8_t[len];
            memcpy(current_response_bytes, bytes, len);
            current_response_bytes_len = len;
            for(int i=3; i< current_response_bytes_len; i++) {
                if(current_response_bytes[i] == 0x55) {
                    return false;
                }
            }
            Serial.println("NO 0x55 found");
            Serial.println(len);
            Serial.flush();

        }
    } else {
        uint8_t* cur2 = new uint8_t[current_response_bytes_len + len];
        memcpy(cur2, current_response_bytes, current_response_bytes_len);
        memcpy(cur2 + current_response_bytes_len, bytes, len);
        delete [] current_response_bytes;
        current_response_bytes = cur2;
        current_response_bytes_len = current_response_bytes_len + len;
    }


    uint8_t received_checksum = current_response_bytes[current_response_bytes_len - 1];
    uint8_t expected_checksum = calculate_checksum(current_response_bytes, current_response_bytes_len - 1);

    if (current_response_bytes[3] != 255 && current_response_bytes[4] != 255 && expected_checksum == received_checksum) {

        Serial.println("Checksum ok");
        Serial.flush();

        if (current_response_bytes[3] == 0x0C && current_response_bytes[4] == 0x00 && current_response_bytes[5] == 0x00) {

            Serial.println("Response to get comfort temp");
            Serial.flush();

            int current_valve_comfort_temp = current_response_bytes[8] * 256 + current_response_bytes[7];

            uint8_t current_valve_packet_number = current_response_bytes[6];


            Serial.println("valve_pkt_number:");
            Serial.println(current_valve_packet_number);

            Serial.println("valve_comfort_temp:");
            Serial.println(current_valve_comfort_temp);

            current_packet_number = current_valve_packet_number;
            current_comfort_temp = current_valve_comfort_temp;
            delete [] current_response_bytes;
            current_response_bytes = nullptr;
            current_response_bytes_len = 0;
            return true;

        }

    }

    current_response_bytes = nullptr;
    current_response_bytes_len = 0;
    return false;
}


static bool set_comfort_temperature(NimBLERemoteCharacteristic* pChar, uint8_t packet_number, bool open) {
    uint8_t low, high;

    if (open) {
        low = (350 % 256) & 255;
        high = ((int) (350 / 256)) & 255;

    } else {
        low = 70 & 255;
        high = 0x00;
    }

    unsigned char msg[19] = {0xAA, 0xAA, 0x13, 0x0C, 0x00, 0x00, packet_number , low, high,
                             low, high, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    int len_msg = sizeof(msg) / sizeof(unsigned char);

    unsigned char checksum = calculate_checksum(msg, len_msg);

    unsigned char message[21];

    for (int i = 0; i < len_msg; i++) {
        message[i] = msg[i];
    }

    message[len_msg] = checksum;


    message[20] = '\0';

    if(pChar->writeValue(message, 20, false)){
        return true;
    }

    return false;

}

static bool set_comfort_mode(NimBLERemoteCharacteristic* pChar, uint8_t packet_number, uint8_t current_mode){
    uint8_t comfort_mode = 0x01;

    unsigned char msg[19] = {0xAA, 0xAA, 0x013, 0x01, 0x00, 0x00, packet_number, comfort_mode, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, current_mode};

    int len_msg = sizeof(msg) / sizeof(unsigned char);

    unsigned char checksum = calculate_checksum(msg, len_msg);

    unsigned char message[21];

    for (int i = 0; i < len_msg; i++) {
        message[i] = msg[i];
    }

    message[len_msg] = checksum;

    message[20] = '\0';

    if(pChar->writeValue(message, 20, false)){
        return true;
    }

    return false;


}
#endif
