#ifdef SHELLY_1_PLUS
#include "BleManager.h"

void BleManager::set_temperature_mode() {
    Serial.println("Setting mode");
    Serial.println(current_pkt_number+1);

    if (!set_comfort_mode(pChr, current_pkt_number + 1, current_mode)) {
        this->valve_conn_state = ValveConnectionState::ERROR;
    } else {
        this->valve_conn_state = ValveConnectionState::SET_MODE;
    }
}

void BleManager::set_temperature_target(bool open) {
    Serial.println("Setting temp target");
    Serial.println(current_pkt_number+2);

    if (!set_comfort_temperature(pChr, current_pkt_number + 2, open)) {
        this->valve_conn_state = ValveConnectionState::ERROR;
    } else {
        this->valve_conn_state = ValveConnectionState::SET_TEMP;
    }
}


void BleManager::send_msg_for_pkt_number() {
    Serial.println("MSG FOR PKT NUM");
    Serial.println(this->current_pkt_number);
    Serial.flush();

    if (!send_message_for_pkt_number(this->pChr, this->current_pkt_number)) {
        this->valve_conn_state = ValveConnectionState::ERROR;
    }
}

void BleManager::send_msg_for_current_mode() {
    if(!send_message_for_current_mode(pChr, current_pkt_number +1)){
        this->valve_conn_state = ValveConnectionState::ERROR;
    }
}

bool BleManager::getServiceAndSubscribe() {

    this->valve_conn_state = SUBSCRIBING_TO_NOTIFYCHAR;

    NimBLERemoteService *pSvc = nullptr;

    pSvc = pClient->getService("ffe0");

    if (pSvc) {     /** make sure it's not null */
        Serial.println("Got service 0000ffe0-0000-1000-8000-00805f9b34fb");
        Serial.flush();

        // notify char
        pChr = pSvc->getCharacteristic("0000ffe4-0000-1000-8000-00805f9b34fb");

        if (pChr) {     /** make sure it's not null */
            /** registerForNotify() has been deprecated and replaced with subscribe() / unsubscribe().
             *  Subscribe parameter defaults are: notifications=true, notifyCallback=nullptr, response=false.
             *  Unsubscribe parameter defaults are: response=false.
             */


            if (pChr->canNotify()) {

                if (!pChr->subscribe(true, notifyCB, false)) {
                    /** Disconnect if subscribe failed */
                    pClient->disconnect();
                    return false;
                }

            }
        }

        pChr = pSvc->getCharacteristic("0000ffe9-0000-1000-8000-00805f9b34fb");
        if (pChr) {     /** make sure it's not null */
            if (pChr->canWrite()) {
                this->valve_conn_state = ValveConnectionState::SUB_TO_NOTIFYCHAR;
            }
        }

    }
}


