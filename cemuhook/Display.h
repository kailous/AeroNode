#ifndef DISPLAY_H
#define DISPLAY_H

#include "Config.h"

// 初始化屏幕
void initDisplay() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.display();
}

// 抽象出的状态显示函数
void showStatus(String title, String info, bool big = false) {
  // 1. 切换 I2C 引脚到 OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  
  display.clearDisplay();
  display.setCursor(0, 0);

  if (big) {
    display.setTextSize(2);
    display.println(title);
    display.setTextSize(1);
    display.println("\n" + info);
  } else {
    display.setTextSize(1);
    display.println(title);
    display.println("---------------------");

    if (isAPMode) {
      display.println("SSID: Phoenix_AP");
      display.println("PASS: 12345678");
      display.println("\nIP: 192.168.4.1");
    } else {
      display.println(info);
      display.print("\nIP: "); 
      display.println(WiFi.localIP());
    }
  }
  display.display();

  // 2. 显示完立即切回 MPU 引脚，保证数据读取不被干扰
  Wire.begin(MPU_SDA, MPU_SCL);
  Wire.setClock(400000);
}

#endif