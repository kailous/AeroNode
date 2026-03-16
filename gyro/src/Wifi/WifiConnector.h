#include <LittleFS.h>

#ifndef WIFICONNECTOR_H
#define WIFICONNECTOR_H

#include "../Common/OledDisplay.h"
#include "../Common/SerialPlotter.h"
#include <WiFi.h>

// WifiConnector 类用于连接到指定的 WiFi 网络
// 它提供了连接到 WiFi 网络的功能，并确保 ESP8266 的 MAC 地址被正确设置
class WifiConnector {
public:
  // 构造函数，初始化 WiFi 网络的 SSID 和密码
  WifiConnector(const char *ssid, const char *password);

  // 连接到 WiFi 网络
  // 它会尝试连接到指定的 SSID，最多重试 40 次 (20秒超时)
  // 如果连接成功，它会在串口和 OLED 上显示连接成功的信息
  // 如果连接失败，它会在 OLED 上显示超时错误信息，并停机
  void connect(SerialPlotter &plotter, OledDisplay &display);

  // 确保 ESP8266 的 MAC 地址被正确设置
  // 如果 macAddress 为 0，则读取 ESP8266 的 MAC 地址并设置到 macAddress 中
  void ensureMacAddress(uint8_t *macAddress);

private:
  const char *_ssid;
  const char *_password;
};

#endif