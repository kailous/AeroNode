#include <Wire.h>

// 定义我们要测试的两组引脚
// 方案 A: 你的 Config.h 默认配置
const int PIN_A_SDA = 14; // D5
const int PIN_A_SCL = 12; // D6

// 方案 B: 自动反转配置
const int PIN_B_SDA = 12; // D6
const int PIN_B_SCL = 14; // D5

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n=== Phoenix Smart Scanner Start ===");
}

void loop() {
  int devicesFound = 0;

  // ==========================================
  // 尝试方案 A (默认线序)
  // ==========================================
  Serial.print("Trying Config A (SDA=D5/14, SCL=D6/12)... ");
  Wire.begin(PIN_A_SDA, PIN_A_SCL);
  devicesFound = scanI2C();

  if (devicesFound > 0) {
    Serial.println("✅ SUCCESS! (Using Config A)");
    Serial.println(">>> 你的 Config.h 设置是正确的，不用改！");
  } 
  else {
    Serial.println("❌ Failed.");
    
    // ==========================================
    // 自动切换到方案 B (反转线序)
    // ==========================================
    delay(100); // 稍微歇一下让总线复位
    Serial.print("Trying Config B (SDA=D6/12, SCL=D5/14)... ");
    Wire.begin(PIN_B_SDA, PIN_B_SCL); // ★ 自动调换引脚
    devicesFound = scanI2C();

    if (devicesFound > 0) {
      Serial.println("✅ SUCCESS! (Using Config B)");
      Serial.println("⚠️ 注意：你的线接反了！");
      Serial.println(">>> 请修改 Config.h: MPU_SDA=12, MPU_SCL=14");
    } else {
      Serial.println("❌ Failed.");
      Serial.println("💀 两次尝试都失败。请检查：");
      Serial.println("   1. 杜邦线是否断了？");
      Serial.println("   2. 模块供电(VCC/GND)是否正常？");
      Serial.println("   3. 模块是否已损坏？");
    }
  }

  Serial.println("----------------------------------------");
  delay(3000); // 3秒后再试一轮
}

// 独立的扫描函数
int scanI2C() {
  byte error, address;
  int count = 0;

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      if (count == 0) Serial.println(""); // 换行美观
      Serial.print("   -> Found device at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      count++;
    }
  }
  return count;
}#include <Wire.h>

// 定义我们要测试的两组引脚
// 方案 A: 你的 Config.h 默认配置
const int PIN_A_SDA = 14; // D5
const int PIN_A_SCL = 12; // D6

// 方案 B: 自动反转配置
const int PIN_B_SDA = 12; // D6
const int PIN_B_SCL = 14; // D5

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\n\n=== Phoenix Smart Scanner Start ===");
}

void loop() {
  int devicesFound = 0;

  // ==========================================
  // 尝试方案 A (默认线序)
  // ==========================================
  Serial.print("Trying Config A (SDA=D5/14, SCL=D6/12)... ");
  Wire.begin(PIN_A_SDA, PIN_A_SCL);
  devicesFound = scanI2C();

  if (devicesFound > 0) {
    Serial.println("✅ SUCCESS! (Using Config A)");
    Serial.println(">>> 你的 Config.h 设置是正确的，不用改！");
  } 
  else {
    Serial.println("❌ Failed.");
    
    // ==========================================
    // 自动切换到方案 B (反转线序)
    // ==========================================
    delay(100); // 稍微歇一下让总线复位
    Serial.print("Trying Config B (SDA=D6/12, SCL=D5/14)... ");
    Wire.begin(PIN_B_SDA, PIN_B_SCL); // ★ 自动调换引脚
    devicesFound = scanI2C();

    if (devicesFound > 0) {
      Serial.println("✅ SUCCESS! (Using Config B)");
      Serial.println("⚠️ 注意：你的线接反了！");
      Serial.println(">>> 请修改 Config.h: MPU_SDA=12, MPU_SCL=14");
    } else {
      Serial.println("❌ Failed.");
      Serial.println("💀 两次尝试都失败。请检查：");
      Serial.println("   1. 杜邦线是否断了？");
      Serial.println("   2. 模块供电(VCC/GND)是否正常？");
      Serial.println("   3. 模块是否已损坏？");
    }
  }

  Serial.println("----------------------------------------");
  delay(3000); // 3秒后再试一轮
}

// 独立的扫描函数
int scanI2C() {
  byte error, address;
  int count = 0;

  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      if (count == 0) Serial.println(""); // 换行美观
      Serial.print("   -> Found device at 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      count++;
    }
  }
  return count;
}