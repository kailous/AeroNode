#ifndef CONFIG_H
#define CONFIG_H

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "MPU6050.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "CRC32.h"

// ================= 常量定义 =================
const uint16_t udpPort = 26760; // ★★★ 补回了这一行！(DSU协议默认端口)

// ================= 引脚定义 =================
const int OLED_SDA = 4;  // D2
const int OLED_SCL = 5;  // D1
const int MPU_SDA = 14;  // D5
const int MPU_SCL = 12;  // D6

const uint32_t sampleInterval = 5000; // 5ms = 200Hz

// ================= 数据结构 =================
struct Config {
  // WiFi 账号密码 (定长数组)
  char wifi_ssid[32];
  char wifi_pass[64];

  bool inv_gx, inv_gy, inv_gz;
  int16_t offset_gx, offset_gy, offset_gz;
  int8_t dlpf_mode; 
  int check_code;
};

// ================= 全局变量声明 =================
extern Config config;
extern MPU6050 mpu;
extern Adafruit_SSD1306 display;
extern ESP8266WebServer server;
extern WiFiUDP udp;

extern int16_t ax, ay, az, gx, gy, gz;
extern float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;
extern int16_t base_gx, base_gy, base_gz;

extern IPAddress clientIP;
extern uint16_t clientPort;
extern bool isConnected;
extern uint32_t packetCount;
extern uint8_t macAddress[6];
extern uint8_t udpDataOut[100];
extern bool isAPMode; 

#endif