#pragma once

#include "OledDisplay.h"
#include "SerialPlotter.h"
#include "../Wifi/WebConfig.h"
#include <Preferences.h>
#include <stdint.h>

// Config Load/Save functions
void loadConfig();
void saveConfig();

// WiFi/UDP config
extern char wifiSSID[32];
extern char wifiPass[64];
extern uint16_t udpPort;

// OLED / I2C config
extern const int OLED_SDA;
extern const int OLED_SCL;
extern const int OLED_RST;

// MPU6050 I2C config
extern const int MPU_INT;
extern const int MPU_SDA;
extern const int MPU_SCL;
extern const int MPU_VCC;
extern const int MPU_GND;

// Buttons
extern const int BUTTON_LEFT;

// BleMouse Config
extern char bleName[32];
extern char customMac[18];
extern float sensitivity;
extern float deadzone;
extern float gyroZ_offset;
extern float gyroX_offset;

// Calibration Config
extern int16_t offsetTable[6];
extern float gyrOffYF;

// System Config
extern float batteryVoltageFull;
extern float batteryVoltageEmpty;
extern float batteryDividerRatio;
extern bool debugLog;

// OLED / Serial plotter
extern OledDisplay display;
extern SerialPlotter plotter;
extern WebConfig webConfig;
