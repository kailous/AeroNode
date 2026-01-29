#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "MPU6050.h" // 作者：jrowberg
#include "I2Cdev.h"  // 库地址：https://github.com/jrowberg/i2cdevlib
#include "CRC32.h"   // 作者：bakercp，https://github.com/bakercp/CRC32
#include "OledDisplay.h" // OLED 显示模块
#include "SerialPlotter.h" // 串口示波器模块
#include "Calibration.h" // 校准参数
#include "UserConfig.h" // 用户自定义数据

int16_t  accXI, accYI, accZI, gyrPI, gyrYI, gyrRI; // 原始整型姿态数据
float    accXF, accYF, accZF, gyrPF, gyrYF, gyrRF; // 浮点姿态数据

/***************************************************************************************
 *
****************************************************************************************/

WiFiUDP udp;
uint8_t udpIn[28];
uint8_t udpInfoOut[32];
uint8_t udpDataOut[100];

// 要上报的 MAC 地址，若设为 0 则使用 ESP8266 的 MAC
uint8_t macAddress[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

bool debugLog = false; // 启用调试日志
bool clientConnected = false; // 标记客户端是否已连接，用于控制 OLED 刷新

uint32_t dataPacketNumber = 0; // 当前数据包计数

// 所有时间变量单位为微秒
uint32_t receiveTime;     // 上一次接收数据包的时间
uint32_t sampleTime;      // 上一次从 MPU6050 读取的时间
uint32_t fifoSendTime;    // 上一次发送 UDP 响应的时间
uint32_t dataRequestTime; // 上一次收到数据请求的时间
const uint32_t receiveDelay       = 30e3; // 接收数据包的最小时间间隔
const uint32_t sampleDelay        = 10e3; // MPU6050 采样间隔
const uint32_t fifoSendDelay      = 1e3;  // UDP 发送间隔
const uint32_t dataRequestTimeout = 0;    // 数据请求超时时间，设为 0 表示禁用 (防止 USB 供电时意外休眠)

// 网络数据包大小
const uint32_t infoResponseSize = 32;
const uint32_t dataResponseSize = 100;

// 数据包返回地址
uint16_t  dataReplyPort;
IPAddress dataReplyIp;

MPU6050 mpu(mpuAddr);
// 陀螺仪灵敏度 0: +/-250 deg/s, 1: +/-500 deg/s, 2: +/-1000 deg/s, 3: +/-2000 deg/s
// 设置过低会产生削波
// 设置过高会降低灵敏度
const uint8_t gyroSens = 2;
const float   gyroLSB  = 131.0f / pow(2, gyroSens);

// 信息包响应
uint8_t makeInfoPacket(uint8_t* output, uint8_t portNumber)
{
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
  }
  else // 其他控制器设为未连接
  {
      output[20] = portNumber;    // 上报的设备槽位（i）
      output[21] = 0x00; // 槽位状态：未连接（0）
      output[22] = 0x00; // 设备型号：不适用（0）
      output[23] = 0x00; // 连接类型：不适用（0）
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
uint8_t makeDataPacket(uint8_t* output,       uint32_t packetCount,  uint32_t timestamp,
                       float accellerometerX, float accellerometerY, float accellerometerZ,
                       float gyroscopePit,    float gyroscopeYaw,    float gyroscopeRol)
{
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
  memcpy(&output[32], &packetCount, sizeof(packetCount)); // 将 packetCount 写入数据包
  // 不关心按键、摇杆和触摸板数据，相关字节清零
  output[36] = 0x00; // 十字键左、下、右、上，Options (?), R3, L3, Share (?)
  output[37] = 0x00; // Y, B, A, X, R1, L1, R2, L2
  output[38] = 0x00; // PS 键（未使用）
  output[39] = 0x00; // 触摸键（未使用）
  output[40] = 0x00; // 左摇杆 X（向右为正）
  output[41] = 0x00; // 左摇杆 Y（向上为正）
  output[42] = 0x00; // 右摇杆 X（向右为正）
  output[43] = 0x00; // 右摇杆 Y（向上为正）
  output[44] = 0x00; // 模拟十字键左
  output[45] = 0x00; // 模拟十字键下
  output[46] = 0x00; // 模拟十字键右
  output[47] = 0x00; // 模拟十字键上
  output[48] = 0x00; // 模拟 Y
  output[49] = 0x00; // 模拟 B
  output[50] = 0x00; // 模拟 A
  output[51] = 0x00; // 模拟 X
  output[52] = 0x00; // 模拟 R1
  output[53] = 0x00; // 模拟 L1
  output[54] = 0x00; // 模拟 R2
  output[55] = 0x00; // 模拟 L2
  output[56] = 0x00; // 是否有第一个触摸点
  output[57] = 0x00; // 第一个触摸点 ID
  output[58] = 0x00; // 第一个触摸点 X（16 位）
  output[59] = 0x00; //...
  output[60] = 0x00; // 第一个触摸点 Y（16 位）
  output[61] = 0x00; //...
  output[62] = 0x00; // 是否有第二个触摸点
  output[63] = 0x00; // 第二个触摸点 ID
  output[64] = 0x00; // 第二个触摸点 X（16 位）
  output[65] = 0x00; //...
  output[66] = 0x00; // 第二个触摸点 Y（16 位）
  output[67] = 0x00; //...
  // 清空时间戳高 4 字节，并拷贝低 4 字节
  output[72] = 0x00;
  output[73] = 0x00;
  output[74] = 0x00;
  output[75] = 0x00;
  memcpy(&output [68], &timestamp, sizeof(timestamp)); // 将时间戳写入数据包
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

template <size_t FIFO_SIZE, size_t MAX_PACKET_SIZE>
class Fifo
{
public:
  void pushPacket(uint16_t port, IPAddress ip, uint32_t dataSize, uint8_t* data)
  {
    // 缓冲区已满则跳过
    if(count == FIFO_SIZE)
      return;

    Packet* writePtr = buffer + rear;

    // 拷贝数据到队列
    writePtr->port = port;
    writePtr->ip = ip;
    writePtr->dataSize = dataSize;
    memcpy(writePtr->data, data, dataSize);

    // 更新尾指针
    rear = (rear + 1) % FIFO_SIZE;
    count++;
  }

  void sendPacket()
  {
    // 缓冲区为空则跳过
    if(count == 0)
      return;

    Packet* readPtr = buffer + front;

    udp.beginPacket(readPtr->ip, readPtr->port);
    udp.write(readPtr->data, readPtr->dataSize);
    udp.endPacket();

    // 更新头指针
    front = (front + 1) % FIFO_SIZE;
    count--;
  }

private:
  uint32_t count = 0, rear = 0, front = 0;

  struct Packet
  {
    uint16_t port;
    IPAddress ip;
    uint32_t dataSize;
    uint8_t data[MAX_PACKET_SIZE];
  };
  Packet buffer[FIFO_SIZE];
};

const uint32_t fifoMaxSize = 64;
Fifo<fifoMaxSize, dataResponseSize> fifo;

void setup()
{
  plotter.begin(74880);

  // 初始化 OLED
  display.begin();
  display.showBooting();
  delay(500); // 稍微停留一下，让用户看到启动画面
  display.showConnecting();

  plotter.logWifiConnecting();
  WiFi.begin(wifiSSID, wifiPass);
  int wifiRetry = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (wifiRetry++ > 40) { // 20秒超时 (40 * 500ms)
      display.showWifiTimeoutError();
      while (true) delay(100); // 停机
    }
    delay(500);
    plotter.printDot();
  }
  plotter.logWifiConnected(WiFi.localIP());

  // WiFi 连接成功，显示网络信息
  display.showServerReady(WiFi.SSID(), WiFi.localIP(), udpPort, WiFi.macAddress());

  // 如果 macAddress 为 0，则读取 ESP8266 的 MAC
  if (!(macAddress[0] | macAddress[1] | macAddress[2] | macAddress[3] | macAddress[4] | macAddress[5]))
    WiFi.macAddress(macAddress);

  plotter.logMacAddress(WiFi.macAddress());

  udp.begin(udpPort);
  plotter.logUdpServerStarted(udpPort);

  plotter.logMpuInitializing();
  Wire.begin(mpuSda, mpuScl); // 将 MPU6050 连接到 mpuSda 和 mpuScl 定义的 GPIO 引脚
  mpu.initialize();
  
  bool mpuConnection = mpu.testConnection();
  plotter.logMpuConnection(mpuConnection);
  if (!mpuConnection) {
    display.showMpuConnectionError();
    // 如果硬件连接失败，停在这里，不再继续执行
    while (true) { delay(100); }
  }

  mpu.setFullScaleGyroRange(gyroSens); // 设置陀螺仪灵敏度
  mpu.setXAccelOffset(offsetTable[0]);
  mpu.setYAccelOffset(offsetTable[1]);
  mpu.setZAccelOffset(offsetTable[2]);
  mpu.setXGyroOffset(offsetTable[3]);
  mpu.setYGyroOffset(offsetTable[4]);
  mpu.setZGyroOffset(offsetTable[5]);

  dataRequestTime = micros(); // 设置 dataRequestTime，若超时未收到数据请求将关机

  plotter.logInitComplete();
}

void loop()
{
  // 1. 向 FIFO 填充数据包
  if (micros() - sampleTime > sampleDelay) // 检查数据包间隔是否满足
  {
    dataPacketNumber++; sampleTime = micros();

    mpu.getMotion6(swapTable[0], swapTable[1], swapTable[2], swapTable[3], swapTable[4], swapTable[5]);

    // 对 MPU-6050 而言，最大值为 32768，最小值为 -32767
    // 但因使用 2 补码 16 位有符号数，32728 会被解释为 -32728
    // 为防止从正值回绕为负值，需要从原始数据减 1
    accXI--; accYI--; accZI--;
    gyrPI--; gyrYI--; gyrRI--;

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

    gyrPF = gyrPI / gyroLSB;  // LSB/deg/s
    gyrYF = gyrYI / gyroLSB;
    gyrRF = gyrRI / gyroLSB;

    gyrYF -= gyrOffYF;

    makeDataPacket(udpDataOut, dataPacketNumber, sampleTime, accXF, accYF, accZF, gyrPF, gyrYF, gyrRF);
    fifo.pushPacket(dataReplyPort, dataReplyIp, dataResponseSize, udpDataOut);

    plotter.plot(accXF, accYF, accZF, gyrPF, gyrYF, gyrRF, dataPacketNumber);
  }

  // 2. 检查请求
  if(micros() - receiveTime > receiveDelay)
  {
    receiveTime = micros();

    uint32_t packetInSize = udp.parsePacket();
    if (packetInSize)
    {
      udp.read(udpIn, sizeof(udpIn));
      switch(udpIn[16]) // udpIn[16] - 事件类型最低字节
      {
        case 0x01: // 控制器信息
          if (debugLog)
            plotter.logInfoRequest();

          for (uint8_t i = 0; i < udpIn[20]; i++) // udpIn[20] - 需要上报的端口数量
          {
            makeInfoPacket(udpInfoOut, udpIn[24 + i]); // udpIn[24 + i] - 需要上报的槽位号
            fifo.pushPacket(udp.remotePort(), udp.remoteIP(), dataResponseSize, udpInfoOut);
          }
          break;

        case 0x02: // 控制器输入数据
          if (debugLog)
            plotter.logDataRequest();

          // 状态切换：仅在首次收到数据请求时更新 OLED
          if (!clientConnected)
          {
            clientConnected = true;
            display.showClientConnected(udp.remoteIP());
          }

          dataReplyPort = udp.remotePort(); dataReplyIp = udp.remoteIP();

          dataRequestTime = micros(); // 刷新超时计时器
          break;
      }
    }
  }

  // 3. 发送 FIFO 内容
  if (micros() - fifoSendTime > fifoSendDelay) // 检查 UDP 包发送间隔
  {
    fifoSendTime = micros();
    fifo.sendPacket();
  }

  // 4. 超时关机
  if (dataRequestTimeout && (micros() - dataRequestTime > dataRequestTimeout)) // 检查是否因缺少控制器请求而超时
  {
    // 若长时间未收到数据包，则不再需要姿态信息，为节能关闭
    plotter.logShuttingDown();
    display.showDeepSleep();
    ESP.deepSleep(0);
  }
}
