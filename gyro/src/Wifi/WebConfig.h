#ifndef WEBCONFIG_H
#define WEBCONFIG_H

#include <DNSServer.h>
#include <WebServer.h>

class WebConfig {
public:
  WebConfig();

  // 启动 Web 服务器 (非阻塞)
  void begin();
  // 处理客户端请求
  void handleClient();

private:
  WebServer server;
  DNSServer dnsServer;
  bool _fsMounted;
};

#endif
