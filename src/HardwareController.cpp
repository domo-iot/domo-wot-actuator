
#include "HardwareController.h"

float HardwareController::getMCUTemperature(){
    #ifndef ESP32
    float Vref = 1.0, Rup=32000, Rntc_base = 10000, Vcc=3.3, T1=298.15, Beta=3350;

    float Vntc = (float) analogRead(0)/1023*Vref; // NTC voltage


    float Rntc = (Vntc * Rup)/(Vcc - Vntc);

    float T2 = Beta/( log(Rntc/(Rntc_base*exp(-Beta/T1))) );

    return T2-273.15; // celsius
    #else
        return 0;
    #endif

}


void HardwareController::loop() {

    // manage external temperature sensors (DS18B20)
    #if defined(SHELLY_1) || defined(SHELLY_1PM)
    this->handle_ambienttemp_reading();
    #endif

    // manage power readings
    #if defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_EM) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_2PM_PLUS)
    this->powerMeasurementController.loop();
    #endif

    // manage pulse actions
    #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_2PM_PLUS)
    this->handle_pulseactions();
    #endif

    // manage inputs
    #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_DIMMER) || defined(SHELLY_RGBW) || defined(SHELLY_2PM_PLUS)
    this->handle_inputs();
    #endif


    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    if (ShellyManager::getInstance().mode == ShellyOutputMode::SHUTTER) {
        this->shutterLoop();
        this->shutterRelayLoop();
    }
    #endif

    #ifdef SHELLY_DIMMER
    this->shellyDimmer.loop();
    this->energyLoop();
    #endif

    #ifdef SHELLY_1_PLUS
    if (this->toReport != "") {
        ShellyManager::getInstance().updateBeaconAdv(this->toReport.c_str());
        this->toReport = "";
    }
    #endif

}

void HardwareController::handle_inputs() {

    #if defined(SHELLY_1) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_RGBW) || defined(SHELLY_DIMMER) || defined(SHELLY_2PM_PLUS)
    auto new_state1 = digitalRead(INPUT_1_PIN);

    if (new_state1 != pinState1) {
        inputTimer1.start();
        pinState1 = new_state1;
        inputHandled1 = false;
    }

    if (!inputHandled1 && \
             inputTimer1.elapsed(INPUT_DEBOUNCE_TIME_MS, false)) {

        ShellyManager::getInstance().inputChanged(1, pinState1);
        inputHandled1 = true;
    }
    #endif

    #if defined(SHELLY_25) || defined(SHELLY_DIMMER) || defined(SHELLY_2PM_PLUS)
    auto new_state2 = digitalRead(INPUT_2_PIN);

    if (new_state2 != pinState2) {
        inputTimer2.start();
        pinState2 = new_state2;
        inputHandled2 = false;
    }

    if (!inputHandled2 && \
             inputTimer2.elapsed(INPUT_DEBOUNCE_TIME_MS, false)) {

        ShellyManager::getInstance().inputChanged(2, pinState2);
        inputHandled2 = true;
    }
    #endif

}

void HardwareController::handle_ambienttemp_reading() {

    #if defined(SHELLY_1) || defined(SHELLY_1PM) || defined(SHELLY_1PM_PLUS)
    this->searchDs18b20Sensors();

    if (temperatureTimer.elapsed(10000)) {
        Ds18x20EverySecond();
        Ds18x20Show();
    }
    #endif
}

void HardwareController::handle_pulseactions() {
    #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_2PM_PLUS)
    if (pulseAction1.toExecute && pulseAction1.timeout.elapsed(pulseAction1.durationMs)) {
        pulseAction1.toExecute = false;
        pulseAction1.timeout.stop();
        setRelay(1, pulseAction1.finalState);
    }
    #endif

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    if (pulseAction2.toExecute && pulseAction2.timeout.elapsed(pulseAction2.durationMs)) {
        pulseAction2.toExecute = false;
        pulseAction2.timeout.stop();
        setRelay(2, pulseAction2.finalState);
    }
    #endif
}

void HardwareController::searchDs18b20Sensors() const {
    #if defined(SHELLY_1) || defined(SHELLY_1PM) || defined(SHELLY_1PM_PLUS)
    static DomoTimer ds18b20SearchTimer;
    static int tries {0};

    if (!Ds18x20FoundSensors() && tries < 5) {
        if (ds18b20SearchTimer.elapsed(5000)) {
            Ds18x20Init();
            ++tries;
        }
    }
    #endif
}

