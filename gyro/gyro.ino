#include <CRC32.h>                 // 作者：bakercp，https://github.com/bakercp/CRC32
#include "src/Common/Calibration.h"    // 校准参数
#include <I2Cdev.h>                // 库地址：https://github.com/jrowberg/i2cdevlib
#include <MPU6050.h>               // 作者：jrowberg
#include "src/Common/OledDisplay.h"    // OLED 显示模块
#include "src/Common/SerialPlotter.h"  // 串口示波器模块
#include "src/Common/UserConfig.h"     // 用户自定义数据
#include "src/Wifi/WifiConnector.h"    // 引入 WiFi 连接器
#include "src/Wifi/WebConfig.h"        // WebConfig 连接器
#include "esp_mac.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "src/Mouse/BleMouseController.h"

BleMouseController* mouseController = nullptr;
Adafruit_MPU6050 adafruitMpu;

int16_t accXI, accYI, accZI, gyrPI, gyrYI, gyrRI; // 原始整型姿态数据
float accXF, accYF, accZF, gyrPF, gyrYF, gyrRF;   // 浮点姿态数据

// 提供给 WiFi 连接流程中的空闲回调
void onIdleTick() {}

// ================= FreeRTOS 任务句柄和互斥锁 =================
TaskHandle_t NetworkTaskHandle;
TaskHandle_t SensorTaskHandle;
SemaphoreHandle_t dataMutex;             // 用于保护 Fifo 和 共享时间戳等
SemaphoreHandle_t mpuInterruptSemaphore; // 用于 MPU 硬件中断同步

// 中断处理函数
void IRAM_ATTR mpuInterruptHandler() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(mpuInterruptSemaphore, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
}
// =========================================================

/***************************************************************************************
 *
 ****************************************************************************************/

WiFiUDP udp;
uint8_t udpIn[28];
uint8_t udpInfoOut[32];
uint8_t udpDataOut[100];

// 要上报的 MAC 地址，若设为 0 则使用 ESP8266 的 MAC
uint8_t macAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// bool debugLog = false;        // 启用调试日志 (在 UserConfig.cpp 中已定义)
bool clientConnected = false; // 标记客户端是否已连接，用于控制 OLED 刷新

// 启动模式标志位
bool enableBleMode = false;
bool enableWifiMode = false;

uint32_t dataPacketNumber = 0; // 当前数据包计数

// 所有时间变量单位为微秒
uint32_t receiveTime;               // 上一次接收数据包的时间
uint32_t sampleTime;                // 上一次从 MPU6050 读取的时间
uint32_t fifoSendTime;              // 上一次发送 UDP 响应的时间
uint32_t dataRequestTime;           // 上一次收到数据请求的时间
const uint32_t receiveDelay = 30e3; // 接收数据包的最小时间间隔
const uint32_t sampleDelay = 10e3;  // MPU6050 采样间隔
const uint32_t fifoSendDelay = 1e3; // UDP 发送间隔
const uint32_t dataRequestTimeout =
    0; // 数据请求超时时间，设为 0 表示禁用 (防止 USB 供电时意外休眠)

// 网络数据包大小
const uint32_t infoResponseSize = 32;
const uint32_t dataResponseSize = 100;

// 数据包返回地址
uint16_t dataReplyPort;
IPAddress dataReplyIp;

// I2C address for MPU6050 (default 0x68)
const uint8_t mpuAddr = 0x68;
MPU6050 mpu(mpuAddr);
// 陀螺仪灵敏度 0: +/-250 deg/s, 1: +/-500 deg/s, 2: +/-1000 deg/s, 3: +/-2000
// deg/s 设置过低会产生削波 设置过高会降低灵敏度
const uint8_t gyroSens = 2;
const float gyroLSB = 131.0f / pow(2, gyroSens);

