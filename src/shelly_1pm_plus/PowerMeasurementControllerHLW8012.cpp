#ifdef SHELLY_1PM_PLUS
#include "PowerMeasurementControllerHLW8012.h"
#include "Arduino.h"
#include "ShellyManager.h"

// GPIOs
#define SEL_PIN                         23
#define CF1_PIN                         18
#define CF_PIN                          5


// Set SEL_PIN to HIGH to sample current
// This is the case for Itead's Sonoff POW, where a
// the SEL_PIN drives a transistor that pulls down
// the SEL pin in the HLW8012 when closed
#define CURRENT_MODE                    HIGH

// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

#define CORRECTION_COEFFICIENT          (64000000*(2.43*2.43)*2351/0.001/24/3579000)*(0.1785)/(1000000.0)


PowerMeasurementControllerHLW8012 *callbackInstancePtr{nullptr};

void PowerMeasurementControllerHLW8012::loop() {
    this->_readPower();
}

double PowerMeasurementControllerHLW8012::getPower() {
    return this->lastPower;
}

double PowerMeasurementControllerHLW8012::getEnergy() {
    auto out = this->energySum;
    this->energySum = 0;
    return out;
}

void PowerMeasurementControllerHLW8012::_readPower() {
    const auto measure_period = DEFAULT_ENERGY_REPORT_INTERVAL;

    double measure_ms = ((double) measure_period) * 1000.0;

    if (!this->measureTimer.elapsed((unsigned long) measure_ms))
        return;

    if (this->cfPulseCount <= 1) {
        this->lastPower = 0;
        this->energySum = 0;
    } else {
        double f_hz = (this->cfPulseCount / 2.0) / measure_period;
        this->lastPower = CORRECTION_COEFFICIENT * f_hz;
        if(this->lastPower < 100) {
            this->lastPower/=1.07;
        } else {
            this->lastPower/=1.12;
        }
        this->energySum += this->lastPower * (double) measure_period / 3600.0;
    }

    ShellyManager::getInstance().reportAsyncEnergyAndPower(this->lastPower, this->getEnergy());

    this->cfPulseCount = 0;


}

void IRAM_ATTR hlw8012_cf_interrupt() {
    callbackInstancePtr->cfPulseCount++;
}

PowerMeasurementControllerHLW8012::PowerMeasurementControllerHLW8012() {

    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true, 1000000);

    // These values are used to calculate current, voltage and power factors as per datasheet formula
    // These are the nominal values for the Sonoff POW resistors:
    // * The CURRENT_RESISTOR is the 1milliOhm copper-manganese
    // resistor in series with the main line
    // * The VOLTAGE_RESISTOR_UPSTREAM are the 5 470kOhm resistors in the voltage divider that feeds the V2P pin in the HLW8012
    // * The VOLTAGE_RESISTOR_DOWNSTREAM is the 1kOhm resistor in the voltage divider that feeds the V2P pin in the HLW8012

    hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);

    callbackInstancePtr = this;

    attachInterrupt(digitalPinToInterrupt(CF_PIN), hlw8012_cf_interrupt, CHANGE);

}
#endif