void HardwareController::applySavedRitenitiveState() {

    auto file = SPIFFS.open(RITENITIVE_FILE, "r");


    if (!file) {
        LogUDP("applySavedRitenitiveState: file not found");
        return;
    }

    std::shared_ptr<void> defer_file_closing(nullptr, [&file](void *) {
        LogUDP(F("applySavedRitenitiveState: closing"));
        file.close();
    });


    size_t size = file.size();
    if (!size) {
        LogUDP("applySavedRitenitiveState: file size=0");
        return;
    }

    DynamicJsonDocument loaded_json{512};

    LogUDP("applySavedRitenitiveState: parsing");

    auto res = deserializeJson(loaded_json, file);

    if (res != DeserializationError::Ok) {
        LogUDP("applySavedRitenitiveState:  bad Json");
        return;
    }

    #ifdef SHELLY_1
    if (loaded_json.containsKey("output1") && loaded_json["output1"]) {
        this->setRelay(1, true);
    }
    #endif

    #ifdef SHELLY_1_PLUS
    if (loaded_json.containsKey("output1") && loaded_json["output1"]) {
        this->setRelay(1, true);
    }
    #endif


    #ifdef SHELLY_1PM
    if (loaded_json.containsKey("output1") && loaded_json["output1"]) {
        this->setRelay(1, true);
    }
    #endif

    #ifdef SHELLY_1PM_PLUS
    if (loaded_json.containsKey("output1") && loaded_json["output1"]) {
        this->setRelay(1, true);
    }
    #endif



    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    auto mode = ShellyManager::getInstance().mode;

    if (mode == ShellyOutputMode::RELAY){

        if(loaded_json.containsKey("output1") && loaded_json["output1"]) {
            this->setRelay(1, true);
        }

        if(loaded_json.containsKey("output2") && loaded_json["output2"]) {
            this->setRelay(2, true);
        }

    }
    else if (mode == ShellyOutputMode::SHUTTER){

        if (!loaded_json.containsKey("shutter_status"))
            return;

        ShellyManager::getInstance().shutterStatusChanged(
                (ShutterStatus) loaded_json["shutter_status"].as<uint32_t>()
        );

    }
    #endif


    #ifdef SHELLY_RGBW

    auto mode = ShellyManager::getInstance().mode;

    uint32_t r,g,b,w;

    for (auto i = 0; i < 4; ++i) {

        auto keyname = String("led_dimmer") + String(i + 1);

        if (loaded_json.containsKey(keyname) && loaded_json[keyname]) {
            auto value = loaded_json[keyname].as<uint32_t>();
            PWMManager::getInstance().set(i, value);

            if(mode == ShellyOutputMode::LED_DIMMER){
                ShellyManager::getInstance().ledDimmerChanged(i+1, value);
            } else {
                if (mode == ShellyOutputMode::RGBW){
                    if (i==1) r = value;
                    if (i==2) g = value;
                    if (i==3) b = value;
                    if (i==4) w = value;
                }
            }
        }
    }
    if (mode == ShellyOutputMode::RGBW){
        ShellyManager::getInstance().RGBWStatusChanged(r,g,b,w);
    }

    #endif

    #ifdef SHELLY_EM
    if (loaded_json.containsKey("output1") && loaded_json["output1"]) {
        this->setRelay(1, true);
    }
    #endif

    #ifdef SHELLY_DIMMER
    if(loaded_json.containsKey("dimmer") && loaded_json["dimmer"]) {
        this->setDimmer(loaded_json["dimmer"].as<uint32_t>());
    }
    #endif

}

#if defined(SHELLY_1_PLUS)
void HardwareController::setToReport(const String &toReport) {
    HardwareController::toReport = toReport;
}
#endif


#if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
void HardwareController::stopShutterWithoutUpdatingCurrentState() {
    digitalWrite(RELAY_1_PIN, false);
    digitalWrite(RELAY_2_PIN, false);
    limitSwitchTimeout.stop();
    relayShutterTimer.stop();
}