// 信息包响应
uint8_t makeInfoPacket(uint8_t *output, uint8_t portNumber) {
  // 服务器魔术字符串
  output[0] = (uint8_t)'D';
  output[1] = (uint8_t)'S';
  output[2] = (uint8_t)'U';
  output[3] = (uint8_t)'S';
  // 协议版本（1001）
  output[4] = 0xE9;
  output[5] = 0x03;
  // 数据包长度（不含头部）+ 事件类型长度（16）
  output[6] = (uint8_t)(16);
  output[7] = 0;
  // CRC32 字段清零
  output[8] = 0;
  output[9] = 0;
  output[10] = 0;
  output[11] = 0;
  // 服务器 ID（0）
  output[12] = 0;
  output[13] = 0;
  output[14] = 0;
  output[15] = 0;
  // 事件类型：控制器信息（0x00100001）
  output[16] = 0x01;
  output[17] = 0x00;
  output[18] = 0x10;
  output[19] = 0x00;

  if (portNumber == 0) // 0 号控制器为唯一活动控制器
  {
    output[20] = 0x00; // 上报的设备槽位（0）
    output[21] = 0x02; // 槽位状态：已连接（2）
    output[22] = 0x02; // 设备型号：完整陀螺仪（DS4）（2）
    output[23] = 0x02; // 连接类型：蓝牙（2）。可能为 USB（1）或蓝牙（2）
    // 设备 MAC 地址（macAddress）
    output[24] = macAddress[0];
    output[25] = macAddress[1];
    output[26] = macAddress[2];
    output[27] = macAddress[3];
    output[28] = macAddress[4];
    output[29] = macAddress[5];
    // 电量状态：满电（5）
    output[30] = 0x05; // ...
    output[31] = 0x00; // 结束字节
  } else               // 其他控制器设为未连接
  {
    output[20] = portNumber; // 上报的设备槽位（i）
    output[21] = 0x00;       // 槽位状态：未连接（0）
    output[22] = 0x00;       // 设备型号：不适用（0）
    output[23] = 0x00;       // 连接类型：不适用（0）
    // 设备 MAC 地址：不适用（0x000000000000）
    output[24] = 0x00;
    output[25] = 0x00;
    output[26] = 0x00;
    output[27] = 0x00;
    output[28] = 0x00;
    output[29] = 0x00;
    // 电量状态：不适用（0）
    output[30] = 0x00; // ...
    output[31] = 0x00; // 结束字节
  }

  uint32_t Checksum = CRC32::calculate(output, 32);
  memcpy(&output[8], &Checksum, sizeof(Checksum)); // 将校验和写入数据包

  return 32; // 返回数据包长度
}
// 数据包响应
uint8_t makeDataPacket(uint8_t *output, uint32_t packetCount,
                       uint32_t timestamp, float accellerometerX,
                       float accellerometerY, float accellerometerZ,
                       float gyroscopePit, float gyroscopeYaw,
                       float gyroscopeRol) {
  // 服务器魔术字符串
  output[0] = (uint8_t)'D';
  output[1] = (uint8_t)'S';
  output[2] = (uint8_t)'U';
  output[3] = (uint8_t)'S';
  // 协议版本（1001）
  output[4] = 0xE9;
  output[5] = 0x03;
  // 数据包长度（不含头部）+ 事件类型长度（4）
  output[6] = (uint8_t)(80 + 4);
  output[7] = 0;
  // CRC32 字段清零
  output[8] = 0;
  output[9] = 0;
  output[10] = 0;
  output[11] = 0;
  // 服务器 ID（0）
  output[12] = 0;
  output[13] = 0;
  output[14] = 0;
  output[15] = 0;
  // 事件类型：控制器数据（0x00100002）
  output[16] = 0x02;
  output[17] = 0x00;
  output[18] = 0x10;
  output[19] = 0x00;

  output[20] = 0x00; // 上报的设备槽位（0）
  output[21] = 0x02; // 槽位状态：已连接（2）
  output[22] = 0x02; // 设备型号：完整陀螺仪（DS4）（2）
  output[23] = 0x02; // 连接类型：蓝牙（2）。可能为 USB（1）或蓝牙（2）
  // 设备 MAC 地址（macAddress）
  output[24] = macAddress[0];
  output[25] = macAddress[1];
  output[26] = macAddress[2];
  output[27] = macAddress[3];
  output[28] = macAddress[4];
  output[29] = macAddress[5];
  // 电量状态：满电（5）
  output[30] = 0x05; // ...

  output[31] = 0x01; // 设备状态：激活（1）
  memcpy(&output[32], &packetCount,
         sizeof(packetCount)); // 将 packetCount 写入数据包
  // 不关心按键、摇杆和触摸板数据，相关字节全部填充清零 (去除冗长的手柄模拟)
  memset(&output[36], 0, 32);
  // 清空时间戳高 4 字节，并拷贝低 4 字节
  output[72] = 0x00;
  output[73] = 0x00;
  output[74] = 0x00;
  output[75] = 0x00;
  memcpy(&output[68], &timestamp, sizeof(timestamp)); // 将时间戳写入数据包
  // 填入加速度计与陀螺仪数据
  memcpy(&output[76], &accellerometerX, sizeof(accellerometerX));
  memcpy(&output[80], &accellerometerY, sizeof(accellerometerY));
  memcpy(&output[84], &accellerometerZ, sizeof(accellerometerZ));
  memcpy(&output[88], &gyroscopePit, sizeof(gyroscopePit));
  memcpy(&output[92], &gyroscopeYaw, sizeof(gyroscopeYaw));
  memcpy(&output[96], &gyroscopeRol, sizeof(gyroscopeRol));

  uint32_t Checksum = CRC32::calculate(output, 100);
  memcpy(&output[8], &Checksum, sizeof(Checksum)); // 将校验和写入数据包

  return 100; // 返回数据包长度
}

