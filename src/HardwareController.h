#ifndef DOMO_ESP_HARDWARECONTROLLER_H
#define DOMO_ESP_HARDWARECONTROLLER_H

#include <Arduino.h>
#include "ShellyManager.h"
#include "DomoTimer.h"
#include "utils.h"
#include "defines.h"
#include "logging.h"


#if defined(SHELLY_1) || defined(SHELLY_1PM) || defined(SHELLY_1PM_PLUS)
#include "DS18B20Dual.h"
#endif

#if defined(SHELLY_1PM)
#include "shelly_1_1pm/PowerMeasurementControllerHLW8012.h"
#endif

#if defined(SHELLY_1PM_PLUS)
#include "shelly_1pm_plus/PowerMeasurementControllerHLW8012.h"
#endif


#if defined(SHELLY_25) || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
#include "shelly_25/PowerMeasurementController_ADE7953.h"
#endif

#ifdef SHELLY_RGBW
#include "shelly_rgbw/PWMManager.h"
#endif

#ifdef SHELLY_DIMMER
#include "shelly_dimmer/cap_shelly_dimmer.h"
#endif

#ifdef SHELLY_DIMMER
class HardwareController : public shelly_dimmer::callback {
#else
class HardwareController {
#endif

public:


    void applySavedRitenitiveState();

    static HardwareController &getInstance() {
        static HardwareController instance;
        return instance;
    }

    void init_shelly_1_plus(){
        #ifdef SHELLY_1_PLUS
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(RELAY_1_PIN, OUTPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);

        this->applySavedRitenitiveState();
        #endif

    }

    void init_shelly_1pm_plus(){
    #ifdef SHELLY_1PM_PLUS
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(RELAY_1_PIN, OUTPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);

        this->applySavedRitenitiveState();
    #endif
    }


    void init_shelly_1(){
        #ifdef SHELLY_1
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(RELAY_1_PIN, OUTPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);

        this->applySavedRitenitiveState();

        Ds18x20Init();
        #endif
    }

    void init_shelly_1pm(){
        #ifdef SHELLY_1PM
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(RELAY_1_PIN, OUTPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);

        this->applySavedRitenitiveState();

        Ds18x20Init();
        #endif
    }

    void init_shelly_25(){
        #ifdef SHELLY_25

        // for preventing overheating GPIO 16 should be configured as INPUT
        pinMode(16, INPUT);
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(INPUT_2_PIN, INPUT);

        pinMode(RELAY_1_PIN, OUTPUT);
        pinMode(RELAY_2_PIN, OUTPUT);

        digitalWrite(RELAY_1_PIN, false);
        digitalWrite(RELAY_2_PIN, false);

        this->pinState1 = digitalRead(INPUT_1_PIN);
        this->pinState2 = digitalRead(INPUT_2_PIN);

        this->applySavedRitenitiveState();
        #endif
    }

    void init_shelly_2_pm_plus(){
        #ifdef SHELLY_2PM_PLUS
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(INPUT_2_PIN, INPUT);

        pinMode(RELAY_1_PIN, OUTPUT);
        pinMode(RELAY_2_PIN, OUTPUT);

        digitalWrite(RELAY_1_PIN, false);
        digitalWrite(RELAY_2_PIN, false);

        this->pinState1 = digitalRead(INPUT_1_PIN);
        this->pinState2 = digitalRead(INPUT_2_PIN);

        this->applySavedRitenitiveState();
        #endif
    }



    void init_shelly_rgbw(){
        #ifdef SHELLY_RGBW
        pinMode(INPUT_1_PIN, INPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);

        PWMManager::getInstance().init();

        delay(50);

        this->applySavedRitenitiveState();
        #endif
    }

    void init_shelly_em(){
        #ifdef SHELLY_EM
        pinMode(RELAY_1_PIN, OUTPUT);
        this->applySavedRitenitiveState();
        #endif
    }

