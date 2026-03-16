#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor(uint8_t adcPin, float adcRefVoltage,
                               float dividerRatio, uint8_t sampleCount,
                               float emptyVoltage, float fullVoltage)
    : _adcPin(adcPin), _adcRefVoltage(adcRefVoltage),
      _dividerRatio(dividerRatio), _sampleCount(sampleCount),
      _emptyVoltage(emptyVoltage), _fullVoltage(fullVoltage) {}

void BatteryMonitor::setParams(float dividerRatio, float emptyVoltage,
                               float fullVoltage) {
  _dividerRatio = dividerRatio;
  _emptyVoltage = emptyVoltage;
  _fullVoltage = fullVoltage;
}

uint16_t BatteryMonitor::readRawAverage() const {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < _sampleCount; i++) {
    sum += analogRead(_adcPin);
  }
  return (sum + (_sampleCount / 2)) / _sampleCount;
}

uint8_t BatteryMonitor::voltageToPercent(float vinVoltage) const {
  if (_fullVoltage <= _emptyVoltage)
    return 0;

  float pct =
      (vinVoltage - _emptyVoltage) * 100.0f / (_fullVoltage - _emptyVoltage);
  if (pct < 0.0f)
    pct = 0.0f;
  if (pct > 100.0f)
    pct = 100.0f;

  return static_cast<uint8_t>(pct + 0.5f);
}

BatteryReading BatteryMonitor::read() {
  BatteryReading reading;
  reading.raw = readRawAverage();
  reading.a0Voltage = (reading.raw / 4095.0f) * _adcRefVoltage;
  reading.vinVoltage = reading.a0Voltage * _dividerRatio;
  reading.percent = voltageToPercent(reading.vinVoltage);
  return reading;
}
