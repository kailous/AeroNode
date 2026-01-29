#include "OledDisplay.h"
#include "U8g2CustomFont.h" // 引入自定义字体文件

// 字体实际高度，用于计算行间距和排版
const uint8_t FONT_HEIGHT = 13;

OledDisplay::OledDisplay(uint8_t sdaPin, uint8_t sclPin)
    : u8g2(U8G2_R0, /* clock=*/ sclPin, /* data=*/ sdaPin, /* reset=*/ U8X8_PIN_NONE) {
}

void OledDisplay::begin() {
    u8g2.begin();
    u8g2.enableUTF8Print(); // 启用 UTF8 支持
    u8g2.setFontPosTop();   // 设置字体定位点为左上角 (解决反色显示被切断的问题)
    u8g2.setFont(u8g2_font_wqy10_my); // 使用自定义剪辑字体 (实际为 13px 高度)
}

void OledDisplay::showBooting() {
    u8g2.clearBuffer();
    u8g2.drawUTF8(0, 0, "系统启动中...");
    u8g2.drawUTF8(0, FONT_HEIGHT, "初始化硬件...");
    u8g2.sendBuffer();
}

void OledDisplay::showError(const String& message) {
    u8g2.clearBuffer();
    u8g2.drawUTF8(0, 0, "错误:");
    u8g2.drawUTF8(0, FONT_HEIGHT, message.c_str());
    u8g2.sendBuffer();
}

void OledDisplay::showWifiTimeoutError() {
    showError("WiFi 连接超时");
}

void OledDisplay::showMpuConnectionError() {
    showError("MPU6050 未连接");
}

void OledDisplay::showConnecting() {
    u8g2.clearBuffer();
    u8g2.drawUTF8(0, 0, "正在连接 WiFi...");
    u8g2.sendBuffer();
}

void OledDisplay::showServerReady(const String& ssid, const IPAddress& ip, uint16_t port, const String& mac) {
    u8g2.clearBuffer();
    // 字体在 64px 屏幕上显示 4-5 行 (0, 13, 26, 39, 52)
    
    // 反色显示标题栏
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, 128, FONT_HEIGHT);
    u8g2.setDrawColor(0); // 设置为黑色文字

    // 居中显示标题
    const char* title = "Kailous DSU 就绪";
    int titleWidth = u8g2.getUTF8Width(title);
    u8g2.drawUTF8((128 - titleWidth) / 2, 0, title);
    u8g2.setDrawColor(1); // 恢复白色文字

    u8g2.drawUTF8(0, FONT_HEIGHT, ssid.c_str());
    u8g2.drawUTF8(0, FONT_HEIGHT * 2, ip.toString().c_str());

    char infoBuf[32];
    sprintf(infoBuf, "端口: %u", port);
    u8g2.drawUTF8(0, FONT_HEIGHT * 3, infoBuf);
    
    // 空间不足，不再显示 MAC 地址
    u8g2.sendBuffer();
}

void OledDisplay::showClientConnected(const IPAddress& remoteIp) {
    u8g2.clearBuffer();
    u8g2.drawUTF8(0, 0, "已连接");
    u8g2.drawUTF8(0, FONT_HEIGHT, remoteIp.toString().c_str());
    u8g2.drawUTF8(0, FONT_HEIGHT * 2, "数据传输中...");
    u8g2.sendBuffer();
}

void OledDisplay::showDeepSleep() {
    u8g2.clearBuffer();
    u8g2.drawUTF8(0, FONT_HEIGHT, "进入休眠...");
    u8g2.sendBuffer();
}