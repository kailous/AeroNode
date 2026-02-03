#pragma once

#include <stdint.h>
#include "OledDisplay.h"
#include "SerialPlotter.h"
#include "WebConfig.h"

// WiFi/UDP config
extern char wifiSSID[];
extern char wifiPass[];
extern uint16_t udpPort;

// MPU6050 I2C config
extern const uint8_t mpuSda;
extern const uint8_t mpuScl;
extern const uint8_t mpuAddr;

// OLED / Serial plotter
extern OledDisplay display;
extern SerialPlotter plotter;
extern WebConfig webConfig;
