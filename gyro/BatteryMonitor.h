#pragma once

#include <Arduino.h>

struct BatteryReading {
    uint16_t raw = 0;
    float a0Voltage = 0.0f;
    float vinVoltage = 0.0f;
    uint8_t percent = 0;
};

class BatteryMonitor {
public:
    BatteryMonitor(uint8_t adcPin,
                   float adcRefVoltage,
                   float dividerRatio,
                   uint8_t sampleCount,
                   float emptyVoltage,
                   float fullVoltage);

    BatteryReading read();

private:
    uint16_t readRawAverage() const;
    uint8_t voltageToPercent(float vinVoltage) const;

    uint8_t _adcPin;
    float _adcRefVoltage;
    float _dividerRatio;
    uint8_t _sampleCount;
    float _emptyVoltage;
    float _fullVoltage;
};
