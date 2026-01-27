#ifndef NETWORK_H
#define NETWORK_H

#include "Config.h"
#include "Hardware.h"
#include "WebConfig.h"

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  if (strlen(config.wifi_ssid) > 0) {
    WiFi.begin(config.wifi_ssid, config.wifi_pass);
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 30) { delay(500); timeout++; }
  }
  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
  } else {
    isAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Phoenix_AP", "12345678");
  }
}

// ★ 核心修改：发送解算后的角度
void handleLiveData() {
  String json = "{";
  json += "\"p\":" + String(p, 1) + ",";
  json += "\"r\":" + String(r, 1) + ",";
  json += "\"y\":" + String(y, 1) + ",";
  json += "\"ax\":" + String(ax_f, 3) + ",";
  json += "\"ay\":" + String(ay_f, 3) + ",";
  json += "\"az\":" + String(az_f, 3);
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  String s = index_html; 
  s.replace("%CHK_GX%", config.inv_gx?"checked":"");
  s.replace("%CHK_GY%", config.inv_gy?"checked":"");
  s.replace("%CHK_GZ%", config.inv_gz?"checked":"");
  s.replace("%OFF_GX%", String(config.offset_gx));
  s.replace("%OFF_GY%", String(config.offset_gy));
  s.replace("%OFF_GZ%", String(config.offset_gz));
  s.replace("%SSID%", String(config.wifi_ssid));
  s.replace("%SEL_0%", config.dlpf_mode==0?"selected":"");
  s.replace("%SEL_2%", config.dlpf_mode==2?"selected":"");
  s.replace("%SEL_4%", config.dlpf_mode==4?"selected":"");
  s.replace("%SEL_6%", config.dlpf_mode==6?"selected":"");
  server.send(200, "text/html; charset=utf-8", s);
}

void handleSave() {
  config.inv_gx = server.hasArg("inv_gx");
  config.inv_gy = server.hasArg("inv_gy");
  config.inv_gz = server.hasArg("inv_gz");
  config.offset_gx = server.arg("off_gx").toInt();
  config.offset_gy = server.arg("off_gy").toInt();
  config.offset_gz = server.arg("off_gz").toInt();
  if (server.hasArg("dlpf")) config.dlpf_mode = server.arg("dlpf").toInt();
  String new_ssid = server.arg("ssid");
  String new_pass = server.arg("pass");
  if (new_ssid.length() > 0) {
    new_ssid.toCharArray(config.wifi_ssid, 32);
    if (new_pass.length() > 0) new_pass.toCharArray(config.wifi_pass, 64);
  }
  saveConfig();
  if (new_ssid.length() > 0) {
    server.send(200, "text/html", "Rebooting...");
    delay(1000); ESP.restart();
  } else {
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

void handleCalibrateWeb() { calibrateMPU(); server.sendHeader("Location", "/"); server.send(303); y = 0; }

void makeDataPacket() {
    memset(udpDataOut, 0, 100);
    udpDataOut[0]='D'; udpDataOut[1]='S'; udpDataOut[2]='U'; udpDataOut[3]='S';
    udpDataOut[4]=0xE9; udpDataOut[5]=0x03; udpDataOut[6]=84; 
    udpDataOut[16]=0x02; udpDataOut[18]=0x10;
    for(int i=0; i<6; i++) udpDataOut[24+i] = macAddress[i];
    memcpy(&udpDataOut[32], &packetCount, 4);
    uint32_t ts = micros();
    memcpy(&udpDataOut[68], &ts, 4);
    memcpy(&udpDataOut[76], &ax_f, 4); memcpy(&udpDataOut[80], &ay_f, 4); memcpy(&udpDataOut[84], &az_f, 4);
    memcpy(&udpDataOut[88], &gx_f, 4); memcpy(&udpDataOut[92], &gy_f, 4); memcpy(&udpDataOut[96], &gz_f, 4);
    uint32_t crc = CRC32::calculate(udpDataOut, 100);
    memcpy(&udpDataOut[8], &crc, 4);
}

#endif