template <size_t FIFO_SIZE, size_t MAX_PACKET_SIZE> class Fifo {
public:
  void pushPacket(uint16_t port, IPAddress ip, uint32_t dataSize,
                  uint8_t *data) {
    // 缓冲区已满则跳过
    if (count == FIFO_SIZE)
      return;

    Packet *writePtr = buffer + rear;

    // 拷贝数据到队列
    writePtr->port = port;
    writePtr->ip = ip;
    writePtr->dataSize = dataSize;
    memcpy(writePtr->data, data, dataSize);

    // 更新尾指针
    rear = (rear + 1) % FIFO_SIZE;
    count++;
  }

  void sendPacket() {
    // 缓冲区为空则跳过
    if (count == 0)
      return;

    Packet *readPtr = buffer + front;

    udp.beginPacket(readPtr->ip, readPtr->port);
    udp.write(readPtr->data, readPtr->dataSize);
    udp.endPacket();

    // 更新头指针
    front = (front + 1) % FIFO_SIZE;
    count--;
  }

private:
  uint32_t count = 0, rear = 0, front = 0;

  struct Packet {
    uint16_t port;
    IPAddress ip;
    uint32_t dataSize;
    uint8_t data[MAX_PACKET_SIZE];
  };
  Packet buffer[FIFO_SIZE];
};

const uint32_t fifoMaxSize = 64;
Fifo<fifoMaxSize, dataResponseSize> fifo;

