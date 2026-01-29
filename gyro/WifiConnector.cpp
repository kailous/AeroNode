#include "WifiConnector.h"

WifiConnector::WifiConnector(const char* ssid, const char* password)
    : _ssid(ssid), _password(password) {
}

void WifiConnector::connect(SerialPlotter& plotter, OledDisplay& display) {
    plotter.logWifiConnecting();
    WiFi.begin(_ssid, _password);
    
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
}

void WifiConnector::ensureMacAddress(uint8_t* macAddress) {
    // 如果 macAddress 为 0，则读取 ESP8266 的 MAC
    if (!(macAddress[0] | macAddress[1] | macAddress[2] | macAddress[3] | macAddress[4] | macAddress[5]))
        WiFi.macAddress(macAddress);
}