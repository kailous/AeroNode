#include "SerialPlotter.h"

SerialPlotter::SerialPlotter(bool enabled) : _enabled(enabled) {
}

void SerialPlotter::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialPlotter::plot(float accX, float accY, float accZ, float gyrP, float gyrY, float gyrR, uint32_t step) {
    if (!_enabled) return;

    Serial.print(" 加速度X: "); Serial.print(accX);
    Serial.print(" 加速度Y: "); Serial.print(accY);
    Serial.print(" 加速度Z: "); Serial.print(accZ);
    Serial.print(" 角速度P: "); Serial.print(gyrP);
    Serial.print(" 角速度Y: "); Serial.print(gyrY);
    Serial.print(" 角速度R: "); Serial.print(gyrR);
    Serial.print(" 计数: "); Serial.println(step);
}

void SerialPlotter::logWifiConnecting() {
    Serial.print("\n正在连接");
}

void SerialPlotter::printDot() {
    Serial.print(".");
}

void SerialPlotter::logWifiConnected(const IPAddress& ip) {
    Serial.print("\n已连接，IP 地址：");
    Serial.println(ip);
}

void SerialPlotter::logMacAddress(const String& mac) {
    Serial.print("MAC 地址：");
    Serial.println(mac);
}

void SerialPlotter::logUdpServerStarted(uint16_t port) {
    Serial.print("UDP 服务器已启动，端口：");
    Serial.println(port);
}

void SerialPlotter::logMpuInitializing() {
    Serial.println("正在初始化 MPU6050");
}

void SerialPlotter::logMpuConnection(bool success) {
    Serial.println(success ? "MPU6050 连接成功" : "MPU6050 连接失败");
}

void SerialPlotter::logInitComplete() {
    Serial.println("初始化完成！");
}

void SerialPlotter::logInfoRequest() {
    Serial.println("收到信息请求！");
}

void SerialPlotter::logDataRequest() {
    Serial.println("收到数据请求！");
}

void SerialPlotter::logShuttingDown() {
    Serial.println("正在关机..."); Serial.flush();
}

void SerialPlotter::setEnabled(bool enabled) {
    _enabled = enabled;
}