    void init_shelly_dimmer(){
        #ifdef SHELLY_DIMMER
        pinMode(INPUT_1_PIN, INPUT);
        pinMode(INPUT_2_PIN, INPUT);

        this->pinState1 = digitalRead(INPUT_1_PIN);
        this->pinState2 = digitalRead(INPUT_2_PIN);

        this->applySavedRitenitiveState();

        this->lastPowerMeasurementTimestamp = millis();
        #endif
    }


    void init() {
        #ifdef SHELLY_1
        this->init_shelly_1();
        #endif

        #ifdef SHELLY_1_PLUS
        this->init_shelly_1_plus();
        #endif

        #ifdef SHELLY_1PM_PLUS
        this->init_shelly_1pm_plus();
        #endif

        #ifdef SHELLY_2PM_PLUS
        this->init_shelly_2_pm_plus();
        #endif



        #ifdef SHELLY_1PM
        this->init_shelly_1pm();
        #endif

        #ifdef SHELLY_25
        this->init_shelly_25();
        #endif

        #ifdef SHELLY_RGBW
        this->init_shelly_rgbw();
        #endif

        #ifdef SHELLY_EM
        this->init_shelly_em();
        #endif

        #ifdef SHELLY_DIMMER
        this->init_shelly_dimmer();
        #endif


    }

    void setRelay(uint8_t port, bool value) {

        #if defined(SHELLY_1) || defined(SHELLY_1_PLUS) || defined(SHELLY_1PM_PLUS) || defined(SHELLY_1PM) || defined(SHELLY_25) || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
        if (port == 1) {
            digitalWrite(RELAY_1_PIN, value);
            ShellyManager::getInstance().outputChanged(port, value);
        }
        #endif

        #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
        if (port == 2) {
            digitalWrite(RELAY_2_PIN, value);
            ShellyManager::getInstance().outputChanged(port, value);
        }
        #endif


    }

    void loop();

    HardwareController(HardwareController &&) = delete;

    HardwareController &operator=(const HardwareController &) = delete;

    HardwareController &operator=(HardwareController &&) = delete;

    virtual ~HardwareController() = default;

    float getMCUTemperature();

    PulseAction pulseAction1, pulseAction2;

    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    void setShutterRelay(ShutterRelayCommand command);
    void setShutter(ShutterRelayCommand cmd);
    #endif

    #ifdef SHELLY_DIMMER
    void setDimmer(uint32_t dimmerValue, uint32_t rate = 0, uint8_t func = 0);
    void energyLoop();
    void lastDimComplete();
    void powerMeasured(double watt) override;
    #endif

    #ifdef SHELLY_RGBW
    void setLedDimmer(uint32_t portNumber, uint32_t dimmerValue);
    void setRGBW(RGBWStatus rgbw);
    #endif


    #ifdef SHELLY_2PM_PLUS
    static uint8_t INPUT_1_PIN;
    #endif
private:

    void shutterRelayLoop();
    void stopShutterWithoutUpdatingCurrentState();

    void shutterLoop();



    HardwareController() = default;

    #ifdef SHELLY_1

    uint8_t RELAY_1_PIN{4};
    uint8_t INPUT_1_PIN{5};

    DomoTimer inputTimer1 {};
    DomoTimer inputTimer2 {};
    DomoTimer temperatureTimer {};

    bool pinState1{false};
    bool inputHandled1 {false};

    #endif

    #ifdef SHELLY_1_PLUS
    uint8_t RELAY_1_PIN{26};
    uint8_t INPUT_1_PIN{4};

    DomoTimer inputTimer1 {};
    DomoTimer inputTimer2 {};
    DomoTimer temperatureTimer {};

    bool pinState1{false};
    bool inputHandled1 {false};

    String toReport;
public:
    void setToReport(const String &toReport);

private:
#endif


    #ifdef SHELLY_1PM_PLUS
        uint8_t RELAY_1_PIN{26};
        uint8_t INPUT_1_PIN{4};

        DomoTimer inputTimer1 {};