void HardwareController::shutterRelayLoop() {
    switch (this->relayShutterCommand) {

        case ShutterRelayCommand::COMMAND_NONE:
        case ShutterRelayCommand::COMMAND_STOP:
            return;

        case ShutterRelayCommand::COMMAND_OPEN:

            if (!this->relayShutterTimer.isRunning()) {
                digitalWrite(RELAY_2_PIN, false);
                this->relayShutterTimer.start();
                return;
            }

            if (this->relayShutterTimer.elapsed(1000)) {
                this->relayShutterTimer.stop();
                digitalWrite(RELAY_1_PIN, true);
                this->relayShutterCommand = ShutterRelayCommand::COMMAND_NONE;
                return;
            }

            break;

        case ShutterRelayCommand::COMMAND_CLOSE:

            if (!this->relayShutterTimer.isRunning()) {
                digitalWrite(RELAY_1_PIN, false);
                this->relayShutterTimer.start();
                return;
            }

            if (this->relayShutterTimer.elapsed(1000)) {
                this->relayShutterTimer.stop();
                digitalWrite(RELAY_2_PIN, true);
                this->relayShutterCommand = ShutterRelayCommand::COMMAND_NONE;
                return;
            }
            break;

    }
}

void HardwareController::shutterLoop() {

    if (desiredShutterState == currentShutterState)
        return;

    auto check_limit_switch_timeout = [&](uint32_t timeout, ShutterStatus stateIfTimeout) {
        if (this->limitSwitchTimeout.elapsed(timeout)) {
            this->currentShutterState = ShutterStatus::SHUTTER_STATUS_STOPPED;
            this->desiredShutterState = ShutterStatus::SHUTTER_STATUS_STOPPED;
            this->stopShutterWithoutUpdatingCurrentState();
            ShellyManager::getInstance().shutterStatusChanged(stateIfTimeout);
        }
    };

    auto timeout = DEFAULT_SHUTTER_TIMEOUT_MS;

    bool limitSwitchReadingConsumption = true;

    if (desiredShutterState == ShutterStatus::SHUTTER_STATUS_FULLY_OPENED) {
//        LogUDP("Should open");
        if (currentShutterState == ShutterStatus::SHUTTER_STATUS_OPENING) {

            if (!justActivatedRelayTimer.elapsed(JUST_ACTIVATED_TIMEOUT)) {
//                LogUDP("Waiting just_activated_timeout open");
                return;
            }

            justActivatedRelayTimer.stop();

            if (limitSwitchReadingConsumption) {
                auto power = this->powerMeasurementController.getSmoothedPower1();

                if (power > SHUTTER_POWER_THRESHOLD) {

                    check_limit_switch_timeout(timeout, ShutterStatus::SHUTTER_STATUS_OPENED);
                    return;

                } else {

                    this->stopShutterWithoutUpdatingCurrentState();
                    this->currentShutterState = ShutterStatus::SHUTTER_STATUS_FULLY_OPENED;
                    ShellyManager::getInstance().shutterStatusChanged(ShutterStatus::SHUTTER_STATUS_FULLY_OPENED);

                }

            } else {
                check_limit_switch_timeout(timeout, ShutterStatus::SHUTTER_STATUS_FULLY_OPENED);
            }


        } else {

            this->setShutterRelay(ShutterRelayCommand::COMMAND_OPEN);
            ShellyManager::getInstance().shutterStatusChanged(ShutterStatus::SHUTTER_STATUS_OPENING);
            this->currentShutterState = ShutterStatus::SHUTTER_STATUS_OPENING;

        }

        return;

    }

    if (desiredShutterState == ShutterStatus::SHUTTER_STATUS_FULLY_CLOSED) {
//        LogUDP("Should close");
        if (currentShutterState == ShutterStatus::SHUTTER_STATUS_CLOSING) {

            if (!justActivatedRelayTimer.elapsed(JUST_ACTIVATED_TIMEOUT)) {
//                LogUDP("Waiting just_activated_timeout close");
                return;
            }

            justActivatedRelayTimer.stop();

            if (limitSwitchReadingConsumption) {
                auto power = this->powerMeasurementController.getSmoothedPower2();

                if (power > SHUTTER_POWER_THRESHOLD) {
                    check_limit_switch_timeout(timeout, ShutterStatus::SHUTTER_STATUS_OPENED);
                    return;
                } else {

                    this->stopShutterWithoutUpdatingCurrentState();
                    this->currentShutterState = ShutterStatus::SHUTTER_STATUS_FULLY_CLOSED;
                    ShellyManager::getInstance().shutterStatusChanged(ShutterStatus::SHUTTER_STATUS_FULLY_CLOSED);

                }
            } else {
                check_limit_switch_timeout(timeout, ShutterStatus::SHUTTER_STATUS_FULLY_CLOSED);
            }


        } else {
            this->setShutterRelay(ShutterRelayCommand::COMMAND_CLOSE);
            ShellyManager::getInstance().shutterStatusChanged(ShutterStatus::SHUTTER_STATUS_CLOSING);
            this->currentShutterState = ShutterStatus::SHUTTER_STATUS_CLOSING;
        }

        return;

    }


}


