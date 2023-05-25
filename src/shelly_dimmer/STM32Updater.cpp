#include "STM32Updater.h"
#include "utils.h"
#include "logging.h"
#ifndef ESP32
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>


static bool downloadAndSaveFile(const String& fileName, const String&  url){

    HTTPClient http;
    WiFiClient c;
    http.begin(c, url);

    // start connection and send HTTP header
    int httpCode = http.GET();

    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled

        File file = SPIFFS.open(fileName, "w");

        // file found at server
        if(httpCode == HTTP_CODE_OK) {

            // get lenght of document (is -1 when Server sends no Content-Length header)
            int len = http.getSize();

            // create buffer for read
            uint8_t buff[128] = { 0 };

            // get tcp stream
            WiFiClient * stream = http.getStreamPtr();

            // read all data from server
            while(http.connected() && (len > 0 || len == -1)) {
                // get available data size
                size_t size = stream->available();
                if(size) {
                    // read up to 128 byte
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    // write it to Serial
                    //Serial.write(buff, c);
                    file.write(buff, c);
                    if(len > 0) {
                        len -= c;
                    }
                }
                delay(1);
            }
            file.flush();
            file.close();

            if (len!=0 && len!=-1) {
                http.end();
                return false;
            }

        } else {
            http.end();
            return false;
        }

    } else {
        http.end();
        return false;
    }

    http.end();
    return true;
}

bool STM32Updater::updateSTM32() {
    LogUDP("STM32Updater::updateSTM32");

    if (SPIFFS.exists(STMFWUPDATEDFILE)) {
        return true;
    }

    if (!this->downloadFW())
        return false;

    if (!this->flashBegin())
        return false;

    File fw_binary_file = SPIFFS.open(F(STMFWPATH), "r");
    if (!fw_binary_file) {
        //LogUDP(" Error while opening STMFWPATH");
        return false;
    }


    size_t size = fw_binary_file.size();
    if (!size) {
        fw_binary_file.close();
        return false;
    }

    if (!this->flashUpload(fw_binary_file))
        return false;

    this->flashEnd();

    LogUDP("Writing STMFWUPDATEDFILE");

    File stm_fw_uploaded_file;
    stm_fw_uploaded_file = SPIFFS.open(STMFWUPDATEDFILE, "w");
    stm_fw_uploaded_file.close();

    LogUDP("STMFWUPDATEDFILE Written");

    return true;
}

bool STM32Updater::downloadFW() {

    if (SPIFFS.exists(STMFWPATH)) {
        //LittleFS.remove(STMFWPATH);
        Serial.println("Found smt32.bin file on fs");
        return true;
    }

    Serial.println("smt32.bin file not found on fs");
    return false;


    if (!downloadAndSaveFile(String(STMFWPATH), String(FWURL))) {
        //LogUDP("STM32Updater::downloadAndSaveFile ERROR");
        return false;
    };

    LogUDP("STM32Updater::downloadAndSaveFile OK");
    return true;

}


void STM32Updater::reset() {
    LogUDP("STM Reset to application mode");

    pinMode(STM_NRST_PIN, OUTPUT);
    pinMode(STM_BOOT0_PIN, OUTPUT);

    digitalWrite(STM_BOOT0_PIN, LOW); // boot stm from its own flash memory

    digitalWrite(STM_NRST_PIN, LOW); // start stm reset
    delay(50);

    digitalWrite(STM_NRST_PIN, HIGH); // end stm reset
    delay(50);
}

void STM32Updater::resetToDFUMode() {
    //LogUDP("STM: Entering DFU Mode");

    pinMode(STM_NRST_PIN, OUTPUT);
    digitalWrite(STM_NRST_PIN, LOW);

    pinMode(STM_BOOT0_PIN, OUTPUT);
    digitalWrite(STM_BOOT0_PIN, HIGH);

    delay(50);

    // clear in the receive buffer
    while (Serial.available())
        Serial.read();

    digitalWrite(STM_NRST_PIN, HIGH); // pull out of reset

    delay(50); // wait 50ms fot the co-processor to come online
}


bool STM32Updater::flashUpload(File& fwFile) {

    if (!this->stm32)
        return false;

    unsigned int len;

    auto size = fwFile.size();

    uint8_t buffer[256];

    uint32_t end = this->stm32Addr + size;

    LogUDP("Writing " + String(size) + " bytes to STM...");

    while (this->stm32Addr < end) {
        uint32_t left = end - this->stm32Addr;

        if (left < sizeof(buffer))
            len = left;
        else
            len = sizeof(buffer);

        auto ret = fwFile.readBytes((char*)buffer, len);

        if (ret <= 0) {
            this->stm32 = nullptr;
            this->stm32Addr = 0;
            return false;
        }

        stm32_err_t s_err = stm32_write_memory(this->stm32, this->stm32Addr, buffer, ret);

        if (s_err != STM32_ERR_OK) {
            // Error
            this->stm32 = nullptr;
            this->stm32Addr = 0;
            return false;
        }

        this->stm32Addr += ret;

        yield();

    }
    LogUDP("Write completed");
    return true;
}


bool STM32Updater::flashBegin() {
    LogUDP(" STM32Updater::flashBegin()");
    Serial.end();
    Serial.begin(115200, SERIAL_8E1);
    this->resetToDFUMode();

    this->stm32 = stm32_init(&Serial, STREAM_SERIAL, 1);
    this->stm32Addr = 0;

    if (this->stm32) {
        stm32_erase_memory(this->stm32, 0, STM32_MASS_ERASE);
        this->stm32Addr = this->stm32->dev->fl_start;
        LogUDP(" STM32Updater::flashBegin() OK");
        return true;
    } else {
        LogUDP(" STM32Updater::flashBegin() FAILED");
        return false;
    }
}

void STM32Updater::flashEnd() {

    if (this->stm32)
        stm32_close(this->stm32);

    this->stm32 = nullptr;

    this->stm32Addr = 0;

    Serial.end();
    this->reset();
    Serial.begin(115200, SERIAL_8N1);
}

STM32Updater::~STM32Updater() {

    if (this->stm32)
        stm32_close(this->stm32);

    this->stm32 = nullptr;

    this->stm32Addr = 0;
}

#endif