        bool pinState1{false};
        bool inputHandled1 {false};

        DomoTimer temperatureTimer {};

        PowerMeasurementControllerHLW8012 powerMeasurementController;

    #endif



    #ifdef SHELLY_1PM
    uint8_t RELAY_1_PIN{15};
    uint8_t INPUT_1_PIN{4};

    DomoTimer inputTimer1 {};

    bool pinState1{false};
    bool inputHandled1 {false};

    DomoTimer temperatureTimer {};

    PowerMeasurementControllerHLW8012 powerMeasurementController;

    #endif

    #ifdef SHELLY_25

    uint8_t RELAY_1_PIN{4};
    uint8_t RELAY_2_PIN{15};

    uint8_t INPUT_1_PIN{13};
    uint8_t INPUT_2_PIN{5};

    DomoTimer inputTimer1 {};
    DomoTimer inputTimer2 {};

    bool pinState1{false};
    bool inputHandled1 {false};

    bool pinState2{false};
    bool inputHandled2 {false};

    PowerMeasurementControllerADE7953 powerMeasurementController;

    DomoTimer justActivatedRelayTimer{};

    DomoTimer relayShutterTimer{};

    DomoTimer limitSwitchTimeout{};

    ShutterRelayCommand relayShutterCommand {ShutterRelayCommand::COMMAND_NONE};

    ShutterStatus desiredShutterState{ShutterStatus::SHUTTER_STATUS_STOPPED};

    ShutterStatus currentShutterState{ShutterStatus::SHUTTER_STATUS_STOPPED};

    #endif


    #ifdef SHELLY_2PM_PLUS

    uint8_t RELAY_1_PIN{13};
    uint8_t RELAY_2_PIN{12};
    uint8_t INPUT_2_PIN{18};

    DomoTimer inputTimer1 {};
    DomoTimer inputTimer2 {};

    bool pinState1{false};
    bool inputHandled1 {false};

    bool pinState2{false};
    bool inputHandled2 {false};

    PowerMeasurementControllerADE7953 powerMeasurementController;

    DomoTimer justActivatedRelayTimer{};

    DomoTimer relayShutterTimer{};

    DomoTimer limitSwitchTimeout{};

    ShutterRelayCommand relayShutterCommand {ShutterRelayCommand::COMMAND_NONE};

    ShutterStatus desiredShutterState{ShutterStatus::SHUTTER_STATUS_STOPPED};

    ShutterStatus currentShutterState{ShutterStatus::SHUTTER_STATUS_STOPPED};

    #endif


    #ifdef SHELLY_RGBW
    uint8_t INPUT_1_PIN{5};

    DomoTimer inputTimer1 {};

    bool pinState1{false};
    bool inputHandled1 {false};

    #endif

    #ifdef SHELLY_DIMMER
    uint8_t INPUT_1_PIN{14};
    uint8_t INPUT_2_PIN{12};

    DomoTimer inputTimer1 {};
    DomoTimer inputTimer2 {};

    bool pinState1{false};
    bool inputHandled1 {false};

    bool pinState2{false};
    bool inputHandled2 {false};

    double pin1Average{0.00};
    double pin2Average{0.00};

    const double alpha{0.99};

    const double lowThresh{0.10};

    const double highThresh{0.4};

    shelly_dimmer shellyDimmer {this};

    uint32_t pendingDimValue {0};
    double energy {0};
    double power {0};
    DomoTimer energyReportingTimer{};
    unsigned long lastPowerMeasurementTimestamp {0};

    #endif

    #ifdef SHELLY_EM
    uint8_t RELAY_1_PIN{15};
    PowerMeasurementControllerADE7953 powerMeasurementController;
    #endif

    #ifdef SHELLY_2PM_PLUS
public:
    void setInput1Pin(uint8_t input1Pin);

private:
#endif


    void searchDs18b20Sensors() const;

    void handle_pulseactions();

    void handle_ambienttemp_reading();

    void handle_inputs();
};


#endif
