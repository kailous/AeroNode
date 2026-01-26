#ifndef NETWORK_H
#define NETWORK_H

#include "Config.h"
#include "Hardware.h"
#include "WebConfig.h"

// ★ 新增：智能配网逻辑
void setupWiFi() {
  WiFi.mode(WIFI_STA); // 先尝试 STA 模式
  
  // 检查是否有保存的 SSID
  if (strlen(config.wifi_ssid) > 0) {
    showStatus("Connecting...", String(config.wifi_ssid), true);
    WiFi.begin(config.wifi_ssid, config.wifi_pass);
    
    // 尝试连接 15 秒
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) { 
      delay(500); 
      timeout++;
      if(timeout % 2 == 0) showStatus("Connecting...", String(timeout/2) + "s", true);
    }
  }

  // 判断连接结果
  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    showStatus("WiFi Linked!", WiFi.localIP().toString(), false);
  } else {
    // ★ 连接失败，或者没有保存的 WiFi -> 开启 AP 模式
    isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Phoenix_AP", "12345678"); // 热点名和密码
    showStatus("AP Mode", "Connect: Phoenix_AP", false);
  }
}

// ... handleLiveData (略，保持不变) ...
void handleLiveData() {
  String json = "{";
  json += "\"ax\":" + String(ax_f, 3) + ","; json += "\"ay\":" + String(ay_f, 3) + ","; json += "\"az\":" + String(az_f, 3) + ",";
  json += "\"gx\":" + String(gx_f, 3) + ","; json += "\"gy\":" + String(gy_f, 3) + ","; json += "\"gz\":" + String(gz_f, 3);
  json += "}";
  server.send(200, "application/json", json);
}

// ... handleRoot ...
void handleRoot() {
  String s = index_html; 
  s.replace("%CHK_GX%", config.inv_gx?"checked":"");
  s.replace("%CHK_GY%", config.inv_gy?"checked":"");
  s.replace("%CHK_GZ%", config.inv_gz?"checked":"");
  s.replace("%OFF_GX%", String(config.offset_gx));
  s.replace("%OFF_GY%", String(config.offset_gy));
  s.replace("%OFF_GZ%", String(config.offset_gz));
  s.replace("%SEL_0%", config.dlpf_mode==0?"selected":"");
  s.replace("%SEL_1%", config.dlpf_mode==1?"selected":"");
  s.replace("%SEL_2%", config.dlpf_mode==2?"selected":"");
  s.replace("%SEL_3%", config.dlpf_mode==3?"selected":"");
  s.replace("%SEL_4%", config.dlpf_mode==4?"selected":"");
  s.replace("%SEL_6%", config.dlpf_mode==6?"selected":"");
  
  // ★ 把当前 SSID 显示在 placeholder 里 (不显示密码)
  s.replace("%SSID%", String(config.wifi_ssid));
  
  server.send(200, "text/html; charset=utf-8", s);
}

void handleSave() {
  // 保存普通参数
  config.inv_gx = server.hasArg("inv_gx");
  config.inv_gy = server.hasArg("inv_gy");
  config.inv_gz = server.hasArg("inv_gz");
  config.offset_gx = server.arg("off_gx").toInt();
  config.offset_gy = server.arg("off_gy").toInt();
  config.offset_gz = server.arg("off_gz").toInt();
  if (server.hasArg("dlpf")) config.dlpf_mode = server.arg("dlpf").toInt();
  
  // ★ 处理 WiFi 保存
  String new_ssid = server.arg("ssid");
  String new_pass = server.arg("pass");
  
  // 只有当用户真的输入了内容才更新 WiFi
  if (new_ssid.length() > 0) {
    new_ssid.toCharArray(config.wifi_ssid, 32);
    // 只有输入了新密码才更新密码
    if (new_pass.length() > 0) {
      new_pass.toCharArray(config.wifi_pass, 64);
    }
  }

  saveConfig();
  
  // 如果改了 WiFi，显示提示并重启
  if (new_ssid.length() > 0) {
    server.send(200, "text/html; charset=utf-8", "<h1>保存成功！设备正在重启...</h1><p>请连接新 WiFi 后刷新。</p>");
    delay(1000);
    ESP.restart(); // ★ 重启以应用新网络
  } else {
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

// ... handleCalibrateWeb (略) ...
void handleCalibrateWeb() {
  calibrateMPU(); 
  server.sendHeader("Location", "/");
  server.send(303); 
}

// ... makeDataPacket (略) ...
void makeDataPacket() {
    // ... 代码不变 ...
    memset(udpDataOut, 0, 100);
    udpDataOut[0]='D'; udpDataOut[1]='S'; udpDataOut[2]='U'; udpDataOut[3]='S';
    udpDataOut[4]=0xE9; udpDataOut[5]=0x03; udpDataOut[6]=84; 
    udpDataOut[16]=0x02; udpDataOut[18]=0x10;
    udpDataOut[20]=0x00; udpDataOut[21]=0x02; udpDataOut[22]=0x02; udpDataOut[23]=0x02;
    for(int i=0; i<6; i++) udpDataOut[24+i] = macAddress[i];
    udpDataOut[30]=0x05; udpDataOut[31]=0x01;
    memcpy(&udpDataOut[32], &packetCount, 4);
    
    uint32_t ts = micros();
    memcpy(&udpDataOut[68], &ts, 4);
    memcpy(&udpDataOut[76], &ax_f, 4); memcpy(&udpDataOut[80], &ay_f, 4); memcpy(&udpDataOut[84], &az_f, 4);
    memcpy(&udpDataOut[88], &gx_f, 4); memcpy(&udpDataOut[92], &gy_f, 4); memcpy(&udpDataOut[96], &gz_f, 4);
    
    uint32_t crc = CRC32::calculate(udpDataOut, 100);
    memcpy(&udpDataOut[8], &crc, 4);
}

#endif