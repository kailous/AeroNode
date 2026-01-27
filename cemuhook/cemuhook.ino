#include "Config.h"
#include "Hardware.h"
#include "Display.h" 
#include "Network.h"

Config config;
MPU6050 mpu(0x68);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);
WiFiUDP udp;

int16_t ax, ay, az, gx, gy, gz;
float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;
float p = 0, r = 0, y = 0; 
int16_t base_gx=0, base_gy=0, base_gz=0;

bool isAPMode = false; 
IPAddress clientIP;
uint16_t clientPort = 0;
bool isConnected = false;
bool lastConnectionState = false;
uint32_t packetCount = 0;
uint32_t lastSampleTime = 0;
uint8_t macAddress[6];
uint8_t udpDataOut[100];
uint8_t udpIn[28];

void setup() {
  Serial.begin(74880);
  loadConfig();
  
  initDisplay(); 
  setupWiFi();
  WiFi.macAddress(macAddress);
  udp.begin(udpPort);
  
  server.on("/", handleRoot);
  server.on("/data", handleLiveData);
  server.on("/save", handleSave);
  server.on("/calibrate", handleCalibrateWeb);
  server.begin();
  
  initMPU();
  showStatus("Calibrating", "DO NOT MOVE!", true);
  calibrateMPU();
  showStatus("READY!", "System Go", false);
}

void loop() {
  uint32_t now = micros();
  server.handleClient();

  if (now - lastSampleTime > sampleInterval) {
    float dt = (now - lastSampleTime) / 1000000.0f;
    lastSampleTime = now;
    packetCount++;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // 陀螺仪原始数据处理
    float raw_gx = (float)(gx - base_gx - config.offset_gx) * 0.007633f;
    float raw_gy = (float)(gy - base_gy - config.offset_gy) * 0.007633f;
    float raw_gz = (float)(gz - base_gz - config.offset_gz) * 0.007633f;

    // 应用反转配置
    if (config.inv_gx) raw_gx = -raw_gx;
    if (config.inv_gy) raw_gy = -raw_gy;
    if (config.inv_gz) raw_gz = -raw_gz;

    ax_f = ax * 0.000061f;
    ay_f = ay * 0.000061f; 
    az_f = az * 0.000061f;
    gx_f = raw_gx; 
    gy_f = raw_gy; 
    gz_f = raw_gz;

    // 姿态解算
    p = atan2(ay_f, az_f) * 57.2958f;
    r = atan2(-ax_f, sqrt(ay_f * ay_f + az_f * az_f)) * 57.2958f;
    y += gz_f * dt; // YAW 由 Z 轴积分得到
    
    // ★ 修正：将 YAW 限制在 -180 到 180 之间，解决跳变问题
    if (y > 180.0f) y -= 360.0f;
    if (y < -180.0f) y += 360.0f;

    if (clientPort != 0) {
       makeDataPacket();
       udp.beginPacket(clientIP, clientPort);
       udp.write(udpDataOut, 100);
       udp.endPacket();
       isConnected = true;
    } else {
       isConnected = false;
    }
  }

  int packetSize = udp.parsePacket();
  if (packetSize >= 20) {
     udp.read(udpIn, 28);
     if (udpIn[16] == 0x02) { 
       clientPort = udp.remotePort();
       clientIP = udp.remoteIP();
     }
  }

  if (isConnected != lastConnectionState) {
    if(isConnected) showStatus("GAMING!", "Client Linked", true);
    else showStatus(isAPMode?"AP MODE":"WAITING...", isAPMode?"Phoenix_AP":"Check Client", false);
    lastConnectionState = isConnected;
  }
}