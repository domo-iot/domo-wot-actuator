# domo-wot-actuator

[![Actions Status][actions badge]][actions]
[![CodeCov][codecov badge]][codecov]
[![LICENSE][license badge]][license]

DoMO WoT implementation for the WiFi actuators provided by DoMO [Web of Things](https://www.w3.org/TR/wot-thing-description/)
[DHT](https://github.com/domo-iot/domo-dht).

The WoT firmware for the DoMO WiFi actuators has been developed using the C++ language and the Arduino ESP8266 Framework. We also used PlatformIO (https://platformio.org/) to simplify the firmware development and building processes. All the different WiFi actuators that we use in the pilot share the same code base. This allows to reduce code repetition and speeds up testing operations.


## Supported HW

- [x] Shelly 1
- [x] Shelly 1 PM
- [x] Shelly 2.5
- [x] Shelly Dimmer
- [x] Shelly RGBW
- [x] Shelly EM
- [x] Shelly 1 Plus
- [x] Shelly 1PM Plus
- [x] Shelly 2PM Plus




# Acknowledgements

This software has been partially developed in the scope of the H2020 project SIFIS-Home with GA n. 952652.

<!-- Links -->
[actions]: https://github.com/domo-iot/domo-wot-actuator/actions
[codecov]: https://codecov.io/gh/domo-iot/domo-wot-actuator
[license]: LICENSE

<!-- Badges -->
[actions badge]: https://github.com/domo-iot/domo-wot-actuator/workflows/domo-wot-actuator/badge.svg
[codecov badge]: https://codecov.io/gh/domo-iot/domo-wot-actuator/branch/master/graph/badge.svg
[license badge]: https://img.shields.io/badge/license-MIT-blue.svg
