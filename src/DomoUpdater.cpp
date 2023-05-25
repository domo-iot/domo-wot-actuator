#ifndef ESP32
#include "ESP8266httpUpdate.h"
#else
#include <HTTPClient.h>
#include <Update.h>
#endif

#include "utils.h"
#include "DomoUpdater.h"
#include "defines.h"
#include "ShellyManager.h"


void updateFirmware(uint8_t *data, size_t len, int& currentLength, int& totalLength){
    Update.write(data, len);
    currentLength += len;
    // Print dots while waiting for update to finish
    //Serial.print('.');
    // if current length of written firmware is not equal to total firmware size, repeat
    if(currentLength != totalLength) return;
    Update.end(true);
    Serial.printf("\nUpdate Success, Total Size: %u\nRebooting...\n", currentLength);
    // Restart ESP32 to see changes
    ESP.restart();
}


void DomoUpdater::loop(){

    if(!this->fwVersionToSet.isEmpty() && this->updateRetries < this->MAX_UPDATE_RETRIES){
        if(this->timerUpdater.elapsed(5000)){
            this->update();
        }
    }

}


void DomoUpdater::setFwToUpdate(const String& url){

    //HardwareController::getInstance().setShutter(HardwareController::ShutterRelayCommand::COMMAND_STOP);

    this->fwVersionToSet = url;

    this->updateRetries = 0;

    uint32_t slotDurationMS = 100;

    long backoff = random(0, 600);

    //for (int i = 0; i < backoff; ++i) {
    //    delay(slotDurationMS);
    //}

    this->timerUpdater.start();


}

bool DomoUpdater::update() {

    //HardwareController::getInstance().setShutter(HardwareController::ShutterRelayCommand::COMMAND_STOP);

    WiFiClient c;

    #if defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_2PM_PLUS)
        HTTPClient client;
        client.begin(this->fwVersionToSet);
        // Get file, just to check if each reachable
        int resp = client.GET();
        Serial.print("Response: ");
        Serial.println(resp);
        int totalLength;
        int written_bytes = 0;

        if(resp == 200){
            // get length of document (is -1 when Server sends no Content-Length header)
            totalLength = client.getSize();
            // transfer to local variable
            int len = totalLength;
            // this is required to start firmware update process
            Update.begin(UPDATE_SIZE_UNKNOWN);
            Serial.printf("FW Size: %u\n",totalLength);
            // create buffer for read
            uint8_t buff[128] = { 0 };
            // get tcp stream
            WiFiClient * stream = client.getStreamPtr();
            // read all data from server
            Serial.println("Updating firmware...");
            while(client.connected() && (len > 0 || len == -1)) {
                // get available data size
                size_t size = stream->available();
                if(size) {
                    // read up to 128 byte
                    int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                    // pass to function
                    updateFirmware(buff, c, written_bytes, totalLength);
                    if(len > 0) {
                        len -= c;
                    }
                }
                delay(1);
        }
        }else{
            Serial.println("Cannot download firmware file.");
            this->updateRetries++;
        }
        client.end();

    #endif

    #ifndef ESP32
    auto ret = ESPhttpUpdate.update(c, this->fwVersionToSet, "DOMO_FW");


    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(),
                 ESPhttpUpdate.getLastErrorString().c_str());
            this->updateRetries++;
            return false;

        case HTTP_UPDATE_NO_UPDATES:
            LogUDP(F("HTTP_UPDATE_NO_UPDATES"));
            this->updateRetries++;
            return false;

        case HTTP_UPDATE_OK:
            this->fwVersionToSet = "";
            LogUDP(F("HTTP_UPDATE_OK"));
            LogUDP(F("Firmware Update Complete!"));
            ESP.restart();
            delay(1000);
            ESP.reset();
            return true;
    }
    return false;
    #endif

}

DomoUpdater::DomoUpdater() {
    this->currentFwVersion = FIRMWARE_VERSION;
    ShellyManager::getInstance().setFwVersion(this->currentFwVersion);
}
