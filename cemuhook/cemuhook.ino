#include "Config.h"
#include "Hardware.h"
#include "Network.h"

// ================= 变量定义 =================
Config config;
MPU6050 mpu(0x68);
Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer server(80);
WiFiUDP udp;

int16_t ax, ay, az, gx, gy, gz;
float ax_f, ay_f, az_f, gx_f, gy_f, gz_f;
int16_t base_gx=0, base_gy=0, base_gz=0;

// ★ 新增变量
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

// ================= SETUP =================
void setup() {
  Serial.begin(74880);
  
  loadConfig();

  // 1. 屏幕初始化 (D1/D2)
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);

  // 2. ★★★ 调用智能配网逻辑 ★★★
  setupWiFi(); 
  
  // 此时已经连上 WiFi 或者开启 AP 了
  // 获取一下 MAC (AP模式下也可以获取)
  WiFi.macAddress(macAddress);
  udp.begin(udpPort);

  // 3. Web Server
  server.on("/", handleRoot);
  server.on("/data", handleLiveData);
  server.on("/save", handleSave);
  server.on("/calibrate", handleCalibrateWeb);
  server.begin();

  // 4. MPU 初始化 (D5/D6)
  initMPU();

  // 5. 自动校准
  showStatus("Calibrating", "DO NOT MOVE!", true);
  delay(1000);
  calibrateMPU();

  // 6. 完成
  showStatus("READY!", "System Go", false);
  delay(1000);
}

// ================= LOOP =================
void loop() {
  // ... Loop 内容和原来一模一样，保持不变 ...
  uint32_t now = micros();
  server.handleClient(); 
  if (now - lastSampleTime > sampleInterval) {
    lastSampleTime = now;
    packetCount++;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    // ... 计算逻辑 ... (请保持你之前的方案A/方案B代码)
    // 这里为了节省篇幅省略了，请确保你保留了 Main.ino 中之前的核心映射代码
    
    // ---------------------------------------------
    // 这里是你之前确定的核心计算逻辑 (不要丢了!)
    // ---------------------------------------------
    float raw_gx = (float)(gx - base_gx - config.offset_gx) * 0.007633f; 
    float raw_gy = (float)(gy - base_gy - config.offset_gy) * 0.007633f;
    float raw_gz = (float)(gz - base_gz - config.offset_gz) * 0.007633f;
    if (config.inv_gx) raw_gx = -raw_gx;
    if (config.inv_gy) raw_gy = -raw_gy;
    if (config.inv_gz) raw_gz = -raw_gz;

    // 方案 A (平放):
    ax_f = ax * 0.000061f; ay_f = ay * 0.000061f; az_f = az * 0.000061f;
    gx_f = raw_gx; gy_f = raw_gy; gz_f = raw_gz;
    // ---------------------------------------------

    // UDP 发送 (仅在非 AP 模式且有客户端时发送)
    // 或者即使在 AP 模式下，只要有 Client 发 handshake 也可以发
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
  if (packetSize) {
    if (packetSize >= 20) {
       udp.read(udpIn, 28);
       if (udpIn[16] == 0x02) { 
         clientPort = udp.remotePort();
         clientIP = udp.remoteIP();
       }
    } else { udp.flush(); }
  }

  if (isConnected != lastConnectionState) {
    if(isConnected) showStatus("GAMING!", "Client Linked", true);
    else showStatus(isAPMode?"AP MODE":"WAITING...", isAPMode?"Phoenix_AP":"Check Client", false);
    lastConnectionState = isConnected;
  }
}