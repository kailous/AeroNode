#include "WifiConnector.h"
#include "WebConfig.h"
#include "UserConfig.h"

extern void onIdleTick();

WifiConnector::WifiConnector(const char* ssid, const char* password)
    : _ssid(ssid), _password(password) {
}

void WifiConnector::connect(SerialPlotter& plotter, OledDisplay& display) {
    plotter.logWifiConnecting();
    
    // 定义状态机状态
    enum ConnectState {
        STATE_BOOT,         // 初始化阶段
        STATE_CONNECTING,   // 正在连接 WiFi
        STATE_CONNECTED,    // 连接成功
        STATE_AP_ONLY       // 连接超时，仅保留 AP
    };

    ConnectState currentState = STATE_BOOT;
    unsigned long stateStartTime = 0;
    unsigned long lastDotTime = 0;
    
    // 使用全局 WebConfig 实例 (UserConfig.cpp 中定义)，确保退出函数后服务不中断
    // WebConfig webConfig;

    // 状态机主循环
    while (true) {
        // 全局任务：处理 Web 请求和系统后台任务
        webConfig.handleClient();
        onIdleTick();
        yield();

        switch (currentState) {
            case STATE_BOOT:
                // 0. 基础设置
                WiFi.setSleepMode(WIFI_NONE_SLEEP);
                WiFi.persistent(false); // 避免频繁写 Flash

                // 1. 启动 AP+STA
                WiFi.mode(WIFI_AP_STA);
                {
                    IPAddress apIP(192, 168, 4, 1);
                    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
                    // 关键修正：固定信道 6，防止 AP 随 STA 乱跳
                    if (WiFi.softAP("IMU_POD_AP", "12345678", 6)) {
                        Serial.print("\nAP Started. SSID: IMU_POD_AP, IP: ");
                        Serial.println(WiFi.softAPIP());
                    }
                }
                display.showAPConfig("IMU_POD_AP", WiFi.softAPIP());

                // 2. 启动 WebConfig & 开始连接
                webConfig.begin();
                
                // 优先尝试连接 Flash 中保存的 WiFi 配置 (由 WebUI 设置)
                if (WiFi.SSID().length() > 0) {
                    Serial.printf("Connecting to saved WiFi: %s\n", WiFi.SSID().c_str());
                    WiFi.begin(); // 无参数调用，使用保存的配置
                } else {
                    Serial.printf("Connecting to default WiFi: %s\n", _ssid);
                    WiFi.begin(_ssid, _password);
                }

                // 状态迁移 -> 连接中
                currentState = STATE_CONNECTING;
                stateStartTime = millis();
                break;

            case STATE_CONNECTING:
                if (WiFi.status() == WL_CONNECTED) {
                    currentState = STATE_CONNECTED;
                } else if (millis() - stateStartTime > 15000) { // 缩短为 15s
                    // 超时 -> 切换到 AP 模式
                    Serial.println("\nWiFi Timeout. Switch to AP only.");
                    currentState = STATE_AP_ONLY;
                } else {
                    // 打印进度点
                    if (millis() - lastDotTime > 500) {
                        lastDotTime = millis();
                        plotter.printDot();
                    }
                }
                break;

            case STATE_CONNECTED:
                plotter.logWifiConnected(WiFi.localIP());
                // 连接成功，关闭 AP，进入正常工作模式
                // WiFi.softAPdisconnect(true); // 保持 AP 开启，方便随时配置
                // WiFi.mode(WIFI_STA);         // 保持 AP+STA 模式
                return; // 退出函数，开始主循环

            case STATE_AP_ONLY:
                // 1. 显式切到仅 AP 模式 (SDK 级重置)
                WiFi.mode(WIFI_AP);
                // 2. 必须重新启动 softAP (带固定信道 6)，确保广播恢复
                WiFi.softAP("IMU_POD_AP", "12345678", 6);
                display.showAPMode("IMU_POD_AP", "12345678");
                
                // 3. 进入死循环驻留，不再返回外层状态机
                while (true) {
                    webConfig.handleClient();
                    onIdleTick();
                    yield();
                }
                break;
        }
    }
}

void WifiConnector::ensureMacAddress(uint8_t* macAddress) {
    // 如果 macAddress 为 0，则读取 ESP8266 的 MAC
    if (!(macAddress[0] | macAddress[1] | macAddress[2] | macAddress[3] | macAddress[4] | macAddress[5]))
        WiFi.macAddress(macAddress);
}
