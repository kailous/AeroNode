#include "WebConfig.h"
#include "../Common/UserConfig.h"
#include <LittleFS.h>
#include <WiFi.h>

WebConfig::WebConfig() : server(80), _fsMounted(false) {}

void WebConfig::begin() {
  // 初始化文件系统
  if (LittleFS.begin()) {
    _fsMounted = true;
  } else {
    Serial.println("LittleFS mount failed");
    _fsMounted = false;
  }

  // 首页：从 LittleFS 读取 index.html
  server.on("/", HTTP_GET, [this]() {
    String html = "";

    // 1. 尝试从文件系统读取模板
    if (_fsMounted && LittleFS.exists("/index.html")) {
      File file = LittleFS.open("/index.html", "r");
      if (file) {
        html = file.readString(); // 读取文件内容到字符串，以便进行替换
        file.close();
      }
    }

    // 2. 如果文件读取失败，返回错误提示
    if (html.length() == 0) {
      server.send(500, "text/plain", "Error: index.html not found in LittleFS");
      return;
    }

    // 3. 准备动态数据
    String statusVal =
        (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
    String ipVal =
        (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "-";
    String rssiVal =
        (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) + " dBm" : "-";

    // 4. 替换占位符
    html.replace("{STATUS}", statusVal);
    html.replace("{IP}", ipVal);
    html.replace("{RSSI}", rssiVal);

    html.replace("{SSID}", wifiSSID);
    html.replace("{PASSWORD}", wifiPass);
    html.replace("{UDP_PORT}", String(udpPort));

    html.replace("{BLE_NAME}", bleName);
    html.replace("{CUSTOM_MAC}", customMac);

    html.replace("{SENSITIVITY}", String(sensitivity, 1));
    html.replace("{DEADZONE}", String(deadzone, 2));
    html.replace("{GYROX_OFFSET}", String(gyroX_offset, 1));
    html.replace("{GYROZ_OFFSET}", String(gyroZ_offset, 1));

    html.replace("{AX_OFF}", String(offsetTable[0]));
    html.replace("{AY_OFF}", String(offsetTable[1]));
    html.replace("{AZ_OFF}", String(offsetTable[2]));
    html.replace("{GX_C_OFF}", String(offsetTable[3]));
    html.replace("{GY_C_OFF}", String(offsetTable[4]));
    html.replace("{GZ_C_OFF}", String(offsetTable[5]));

    html.replace("{GY_DRIFT}", String(gyrOffYF, 2));

    html.replace("{BAT_FULL}", String(batteryVoltageFull, 1));
    html.replace("{BAT_EMPTY}", String(batteryVoltageEmpty, 1));
    html.replace("{BAT_DIV}", String(batteryDividerRatio, 1));

    html.replace("{DEBUG_CHECKED}", debugLog ? "checked" : "");

    server.send(200, "text/html", html);
  });

  // 保存页：接收数据并重连
  server.on("/save", HTTP_POST, [this]() {
    if (server.hasArg("s"))
      strncpy(wifiSSID, server.arg("s").c_str(), sizeof(wifiSSID) - 1);
    if (server.hasArg("p"))
      strncpy(wifiPass, server.arg("p").c_str(), sizeof(wifiPass) - 1);
    if (server.hasArg("udp"))
      udpPort = server.arg("udp").toInt();

    if (server.hasArg("ble_name"))
      strncpy(bleName, server.arg("ble_name").c_str(), sizeof(bleName) - 1);

    // Process MAC strictly (allow empty to reset to default or valid format)
    if (server.hasArg("mac")) {
      String newMac = server.arg("mac");
      if (newMac.length() == 17 || newMac.length() == 0 ||
          newMac == "00:00:00:00:00:00") {
        strncpy(customMac, newMac.c_str(), sizeof(customMac) - 1);
      }
    }

    if (server.hasArg("sens"))
      sensitivity = server.arg("sens").toFloat();
    if (server.hasArg("dead"))
      deadzone = server.arg("dead").toFloat();
    if (server.hasArg("gx_off"))
      gyroX_offset = server.arg("gx_off").toFloat();
    if (server.hasArg("gz_off"))
      gyroZ_offset = server.arg("gz_off").toFloat();

    if (server.hasArg("ax_off"))
      offsetTable[0] = server.arg("ax_off").toInt();
    if (server.hasArg("ay_off"))
      offsetTable[1] = server.arg("ay_off").toInt();
    if (server.hasArg("az_off"))
      offsetTable[2] = server.arg("az_off").toInt();
    if (server.hasArg("gx_c_off"))
      offsetTable[3] = server.arg("gx_c_off").toInt();
    if (server.hasArg("gy_c_off"))
      offsetTable[4] = server.arg("gy_c_off").toInt();
    if (server.hasArg("gz_c_off"))
      offsetTable[5] = server.arg("gz_c_off").toInt();

    if (server.hasArg("gy_drift"))
      gyrOffYF = server.arg("gy_drift").toFloat();

    if (server.hasArg("bat_f"))
      batteryVoltageFull = server.arg("bat_f").toFloat();
    if (server.hasArg("bat_e"))
      batteryVoltageEmpty = server.arg("bat_e").toFloat();
    if (server.hasArg("bat_div"))
      batteryDividerRatio = server.arg("bat_div").toFloat();

    debugLog = server.hasArg("debug");

    saveConfig();

    if (String(wifiSSID).length() > 0) {
      // 强制开启持久化，确保 SSID/密码 写入 Flash
      WiFi.persistent(true);
      WiFi.begin(wifiSSID, wifiPass);

      String html = "";
      if (_fsMounted && LittleFS.exists("/saved.html")) {
        File file = LittleFS.open("/saved.html", "r");
        if (file) {
          html = file.readString();
          file.close();
        }
      }
      if (html.length() == 0)
        html = "Saved. Restarting...";

      server.send(200, "text/html", html);
      delay(1000);
      ESP.restart(); // ⭐️ 关键：保存后直接重启，避免运行时切换 WiFi
                     // 导致的不稳定
    } else {
      server.send(200, "text/html",
                  "Error: SSID required. <a href='/'>Back</a>");
    }
  });

  // 捕获所有未定义的请求 (Captive Portal 关键)
  // 无论手机请求 /generate_204 还是其他地址，都重定向到首页
  server.onNotFound([this]() {
    // 1. 尝试从文件系统加载静态资源 (img, js, css 等)
    if (_fsMounted) {
      String path = server.uri();
      if (LittleFS.exists(path)) {
        String contentType = "text/plain";
        if (path.endsWith(".css"))
          contentType = "text/css";
        else if (path.endsWith(".js"))
          contentType = "application/javascript";
        else if (path.endsWith(".png"))
          contentType = "image/png";
        else if (path.endsWith(".jpg"))
          contentType = "image/jpeg";
        else if (path.endsWith(".ico"))
          contentType = "image/x-icon";

        File file = LittleFS.open(path, "r");
        server.streamFile(file, contentType);
        file.close();
        return;
      }
    }

    // 2. 如果文件不存在，则执行 Captive Portal 重定向
    // 对于真正的 Captive Portal，最好重定向到绝对 IP 而不是相对路径 "/"
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();

  // 启动 DNS 服务器
  // 53 是 DNS 端口
  // "*" 代表拦截所有域名请求
  // WiFi.softAPIP() 是目标 IP (通常是 192.168.4.1)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
}

void WebConfig::handleClient() {
  dnsServer.processNextRequest();
  server.handleClient();
}