void setup() {
  loadConfig();
  loadConfig();

  plotter.begin(74880);

  // 初始化 OLED
  display.begin();
  display.showBooting();

  // Load custom MAC if configured ("00:00:00:00:00:00" is default/empty)
  if (customMac[0] != '\0' && strcmp(customMac, "00:00:00:00:00:00") != 0) {
    uint8_t parsedMac[6];
    int values[6];
    if (6 == sscanf(customMac, "%x:%x:%x:%x:%x:%x", &values[0], &values[1],
                    &values[2], &values[3], &values[4], &values[5])) {
      for (int i = 0; i < 6; i++) {
        parsedMac[i] = (uint8_t)values[i];
        macAddress[i] = parsedMac[i]; // Also save for UDP polling reporting
      }
      // Set Station MAC and Base MAC early
      esp_base_mac_addr_set(parsedMac);
    }
  }

  // 初始化左键
  pinMode(BUTTON_LEFT, INPUT_PULLUP);

  // ================= 启动模式选择 (双启动) =================
  // 读取 BOOT 按钮 (GPIO 0, 低电平有效)
  pinMode(0, INPUT_PULLUP);
  delay(50); // 防抖
  bool bootButtonPressed = (digitalRead(0) == LOW);
  
  // 屏幕上已去掉电量显示
  delay(500); // 稍微停留一下，让用户看到启动画面

  if (bootButtonPressed) {
    // 模式 A: 蓝牙鼠标模式 (按住 BOOT 开机)
    enableBleMode = true;
    
    // 获取蓝牙 MAC 地址
    uint8_t btMac[6];
    esp_read_mac(btMac, ESP_MAC_BT);
    char btMacStr[18];
    snprintf(btMacStr, sizeof(btMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             btMac[0], btMac[1], btMac[2], btMac[3], btMac[4], btMac[5]);

    display.showBleReady(bleName, btMacStr);
    Serial.println("Booting in BLE Mouse Mode...");
    
    // 初始化 BleMouseController
    if (bleName[0] == '\0') {
      strcpy(bleName, "IMU_Air_Mouse");
    }
    mouseController = new BleMouseController();
    mouseController->begin(bleName, "Lipeng_Design", 100);
  } else {
    // 模式 B: Wi-Fi UDP 飞控模式 (默认开机模式)
    enableWifiMode = true;
    display.showConnecting();
    Serial.println("Booting in Wi-Fi UDP Mode...");

    // 使用 WifiConnector 接管连接逻辑 (包含 AP 模式和超时保护)
    WifiConnector wifiConnector(wifiSSID, wifiPass);
    wifiConnector.connect(plotter, display);

    // WiFi 连接成功，显示网络信息
    display.showServerReady(WiFi.SSID(), WiFi.localIP(), udpPort,
                            WiFi.macAddress());
  }

  // 如果 macAddress 为 0，则读取 ESP8266 的 MAC
  if (enableWifiMode) {
    if (!(macAddress[0] | macAddress[1] | macAddress[2] | macAddress[3] |
          macAddress[4] | macAddress[5])) {
      WiFi.macAddress(macAddress);
    }
    plotter.logMacAddress(WiFi.macAddress());

    udp.begin(udpPort);
    plotter.logUdpServerStarted(udpPort);
  }

  plotter.logMpuInitializing();

  // 给 MPU6050 供电 (ESP32)
  pinMode(MPU_VCC, OUTPUT);
  pinMode(MPU_GND, OUTPUT);
  digitalWrite(MPU_VCC, HIGH);
  digitalWrite(MPU_GND, LOW);
  delay(100); // 等待 MPU6050 上电稳定

  // 将 MPU6050 连接到 MPU_SDA 和 MPU_SCL 定义的 GPIO 引脚
  Wire.begin(MPU_SDA, MPU_SCL, 400000); 

  if (enableBleMode) {
    // -------------------------------------------------------------
    // === 蓝牙鼠标专供：Adafruit 纯净驱动初始化 ===
    // -------------------------------------------------------------
    if (!adafruitMpu.begin(0x68, &Wire)) {
      Serial.println("Adafruit MPU6050 init failed!");
      display.showMpuConnectionError();
      while(1) delay(10);
    }
    // 完全复刻 m.ino 的滤波器和量程设置
    adafruitMpu.setGyroRange(MPU6050_RANGE_250_DEG); 
    adafruitMpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    
  } else {
    // -------------------------------------------------------------
    // === Wi-Fi DSU 游戏专供：中断驱动带校准版初始化 ===
    // -------------------------------------------------------------
    // 初始化中断引脚
    mpuInterruptSemaphore = xSemaphoreCreateBinary();
    pinMode(MPU_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MPU_INT), mpuInterruptHandler, RISING);

    mpu.initialize();

    bool mpuConnection = mpu.testConnection();
    plotter.logMpuConnection(mpuConnection);
    if (!mpuConnection) {
      display.showMpuConnectionError();
      // 如果硬件连接失败，停在这里，不再继续执行
      while (true) {
        if (enableWifiMode) {
          webConfig.handleClient(); // 关键修复：即使硬件报错，也要保持 WebUI 可用
        }
        delay(10);
      }
    }

    mpu.setFullScaleGyroRange(gyroSens); // 设置陀螺仪灵敏度

    // ================= 中断优化配置 =================
    mpu.setIntDataReadyEnabled(true); // 开启数据就绪中断
    mpu.setRate(9);                   // 设置采样率为 100Hz (1kHz / (1 + 9))
    // ===============================================
    mpu.setXAccelOffset(offsetTable[0]);
    mpu.setYAccelOffset(offsetTable[1]);
    mpu.setZAccelOffset(offsetTable[2]);
    mpu.setXGyroOffset(offsetTable[3]);
    mpu.setYGyroOffset(offsetTable[4]);
    mpu.setZGyroOffset(offsetTable[5]);
  }

  dataRequestTime =
      micros(); // 设置 dataRequestTime，若超时未收到数据请求将关机

  plotter.logInitComplete();

  // 初始化互斥锁
  dataMutex = xSemaphoreCreateMutex();

  // 创建网络任务，固定在 Core 0 (PRO_CPU)
  if (enableWifiMode) {
    xTaskCreatePinnedToCore(networkTask,   // 任务函数
                            "NetworkTask", // 任务名称
                            8192, // 栈大小 (Word, 在 ESP32 上通常是 Bytes)
                            NULL, // 任务参数
                            1,    // 优先级
                            &NetworkTaskHandle, // 任务句柄
                            0                   // 核心 0
    );
  }

  // MPU 读取任务放在原本的 loop() 中（ESP32 的 loop 默认在 Core 1
  // (APP_CPU)），但为了更精确的定时，我们也可以将其作为一个专用任务
}

