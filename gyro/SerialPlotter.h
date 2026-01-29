#ifndef SERIAL_PLOTTER_H
#define SERIAL_PLOTTER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

class SerialPlotter {
public:
    // 构造函数：默认是否启用
    SerialPlotter(bool enabled = false);

    // 初始化串口
    void begin(unsigned long baud);

    // 输出波形数据到串口
    void plot(float accX, float accY, float accZ, float gyrP, float gyrY, float gyrR, uint32_t step);

    // --- WiFi 日志方法 ---
    void logWifiConnecting();
    void printDot();
    void logWifiConnected(const IPAddress& ip);

    // --- 系统日志方法 ---
    void logMacAddress(const String& mac);
    void logUdpServerStarted(uint16_t port);
    void logMpuInitializing();
    void logMpuConnection(bool success);
    void logInitComplete();
    void logInfoRequest();
    void logDataRequest();
    void logShuttingDown();

    // 启用或禁用
    void setEnabled(bool enabled);

private:
    bool _enabled;
};

#endif