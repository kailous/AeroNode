#include "WifiConnector.h"
#include "../Common/UserConfig.h"
#include "WebConfig.h"
#include <WiFi.h>

extern void onIdleTick();

WifiConnector::WifiConnector(const char *ssid, const char *password)
    : _ssid(ssid), _password(password) {}

void WifiConnector::connect(SerialPlotter &plotter, OledDisplay &display) {
  plotter.logWifiConnecting();

  // 0. 基础设置
  // ESP32: WiFi.setSleep(true) must be used when WiFi and Bluetooth coexist
  WiFi.setSleep(true);
  WiFi.persistent(false);

  // 1. 尝试连接到保存或默认的 Wi-Fi
  WiFi.mode(WIFI_STA);
  if (WiFi.SSID().length() > 0) {
    Serial.printf("Connecting to saved WiFi: %s\n", WiFi.SSID().c_str());
    WiFi.begin();
  } else {
    Serial.printf("Connecting to default WiFi: %s\n", _ssid);
    WiFi.begin(_ssid, _password);
  }

  // 2. 检查连接状态 (超时 15 秒)
  unsigned long stateStartTime = millis();
  unsigned long lastDotTime = 0;
  bool connected = false;

  while (millis() - stateStartTime < 15000) {
    // 保持后台任务运行
    onIdleTick();
    yield();

    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }

    if (millis() - lastDotTime > 500) {
      lastDotTime = millis();
      plotter.printDot();
    }
  }

  // 3. 根据结果切换模式
  if (connected) {
    // ---- 连接成功 ----
    plotter.logWifiConnected(WiFi.localIP());
    // 启动 WebConfig 以防需要修改参数
    webConfig.begin(); 
    return; // 退出函数，开始主循环
  } else {
    // ---- 超时，进入纯 AP 配置模式 ----
    Serial.println("\nWiFi Timeout. Switch to AP mode for configuration.");
    
    WiFi.disconnect(true); // 断开任何残余的站模式连接
    delay(100);
    
    // 彻底切换为仅 AP 模式
    WiFi.mode(WIFI_AP);
    delay(100);

    // 标准 ESP32 AP 配置流程：先 Config，再 SoftAP
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    
    // 启动热点 (固定信道 6)
    WiFi.softAP("IMU_POD_AP", "12345678", 6);
    
    Serial.print("AP Started. SSID: IMU_POD_AP, IP: ");
    Serial.println(WiFi.softAPIP());

    display.showAPMode("IMU_POD_AP", "12345678");

    // 启动 WebConfig 和 Captive Portal
    webConfig.begin();

    // 进入死循环驻留，拦截并处理所有配网请求
    while (true) {
      webConfig.handleClient();
      onIdleTick();
      yield();
    }
  }
}

void WifiConnector::ensureMacAddress(uint8_t *macAddress) {
  // 如果 macAddress 为 0，则读取 MAC
  if (!(macAddress[0] | macAddress[1] | macAddress[2] | macAddress[3] |
        macAddress[4] | macAddress[5])) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    for (int i = 0; i < 6; i++) {
      macAddress[i] = mac[i];
    }
  }
}
