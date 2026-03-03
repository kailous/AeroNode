#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <U8g2lib.h>
#include <ESP8266WiFi.h>

class OledDisplay {
public:
    // 构造函数：指定 SDA 和 SCL 引脚
    OledDisplay(uint8_t sdaPin, uint8_t sclPin);

    // 初始化显示屏
    void begin();

    // 显示启动画面
    void showBooting();

    // 显示正在连接 WiFi
    void showConnecting();

    // 显示错误信息
    void showError(const String& message, const char* errorCode = nullptr);

    // 显示 WiFi 连接超时错误
    void showWifiTimeoutError();

    // 显示 MPU6050 连接失败错误
    void showMpuConnectionError();

    // 显示 AP 模式 (WIFI 未连接)
    void showAPMode(const char* ssid, const char* password);

    // 显示 AP 配网模式
    void showAPConfig(const String& ssid, const IPAddress& ip);

    // 显示服务器就绪信息
    void showServerReady(const String& ssid, const IPAddress& ip, uint16_t port, const String& mac);

    // 显示客户端已连接
    void showClientConnected(const IPAddress& remoteIp);

    // 显示深度睡眠状态
    void showDeepSleep();

    // 更新电量百分比（0-100）
    void setBatteryPercent(uint8_t percent);

private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;
    bool _available = false;
    uint8_t _batteryPercent = 255; // 255 表示未知

    // UI Constants
    static const uint8_t RIGHT_X = 123;
    static const uint8_t LINE_X_START = 5;
    static const uint8_t LINE_X_END = 122;
    static const uint8_t HEADER_HEIGHT = 15;
    static const uint8_t BATTERY_AREA_WIDTH = 30;
    static const uint8_t BATTERY_AREA_X = 128 - BATTERY_AREA_WIDTH;

    // Helpers
    void drawHeader();
    void drawBatteryPercent();
    void drawRightAlignedStr(uint8_t y, const char* str);
    void drawSeparator(uint8_t y);
};

#endif
