#ifndef DOMO_ESP_POWERMEASUREMENTCONTROLLER_H
#define DOMO_ESP_POWERMEASUREMENTCONTROLLER_H

#include "HLW8012.h"
#include "DomoTimer.h"
#include "defines.h"

class PowerMeasurementControllerHLW8012 {

public:

    PowerMeasurementControllerHLW8012();

    void loop();

    double getPower();

    double getEnergy();

    HLW8012 hlw8012 {};

    uint32_t cfPulseCount;
protected:

    void _readPower();

    DomoTimer measureTimer {};

    double energySum {0};

    double lastPower {0};

    double lastUpdateTimestamp {-1};

};


#endif
