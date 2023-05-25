#include "PowerMeasurementController_ADE7953.h"
#include "Arduino.h"
#include "utils.h"

#include "defines.h"
#include "HardwareController.h"

#define MEASURE_PERIOD_MS                500
#define RELAY_QUICK_REPORT_PERIOD_MS     2000

#if defined(SHELLY_25) || defined(SHELLY_EM) || defined(SHELLY_2PM_PLUS)
PowerMeasurementControllerADE7953 *callbackInstancePtr{nullptr};
#endif


void PowerMeasurementControllerADE7953::loop() {
    #if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
    this->_readPowerShelly25();
    #endif

    #ifdef SHELLY_EM
    this->_readPowerShellyEM();
    #endif

}




double PowerMeasurementControllerADE7953::getPower1() {
    return this->lastPower1;
}

double PowerMeasurementControllerADE7953::getEnergy1() {
    auto out = this->energy1Sum;
    this->energy1Sum = 0;
    return out;
}

double PowerMeasurementControllerADE7953::getPower2() {
    return this->lastPower2;
}

double PowerMeasurementControllerADE7953::getEnergy2() {
    auto out = this->energy2Sum;
    this->energy2Sum = 0;
    return out;
}

#ifdef SHELLY_EM
void PowerMeasurementControllerADE7953::_readPowerShellyEM() {

        constexpr double correction_factor = (1.17/105.0);
        constexpr double energy_correction_factor = 0.255;

        if (!this->measureSamplingTimer.elapsed(MEASURE_PERIOD_MS))
            return;

        this->powerData.voltage = this->ade7953.getVrms();

        this->powerData.channel1.activePower = fabs(this->ade7953.getInstActivePowerA() * correction_factor);
        this->powerData.channel1.reactivePower = fabs(this->ade7953.getInstReactivePowerA() * correction_factor);

        this->powerData.channel2.activePower = fabs(this->ade7953.getInstActivePowerB() * correction_factor);
        this->powerData.channel2.reactivePower = fabs(this->ade7953.getInstReactivePowerB() * correction_factor) ;

        this->powerData.channel1.energy += fabs(this->ade7953.getActiveEnergyA() * energy_correction_factor) / 3600.0;
        this->powerData.channel1.apparentEnergy += fabs(this->ade7953.getApparentEnergyA() * energy_correction_factor) / 3600.0;

        this->powerData.channel2.energy += fabs(this->ade7953.getActiveEnergyB() * energy_correction_factor) / 3600.0;
        this->powerData.channel2.apparentEnergy += fabs(this->ade7953.getApparentEnergyB() * energy_correction_factor) / 3600.0;

        if (!this->measureReportTimer.elapsed(DEFAULT_ENERGY_REPORT_INTERVAL * 1000))
            return;

        ShellyManager::getInstance().reportPeriodicEnergyAndPower_ShellyEM(this->powerData);

        this->powerData.channel1.energy = 0;
        this->powerData.channel1.apparentEnergy = 0;
        this->powerData.channel2.energy = 0;
        this->powerData.channel2.apparentEnergy = 0;

}
#endif

#if defined(SHELLY_25) || defined(SHELLY_2PM_PLUS)
void PowerMeasurementControllerADE7953::_readPowerShelly25() {
    static bool first_measure_sent{false};

    if (!this->measureSamplingTimer.elapsed(MEASURE_PERIOD_MS)) {
        return;
    }

    bool dc_mode = this->ade7953.getVrms() < 200;

    if (dc_mode) {
        return;
    }



    // shelly 2.5 uses channel b for power 1 and channel a for power 2
    #ifdef SHELLY_25
    auto power1 = (this->ade7953.getInstActivePowerB() / 100.0) / 1.05;
    auto power2 = (this->ade7953.getInstActivePowerA() / 100.0) / 1.05;
    this->lastPower1 = power1/1.17;
    this->lastPower2 = power2/1.17;
    #endif

    #ifdef SHELLY_2PM_PLUS
    auto power1 = (this->ade7953.getInstActivePowerB() / 100.0);
    auto power2 = (this->ade7953.getInstActivePowerA() / 100.0);

    if(power1 < 100) {
        power1/=1.25;
    } else {
        power1/=1.2;
    }

    if(power2 < 100) {
        power2*=1.15;
    } else {
        power2/=1.15;
    }

    this->lastPower1 = power2;
    this->lastPower2 = power1;
    #endif

    if(!ShellyManager::getInstance().isOutput1()) {
        this->lastPower1 =0;
    }

    if(!ShellyManager::getInstance().isOutput2()) {
        this->lastPower2 =0;
    }


    this->smoothedPower1 = this->SMOOTHING_ALPHA*this->smoothedPower1 + (1-this->SMOOTHING_ALPHA)*this->lastPower1;
    this->smoothedPower2 = this->SMOOTHING_ALPHA*this->smoothedPower2 + (1-this->SMOOTHING_ALPHA)*this->lastPower2;


    if (!this->energySamplingTimer.elapsed(10 * MEASURE_PERIOD_MS))
        return;

    auto energy_b = this->ade7953.getActiveEnergyB();
    auto energy_a = this->ade7953.getActiveEnergyA();

    #ifdef SHELLY_25
    this->energy1Sum += ((double) fabs(energy_b)) / 3600.0;
    this->energy2Sum += ((double) fabs(energy_a)) / 3600.0;
    #endif

    #ifdef SHELLY_2PM_PLUS
    this->energy1Sum += ((double) fabs(energy_a)) / 3600.0;
    this->energy2Sum += ((double) fabs(energy_b)) / 3600.0;
    #endif

    if (!this->measureReportTimer.elapsed(DEFAULT_ENERGY_REPORT_INTERVAL * 1000)
        && first_measure_sent)
        return;

    first_measure_sent = true;

    ShellyManager::getInstance().reportPeriodicEnergyAndPower25(this->lastPower1, this->getEnergy1(), this->lastPower2, this->getEnergy2());

}
#endif

PowerMeasurementControllerADE7953::PowerMeasurementControllerADE7953() {
    this->ade7953.initialize();
}


const ADE7953 &PowerMeasurementControllerADE7953::getAde7953() const {
    return ade7953;
}

double PowerMeasurementControllerADE7953::getSmoothedPower1() const {
    return smoothedPower1;
}

double PowerMeasurementControllerADE7953::getSmoothedPower2() const {
    return smoothedPower2;
}
