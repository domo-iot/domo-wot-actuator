#ifndef DOMO_ESP_UTILS_H
#define DOMO_ESP_UTILS_H

#include "DomoTimer.h"
#include "defines.h"
#include "ArduinoJson.h"
#include "logging.h"

template<class T>
inline T deadzone(T val, T threshold) {
    return (val > threshold) ? (val) : (0);
};

struct PulseAction {
    DomoTimer timeout {};
    uint32_t durationMs {500};
    bool finalState {false};
    bool toExecute {false};
};

enum ShellyOutputMode {
    RELAY, // relay mode
    SHUTTER, // shutter_action_handler mode
    DIMMER, // dimmer mode
    RGBW, // rgbw_action_handler mode
    LED_DIMMER // rgbw_action_handler channels used as separate led dimmers
};


enum ShutterCommand{
    OPEN,
    CLOSE,
    STOP
};


struct RGBWStatus{
    uint32_t r;
    uint32_t g;
    uint32_t b;
    uint32_t w;
    String string_representation;

    RGBWStatus(){
        this->r = 0;
        this->g = 0;
        this->b = 0;
        this->w = 0;
        this->to_string();
    }

    void setRGBW(uint32_t r, uint32_t g,uint32_t b,uint32_t w){
        this->r = r;
        this->g = g;
        this->b = b;
        this->w = w;
        this->to_string();
    }

    void setR(uint32_t r) {
        RGBWStatus::r = r;
        this->to_string();
    }

    void setG(uint32_t g) {
        RGBWStatus::g = g;
        this->to_string();
    }

    void setB(uint32_t b) {
        RGBWStatus::b = b;
        this->to_string();
    }

    void setW(uint32_t w) {
        RGBWStatus::w = w;
        this->to_string();
    }



    void to_string(){
        String ret;
        ret = "{ \"r\": " + String(r) +
                ",  \"g\": " + String(g) +
                ", \"b\": " + String(b) +
                ", \"w\": " + String(w) + "}";

        this->string_representation = ret;
    }
};


struct PowerData {


    struct PowerChannelData {
        float activePower {0.0}, reactivePower {0.0}, energy{0.0}, apparentEnergy {0.0};

        PowerChannelData& operator=(const PowerChannelData& other) {
            this->activePower = deadzone(other.activePower, POWER_MEASUREMENT_DEADZONE_W);
            this->reactivePower = deadzone(other.reactivePower, POWER_MEASUREMENT_DEADZONE_W);
            this->apparentEnergy = other.apparentEnergy;
            this->energy = other.energy;
            return *this;
        }

        DynamicJsonDocument to_json(){
            DynamicJsonDocument out{512};
            out["active_power"] = activePower;
            out["reactive_power"] = reactivePower;
            out["energy"] = energy;
            out["apparent_energy"] = apparentEnergy;

            return out;
        }

    };

    PowerChannelData channel1;

    PowerChannelData channel2;

    float voltage {0.0};

    DynamicJsonDocument to_json(){
        DynamicJsonDocument out{1024};
        out["channel1"] = this->channel1.to_json();
        out["channel2"] = this->channel2.to_json();
        out["voltage"]  = this->voltage;
        return out;
    }

};


enum ShutterStatus {
    SHUTTER_STATUS_UNDEFINED = 0,
    SHUTTER_STATUS_CLOSING = 1,
    SHUTTER_STATUS_FULLY_CLOSED = 2,
    SHUTTER_STATUS_OPENING = 3,
    SHUTTER_STATUS_OPENED = 4,
    SHUTTER_STATUS_FULLY_OPENED = 5,
    SHUTTER_STATUS_STOPPED=6
};

enum class ShutterRelayCommand {
    COMMAND_NONE,
    COMMAND_STOP,
    COMMAND_OPEN,
    COMMAND_CLOSE
};

#endif