#include "OledDisplay.h"
#include "U8g2CustomFont.h" // 引入自定义字体文件

extern uint16_t udpPort; // 引用 UserConfig.cpp 中的端口变量

// Bitmaps defined in OLED.txt
static const unsigned char image_Rpc_active_bits[] PROGMEM = {0x07,0x35,0x47,0x40,0x01,0x71,0x56,0x70};
static const unsigned char image_BLE_beacon_bits[] PROGMEM = {0x22,0x49,0x55,0x49,0x2a,0x08,0x08,0x3e};
static const unsigned char image_GameMode_bits[] PROGMEM = {0x20,0x00,0xfe,0x03,0xfb,0x07,0x71,0x05,0xfb,0x07,0x8f,0x07,0x07,0x07,0x03,0x06};
static const unsigned char image_Alert_bits[] PROGMEM = {0x10,0x00,0x38,0x00,0x28,0x00,0x6c,0x00,0x6c,0x00,0xfe,0x00,0xee,0x00,0xff,0x01};

OledDisplay::OledDisplay(uint8_t sdaPin, uint8_t sclPin)
    : u8g2(U8G2_R0, /* clock=*/ sclPin, /* data=*/ sdaPin, /* reset=*/ U8X8_PIN_NONE) {
}

void OledDisplay::begin() {
    _available = u8g2.begin(); // 如果初始化失败（未连接），返回 false
    if (!_available) return;

    u8g2.enableUTF8Print(); // 启用 UTF8 支持
    // u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_6x10_tr); // 统一使用 6x10 字体
}

void OledDisplay::drawHeader() {
    if (!_available) return;
    u8g2.setFontMode(1);
    u8g2.setBitmapMode(1);
    u8g2.drawBox(0, 0, 128, HEADER_HEIGHT);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(5, 11, "IMU Pod");
}

void OledDisplay::drawBatteryPercent() {
    if (!_available) return;

    char buf[6];
    if (_batteryPercent > 100) {
        snprintf(buf, sizeof(buf), "--%%");
    } else {
        snprintf(buf, sizeof(buf), "%u%%", _batteryPercent);
    }

    // 清除右上角电量区域并绘制百分比
    u8g2.setDrawColor(1);
    u8g2.drawBox(BATTERY_AREA_X, 0, BATTERY_AREA_WIDTH, HEADER_HEIGHT);
    u8g2.setDrawColor(2);
    u8g2.setFont(u8g2_font_6x10_tr);
    drawRightAlignedStr(11, buf);
}

void OledDisplay::drawRightAlignedStr(uint8_t y, const char* str) {
    int w = u8g2.getStrWidth(str);
    u8g2.drawStr(RIGHT_X - w, y, str);
}

void OledDisplay::drawSeparator(uint8_t y) {
    u8g2.drawLine(LINE_X_END, y, LINE_X_START, y);
}

void OledDisplay::showBooting() {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "Booting...");
    u8g2.drawStr(5, 40, "Init Hardware...");
    drawBatteryPercent();
    u8g2.sendBuffer();
}

void OledDisplay::showConnecting() {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "Connecting...");
    u8g2.drawStr(5, 40, "Please wait...");
    drawBatteryPercent();
    u8g2.sendBuffer();
}

void OledDisplay::showError(const String& message, const char* errorCode) {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "ERROR");

    u8g2.setDrawColor(2);
    u8g2.drawXBMP(114, 4, 9, 8, image_Alert_bits);

    u8g2.setDrawColor(1);
    // 显示传入的错误信息
    u8g2.drawStr(5, 46, message.c_str());

    if (errorCode) {
    u8g2.drawStr(5, 59, errorCode);
    }

    drawSeparator(33);
    drawBatteryPercent();

    u8g2.sendBuffer();
}

void OledDisplay::showWifiTimeoutError() {
    showError("WiFi Timeout");
}

void OledDisplay::showMpuConnectionError() {
    showError("MPU Error", "E-01");
}

void OledDisplay::showAPMode(const char* ssid, const char* password) {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();
    
    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 46, "AP");

    drawRightAlignedStr(46, ssid);
    drawRightAlignedStr(59, password);

    u8g2.drawStr(5, 59, "PWD");
    u8g2.drawStr(5, 28, "Wi-Fi");
    drawRightAlignedStr(28, "OFF");

    drawSeparator(32);
    
    u8g2.setDrawColor(2);
    u8g2.drawXBMP(116, 4, 7, 8, image_Rpc_active_bits);
    drawBatteryPercent();

    u8g2.sendBuffer();
}

void OledDisplay::showAPConfig(const String& ssid, const IPAddress& ip) {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "AP Config");

    u8g2.drawStr(5, 46, "SSID");
    drawRightAlignedStr(46, ssid.c_str());

    u8g2.drawStr(5, 59, "IP");
    char buf[24];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    drawRightAlignedStr(59, buf);

    drawSeparator(33);
    drawBatteryPercent();

    u8g2.sendBuffer();
}

void OledDisplay::showServerReady(const String& ssid, const IPAddress& ip, uint16_t port, const String& mac) {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();
    
    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 46, "IP");

    char buf[24];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    drawRightAlignedStr(46, buf);

    snprintf(buf, sizeof(buf), "%u", port);
    drawRightAlignedStr(59, buf);

    u8g2.drawStr(5, 59, "PORT");
    u8g2.drawStr(5, 28, "Wi-Fi");
    drawRightAlignedStr(28, "READY");

    drawSeparator(32);
    
    u8g2.setDrawColor(2);
    u8g2.drawXBMP(116, 4, 7, 8, image_Rpc_active_bits);
    u8g2.drawXBMP(103, 4, 7, 8, image_BLE_beacon_bits);
    drawBatteryPercent();
    
    u8g2.sendBuffer();
}

void OledDisplay::showClientConnected(const IPAddress& remoteIp) {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(2);
    u8g2.drawXBMP(116, 4, 7, 8, image_Rpc_active_bits);
    u8g2.drawXBMP(103, 4, 7, 8, image_BLE_beacon_bits);
    u8g2.drawXBMP(86, 4, 11, 8, image_GameMode_bits);

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "SENDING...");

    char buf[24];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", remoteIp[0], remoteIp[1], remoteIp[2], remoteIp[3]);
    u8g2.drawStr(5, 46, buf);

    snprintf(buf, sizeof(buf), "%u", udpPort);
    u8g2.drawStr(5, 59, buf);

    drawSeparator(33);
    drawBatteryPercent();

    u8g2.sendBuffer();
}

void OledDisplay::showDeepSleep() {
    if (!_available) return;
    u8g2.clearBuffer();
    drawHeader();

    u8g2.setDrawColor(1);
    u8g2.drawStr(5, 28, "Deep Sleep...");
    drawBatteryPercent();
    u8g2.sendBuffer();
}

void OledDisplay::setBatteryPercent(uint8_t percent) {
    if (!_available) return;

    if (percent > 100) percent = 100;
    if (_batteryPercent == percent) return;

    _batteryPercent = percent;
    drawBatteryPercent();
    u8g2.sendBuffer();
}
