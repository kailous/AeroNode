#include "WebConfig.h"
#include <ESP8266WiFi.h>
#include <LittleFS.h>

WebConfig::WebConfig() : server(80), _fsMounted(false) {
}

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
        String statusVal = (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
        String ipVal = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "-";
        String rssiVal = (WiFi.status() == WL_CONNECTED) ? String(WiFi.RSSI()) + " dBm" : "-";

        // 4. 替换占位符
        html.replace("{STATUS}", statusVal);
        html.replace("{IP}", ipVal);
        html.replace("{RSSI}", rssiVal);
        html.replace("{SSID}", WiFi.SSID());

        server.send(200, "text/html", html);
    });

    // 保存页：接收数据并重连
    server.on("/save", HTTP_POST, [this]() {
        String ssid = server.arg("s");
        String pass = server.arg("p");
        if (ssid.length() > 0) {
            // 强制开启持久化，确保 SSID/密码 写入 Flash
            WiFi.persistent(true);
            WiFi.begin(ssid.c_str(), pass.c_str()); 
            
            String html = F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'></head><body>");
            html += F("<h2>Saved. Restarting...</h2>");
            html += F("<p>Device is restarting to apply settings.</p>");
            html += F("<p>Please wait about 15 seconds, then:</p>");
            html += F("<p><a href='/' style='font-size:18px; font-weight:bold;'>Click here to return to Status Page</a></p>");
            html += F("<script>setTimeout(function(){window.location.href='/';}, 15000);</script>");
            html += F("</body></html>");

            server.send(200, "text/html", html);
            delay(1000);
            ESP.restart(); // ⭐️ 关键：保存后直接重启，避免运行时切换 WiFi 导致的不稳定
        } else {
            server.send(200, "text/html", "Error: SSID required. <a href='/'>Back</a>");
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
                if (path.endsWith(".css")) contentType = "text/css";
                else if (path.endsWith(".js")) contentType = "application/javascript";
                else if (path.endsWith(".png")) contentType = "image/png";
                else if (path.endsWith(".jpg")) contentType = "image/jpeg";
                else if (path.endsWith(".ico")) contentType = "image/x-icon";
                
                File file = LittleFS.open(path, "r");
                server.streamFile(file, contentType);
                file.close();
                return;
            }
        }

        // 2. 如果文件不存在，则执行 Captive Portal 重定向
        server.sendHeader("Location", "/", true);
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