void HardwareController::setShutterRelay(ShutterRelayCommand command) {
    this->relayShutterTimer.stop();
    this->relayShutterCommand = command;
}

void HardwareController::setShutter(ShutterRelayCommand cmd) {

    if (ShellyManager::getInstance().mode != ShellyOutputMode::SHUTTER) {
        return;
    }

    switch (cmd) {

        case ShutterRelayCommand::COMMAND_OPEN:
            //LogUDP("Received Open Command");
            this->desiredShutterState = ShutterStatus::SHUTTER_STATUS_FULLY_OPENED;
            limitSwitchTimeout.start();
            justActivatedRelayTimer.start();
            break;

        case ShutterRelayCommand::COMMAND_CLOSE:
            //LogUDP("Received Close Command");
            this->desiredShutterState = ShutterStatus::SHUTTER_STATUS_FULLY_CLOSED;
            limitSwitchTimeout.start();
            justActivatedRelayTimer.start();
            break;

        case ShutterRelayCommand::COMMAND_NONE:
            break;

        case ShutterRelayCommand::COMMAND_STOP:
            //LogUDP("Received Stop Command");
            this->desiredShutterState = ShutterStatus::SHUTTER_STATUS_STOPPED;
            this->stopShutterWithoutUpdatingCurrentState();
            this->currentShutterState = ShutterStatus::SHUTTER_STATUS_STOPPED;
            this->relayShutterCommand = ShutterRelayCommand::COMMAND_NONE;
            break;
    }

}


#endif


#ifdef SHELLY_DIMMER
void HardwareController::setDimmer(uint32_t dimmerValue, uint32_t rate, uint8_t func) {
    this->pendingDimValue = dimmerValue;

    if (rate != 0 && func != 0)
        this->shellyDimmer.set_dim_faded(dimmerValue, rate, func);
    else
        this->shellyDimmer.set_dim(dimmerValue);
}

void HardwareController::powerMeasured(double watt) {

    double now = millis();
    if (lastPowerMeasurementTimestamp < 1) {
        this->lastPowerMeasurementTimestamp = now;
        return;
    }

    this->energy += ((double) watt)*((double)(now - this->lastPowerMeasurementTimestamp)) / 3600000.0;
    this->power = watt;
    this->lastPowerMeasurementTimestamp = now;
}

void HardwareController::lastDimComplete() {
    ShellyManager::getInstance().dimmerChanged(this->pendingDimValue);
}

void HardwareController::energyLoop() {
    if (!this->energyReportingTimer.elapsed(DEFAULT_ENERGY_REPORT_INTERVAL*1000))
        return;

    ShellyManager::getInstance().reportPeriodicEnergyAndPower1(this->power, this->energy);

    this->energy = 0;

}

#endif

#ifdef SHELLY_RGBW


void HardwareController::setRGBW(RGBWStatus rgbw) {


    LogUDP("setRGBW " + String(rgbw.r) + " " + String(rgbw.g) + " "+ String(rgbw.b) + " " + String(rgbw.w));

    double r_d = (double) rgbw.r/255;
    double g_d = (double) rgbw.g/255;
    double b_d = (double) rgbw.b/255;

    int r = r_d * 100;
    int g = g_d * 100;
    int b = b_d * 100;

    LogUDP("setRGBW " + String(r) + " " + String(g) + " "+ String(b) + " " + String(rgbw.w));

    PWMManager::getInstance().set(0, r);
    PWMManager::getInstance().set(1, g);
    PWMManager::getInstance().set(2, b);
    PWMManager::getInstance().set(3, rgbw.w);

    ShellyManager::getInstance().RGBWStatusChanged(rgbw.r, rgbw.g, rgbw.b, rgbw.w);

}

void HardwareController::setLedDimmer(uint32_t portNumber, uint32_t dimmerValue) {

    LogUDP("setLedDimmer HW Controller" + String(portNumber) + String(dimmerValue));

    auto pwm_channel = portNumber - 1;

    PWMManager::getInstance().set(pwm_channel, dimmerValue);

    ShellyManager::getInstance().ledDimmerChanged(portNumber, dimmerValue);

}

#endif