// ================= 任务分离: 网络与后台协议处理 =================
void networkTask(void *parameter) {
  for (;;) {
    uint32_t currentMicros = micros();

    if (enableWifiMode) {
      // 1. 检查 UDP 请求
      if (currentMicros - receiveTime > receiveDelay) {
        receiveTime = currentMicros;

        uint32_t packetInSize = udp.parsePacket();
        if (packetInSize) {
          udp.read(udpIn, sizeof(udpIn));
          switch (udpIn[16]) // udpIn[16] - 事件类型最低字节
          {
          case 0x01: // 控制器信息
            if (debugLog)
              plotter.logInfoRequest();

            for (uint8_t i = 0; i < udpIn[20];
                 i++) // udpIn[20] - 需要上报的端口数量
            {
              makeInfoPacket(udpInfoOut,
                             udpIn[24 + i]); // udpIn[24 + i] - 需要上报的槽位号
              xSemaphoreTake(dataMutex, portMAX_DELAY);
              fifo.pushPacket(udp.remotePort(), udp.remoteIP(), dataResponseSize,
                              udpInfoOut);
              xSemaphoreGive(dataMutex);
            }
            break;

          case 0x02: // 控制器输入数据
            if (debugLog)
              plotter.logDataRequest();

            // 状态切换：仅在首次收到数据请求时更新 OLED
            if (!clientConnected) {
              clientConnected = true;
              display.showClientConnected(udp.remoteIP());
            }

            dataReplyPort = udp.remotePort();
            dataReplyIp = udp.remoteIP();

            xSemaphoreTake(dataMutex, portMAX_DELAY);
            dataRequestTime = currentMicros; // 刷新超时计时器
            xSemaphoreGive(dataMutex);
            break;
          }
        }
      }

      // 2. 发送 FIFO 内容
      if (currentMicros - fifoSendTime > fifoSendDelay) // 检查 UDP 包发送间隔
      {
        fifoSendTime = currentMicros;
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        fifo.sendPacket();
        xSemaphoreGive(dataMutex);
      }

      // 3. 超时关机
      bool isTimeout = false;
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      isTimeout = dataRequestTimeout &&
                  (currentMicros - dataRequestTime > dataRequestTimeout);
      xSemaphoreGive(dataMutex);

      if (isTimeout) // 检查是否因缺少控制器请求而超时
      {
        plotter.logShuttingDown();
        display.showDeepSleep();
        esp_deep_sleep_start();
      }

      // 处理 Web 配置请求 (保持 AP 访问能力)
      webConfig.handleClient();
    }



    // 适当让出 CPU
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void loop() {
  if (enableBleMode) {
    // ------------------------------------------------------------------
    // 超高速轮询任务 (100Hz 刷新率，保证鼠标丝滑)
    // 注意：为了不阻塞 CPU 从而保证 100Hz 的稳定读取，
    // OLED 屏幕已经在 setup 中一次性绘制完毕，这里绝对不要再调用 display 
    // ------------------------------------------------------------------
    if (mouseController != nullptr) {
      bool currentButtonState = digitalRead(BUTTON_LEFT); // LOW=pressed

      sensors_event_t a, g, temp;
      adafruitMpu.getEvent(&a, &g, &temp);

      // X 和 Z 直接传给鼠标控制器，无需经过换算
      mouseController->update(g.gyro.x, g.gyro.z, currentButtonState, 
                              deadzone, sensitivity, 
                              gyroX_offset, gyroZ_offset);
      
      // 10ms 延迟，既控制了陀螺仪刷新率，也起到了按键防抖的作用
      delay(10);
    }
  }

  // ================= 任务分离: MPU 传感器读取 (默认在 Core 1)
  if (enableWifiMode) {
    // ================= 等待 MPU6050 Data Ready 中断信号 (Wi-Fi 模式)
    if (xSemaphoreTake(mpuInterruptSemaphore, portMAX_DELAY) == pdTRUE) {
      uint32_t currentMicros = micros();
      dataPacketNumber++;
      sampleTime = currentMicros;

      mpu.getMotion6(swapTable[0], swapTable[1], swapTable[2], swapTable[3],
                     swapTable[4], swapTable[5]);

      // 对 MPU-6050 而言，最大值为 32768，最小值为 -32767
      // 但因使用 2 补码 16 位有符号数，32728 会被解释为 -32728
      // 为防止从正值回绕为负值，需要从原始数据减 1
      accXI--;
      accYI--;
      accZI--;
      gyrPI--;
      gyrYI--;
      gyrRI--;

      // 反转选定轴方向
      accXI *= signTable[0] ? -1 : 1;
      accYI *= signTable[1] ? -1 : 1;
      accZI *= signTable[2] ? -1 : 1;
      gyrPI *= signTable[3] ? -1 : 1;
      gyrYI *= signTable[4] ? -1 : 1;
      gyrRI *= signTable[5] ? -1 : 1;

      // 将原始数据转换为浮点值
      accXF = accXI / 16384.0f; // LSB/mg，2g 灵敏度下为 16384
      accYF = accYI / 16384.0f;
      accZF = accZI / 16384.0f;

      gyrPF = gyrPI / gyroLSB; // LSB/deg/s
      gyrYF = gyrYI / gyroLSB;
      gyrRF = gyrRI / gyroLSB;

      gyrYF -= gyrOffYF;

      makeDataPacket(udpDataOut, dataPacketNumber, sampleTime, accXF, accYF,
                     accZF, gyrPF, gyrYF, gyrRF);

      // 使用 Mutex 锁定共享 Fifo 队列
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      fifo.pushPacket(dataReplyPort, dataReplyIp, dataResponseSize, udpDataOut);
      xSemaphoreGive(dataMutex);

      plotter.plot(accXF, accYF, accZF, gyrPF, gyrYF, gyrRF, dataPacketNumber);
    }
  }
}
