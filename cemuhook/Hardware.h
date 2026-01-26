#ifndef HARDWARE_H
#define HARDWARE_H

#include "Config.h"

// === EEPROM 存取 ===
void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, config);
  
  if (config.check_code != 66666) {
    Config defaultConfig;
    memset(defaultConfig.wifi_ssid, 0, 32);
    memset(defaultConfig.wifi_pass, 0, 64);
    
    defaultConfig.inv_gx = false;
    defaultConfig.inv_gy = false;
    defaultConfig.inv_gz = false;
    defaultConfig.offset_gx = 0;
    defaultConfig.offset_gy = 0;
    defaultConfig.offset_gz = 0;
    defaultConfig.dlpf_mode = 2;
    defaultConfig.check_code = 66666;

    config = defaultConfig;
    EEPROM.put(0, config);
    EEPROM.commit();
  }
}

void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
  mpu.setDLPFMode(config.dlpf_mode);
}

// === 屏幕显示 (优化 AP 模式显示) ===
void showStatus(String title, String info, bool big = false) {
  // 1. 切到 OLED 引脚
  Wire.begin(OLED_SDA, OLED_SCL);
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  if (big) {
    // 大字模式 (游戏时)
    display.setTextSize(2);
    display.println(title);
    display.setTextSize(1);
    display.println("");
    display.println(info);
  } else {
    // 普通模式 (信息显示)
    display.setTextSize(1);
    display.println(title);
    display.println("---------------------");

    if (isAPMode) {
      // ★ AP 模式：详细显示账号密码
      display.println("SSID: Phoenix_AP");
      display.println("PASS: 12345678"); // ★ 这里显示密码
      display.println("");
      display.println("IP:   192.168.4.1");
    } else {
      // STA 模式：显示连接状态和 IP
      display.println(info);
      display.println("");
      display.print("IP: "); display.println(WiFi.localIP());
    }
  }
  display.display();

  // 2. 切回 MPU 引脚
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(400000);
}

// === MPU 初始化 ===
void initMPU() {
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(400000);
  mpu.initialize();
  mpu.setFullScaleGyroRange(0);
  mpu.setFullScaleAccelRange(0);
  mpu.setDLPFMode(config.dlpf_mode);
}

// === 校准 ===
void calibrateMPU() {
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;
  for (int i = 0; i < 1000; i++) {
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    gx_sum += gx; gy_sum += gy; gz_sum += gz;
    delay(1);
  }
  base_gx = gx_sum / 1000;
  base_gy = gy_sum / 1000;
  base_gz = gz_sum / 1000;
}

#endif