bool BleManager::connectToValve(const NimBLEAddress& macAddress){

    /** Check if we have a client we should reuse first **/
    if(NimBLEDevice::getClientListSize()) {
        /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        this->pClient = NimBLEDevice::getClientByPeerAddress(macAddress);
        if(this->pClient){
            if(!this->pClient->connect(macAddress, false)) {
                Serial.println("Reconnect failed");
                Serial.flush();
                this->valve_conn_state = ERROR;
                return false;
            }


            Serial.println("Reconnected client");
            Serial.flush();

            if(this->pClient->getRssi() < -75) {
                this->pClient->disconnect();
                Serial.println("Low rssi, disconnecting");
                Serial.flush();
                this->valve_conn_state = ERROR;
                return false;
            }


        }
            /** We don't already have a client that knows this device,
             *  we will check for a client that is disconnected that we can use.
             */
        else {
            this->pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if(!this->pClient) {
        if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.println("Max clients reached - no more connections available");
            Serial.flush();
            this->valve_conn_state = ERROR;
            return false;
        }

        this->pClient = NimBLEDevice::createClient();

        Serial.println("New client created");
        Serial.flush();

        this->pClient->setClientCallbacks(&clientCB, false);

        /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
         */


        pClient->setConnectionParams(6,160,0,1000);


        /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */

        if (!pClient->connect(macAddress, false)) {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            pClient->disconnect();
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Failed to connect, deleted client");
            Serial.flush();
            this->valve_conn_state = ERROR;
            return false;
        }
    }

    if(!pClient->isConnected()) {
        if (!pClient->connect(macAddress, false)) {
            Serial.println("Failed to connect");
            Serial.flush();
            this->valve_conn_state = ERROR;
            return false;
        }
    }

    return false;

}


void BleManager::controlRadiator(const String& macAddress, bool open){


    #ifdef SHELLY_1_PLUS
    Lockguard l(BleManager::getInstance().bleStateMutex);
    #endif

    Serial.println("VALVE OP");
    Serial.println(open);
    Serial.flush();

    // we cannot handle two concurrent valve operations
    if (this->operation_in_progress) return;

    this->operation_in_progress = true;

    this->valve_operation_total_attempts = 0;

    // we stop scanning for advertisements
    int tries = 0;
    while(!this->pScan->stop() && tries < 5) {
        Serial.println("Stopping scan tentative number ");
        Serial.println(tries);
        tries++;
    }

    if(tries == 5) {
        this->operation_in_progress = false;
        return;
    }

    this->scan_in_progress = false;

    this->reset_valve_variables();

    this->valve_operation_timer.start();
    this->desired_state.mac_address = macAddress;
    this->desired_state.open = open;

    Serial.println("CONTROL_RADIATOR_CALL_START");
    Serial.flush();
}


void BleManager::loop(){

#ifdef SHELLY_1_PLUS
    Lockguard l(BleManager::getInstance().bleStateMutex);
    #endif

    if(!this->operation_in_progress) {
        if(!this->scan_in_progress) {
            this->pScan->start(0, scanEndedCB);
            this->scan_in_progress = true;
        }

        return;
    };



    if(this->valve_conn_state == ERROR) {
        Serial.println("Valve operation error");
        Serial.flush();


        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }
    }

    if(this->valve_conn_state == JUST_STARTED && (this->valve_operation_total_attempts == 0 || this->valve_operation_timer.elapsed(5000, false))) {
        Serial.println("Valve operation JUST STARTED");
        Serial.println("Connection attempt number");
        Serial.println(this->valve_operation_total_attempts);
        Serial.flush();

        NimBLEAddress addr(this->desired_state.mac_address.c_str());
        this->valve_conn_state = CONNECTING;
        this->valve_operation_timer.start();
        this->connectToValve(addr);
    }

    if(this->valve_conn_state == CONNECTING && this->valve_operation_timer.elapsed(3000, false)) {

        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }

    }

    if(this->valve_conn_state == CONNECTED) {
        Serial.println("Valve operation CONNECTED");
        Serial.flush();

        this->valve_operation_timer.start();
        this->getServiceAndSubscribe();
    }

    if(this->valve_conn_state == SUBSCRIBING_TO_NOTIFYCHAR &&
            this->valve_operation_timer.elapsed(3000, false)){
        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }
    }

    if(this->valve_conn_state == SUB_TO_NOTIFYCHAR) {
        Serial.println("Valve operation SUB_TO_NOTIFYCHAR");
        Serial.flush();

        this->valve_conn_state = SEARCH_FOR_PKT_NUMBER;
        this->valve_operation_timer.start();
        this->attempts = 0;
        this->send_msg_for_pkt_number();
    }


    if(this->valve_conn_state == SEARCH_FOR_PKT_NUMBER && attempts > 5){
        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }

    } else {
        if (this->valve_conn_state == SEARCH_FOR_PKT_NUMBER &&
            this->valve_operation_timer.elapsed(500, true)) {
            this->current_pkt_number++;
            this->send_msg_for_pkt_number();
            this->attempts ++;
        }
    }


    if(this->valve_conn_state == GOT_PKT_NUMBER) {
        Serial.println("Valve operation GOT_PKT_NUMBER");
        Serial.flush();
        this->valve_conn_state = SEARCH_FOR_CURRENT_TEMP;
        this->valve_operation_timer.start();
        this->send_msg_for_current_mode();
    }

    if(this->valve_conn_state == SEARCH_FOR_CURRENT_TEMP &&
       this->valve_operation_timer.elapsed(2000, false)) {
        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }
    }

    if(this->valve_conn_state == GOT_CURRENT_TEMP && this->valve_operation_timer.elapsed(2000, false)){
        Serial.println("Valve operation GOT_CURRENT_TEMP");
        Serial.flush();
        this->valve_conn_state = SETTING_MODE;
        this->valve_operation_timer.start();
        this->set_temperature_mode();
    }

    if(this->valve_conn_state == SETTING_MODE &&
        this->valve_operation_timer.elapsed(2000, false)) {
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }
    }

    if(this->valve_conn_state == SET_MODE && this->valve_operation_timer.elapsed(2000, false)){
        Serial.println("Valve operation SET_MODE");
        Serial.flush();

        this->valve_conn_state = SETTING_TEMP;
        this->valve_operation_timer.start();
        this->set_temperature_target(this->desired_state.open);
    }

    if(this->valve_conn_state == SETTING_TEMP &&
       this->valve_operation_timer.elapsed(2000, false)) {
        this->valve_operation_total_attempts++;
        if(this->valve_operation_total_attempts == 3) {
            this->reset_valve_variables();
            this->operation_in_progress = false;
        } else {
            DesiredValveState desired_state;
            desired_state.open =  this->desired_state.open;
            desired_state.mac_address = this->desired_state.mac_address;
            this->reset_valve_variables();
            this->desired_state = desired_state;
            this->valve_conn_state = JUST_STARTED;
            this->valve_operation_timer.start();
        }
    }

    if(this->valve_conn_state == SET_TEMP && this->valve_operation_timer.elapsed(2000, false)) {
        Serial.println("Valve operation SET_TEMP");
        Serial.flush();


        this->operation_in_progress = false;

        ShellyManager::getInstance().updateValveOperation(
                 this->desired_state.mac_address + " " +
                                this->desired_state.open);

        this->reset_valve_variables();

    }

}

#endif