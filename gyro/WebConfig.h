#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <ESP8266WebServer.h>
#include <DNSServer.h>

class WebConfig {
public:
    WebConfig();
    
    // 启动 Web 服务器 (非阻塞)
    void begin();
    // 处理客户端请求
    void handleClient();

private:
    ESP8266WebServer server;
    DNSServer dnsServer;
    bool _fsMounted;
};

#endif
