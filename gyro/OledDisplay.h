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

    // 显示错误信息
    void showError(const String& message);

    // 显示 WiFi 连接超时错误
    void showWifiTimeoutError();

    // 显示 MPU6050 连接失败错误
    void showMpuConnectionError();

    // 显示正在连接 WiFi
    void showConnecting();

    // 显示服务器就绪信息
    void showServerReady(const String& ssid, const IPAddress& ip, uint16_t port, const String& mac);

    // 显示客户端已连接
    void showClientConnected(const IPAddress& remoteIp);

    // 显示深度睡眠状态
    void showDeepSleep();

private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2;
};

#endif