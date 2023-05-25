//
// Created by domenico on 22/03/23.
//

#ifndef SHELLY_WEBTHINGS_ESP8266_MUTEX_H
#define SHELLY_WEBTHINGS_ESP8266_MUTEX_H

#ifdef SHELLY_1_PLUS
#include <freertos/semphr.h>
#include <freertos/FreeRTOS.h>
#include "Arduino.h"
#define ENABLE_DOUBLE_LOCK_DETECTION

class Mutex {

public:
    Mutex() {
        handle = xSemaphoreCreateMutex();
        if (this->handle == nullptr)
            while (1);
    }

    ~Mutex() {
        if (this->handle != nullptr) {
            if (this->isLocked()) {// semaforo preso
                this->lock(); // aspetta che si liberi
                this->unlock();
            }
            vSemaphoreDelete(this->handle);
        }
    }

    inline bool isLocked() {
        return uxSemaphoreGetCount(this->handle) == 0;
    }

    inline void lock() {

#ifdef ENABLE_DOUBLE_LOCK_DETECTION
        auto holder = xSemaphoreGetMutexHolder(this->handle);

        if (holder == xTaskGetCurrentTaskHandle()) {

            while (true) {
                Serial.println("DOUBLE LOCK DETECTED! Task is " + String(pcTaskGetTaskName(holder)) );
                delay(100);
            }
        }
#endif

        xSemaphoreTake(this->handle, 0);
    }

    inline void unlock() {
        xSemaphoreGive(this->handle);
    }

protected:
    SemaphoreHandle_t handle;

};


class Lockguard {
public:
    explicit Lockguard(Mutex &mutex) : mutex(mutex) {
        this->mutex.lock();
    }

    ~Lockguard() {
        this->mutex.unlock();
    }

protected:
    Mutex &mutex;


};
#endif
#endif //SHELLY_WEBTHINGS_ESP8266_MUTEX_H
