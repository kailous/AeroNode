#include <BleMouse.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>

// --- 引脚定义 ---
#define OLED_SDA 5
#define OLED_SCL 4
#define OLED_RST 16
#define MPU_SCL 26
#define MPU_SDA 27
#define MPU_VCC 14
#define MPU_GND 12

// 鼠标左键定义为 BOOT 键 (GPIO 0)
#define BUTTON_LEFT 0

BleMouse bleMouse("Mac_Air_Mouse_Tuned", "Lipeng_Design", 100);
Adafruit_MPU6050 mpu;
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RST);

// ==========================================
// 🛠️ 核心调教区 
// ==========================================
float sensitivity = 30.0; 
float deadzone = 0.05;
float gyroZ_offset = 0.0; 
float gyroX_offset = 0.0; 
// ==========================================

// 记录按键的上一状态，用于检测按下的“瞬间”和松开的“瞬间”
bool lastButtonState = HIGH; 

void setup() {
  Serial.begin(115200);

  // 初始化 BOOT 按键为输入，并开启内部上拉电阻
  pinMode(BUTTON_LEFT, INPUT_PULLUP);

  // 1. 硬件供电设置
  pinMode(MPU_VCC, OUTPUT);
  digitalWrite(MPU_VCC, HIGH); 
  pinMode(MPU_GND, OUTPUT);
  digitalWrite(MPU_GND, LOW);  
  delay(500); 

  // 2. 初始化板载 OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display.setRotation(2);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println("MPU + Mouse L-Click");
    display.display();
  }

  // 3. 初始化外接 MPU6050 (使用 Wire1)
  Wire1.begin(MPU_SDA, MPU_SCL, 400000); 
  if (!mpu.begin(0x68, &Wire1)) {
    Serial.println("MPU6050 init failed!");
    while(1) delay(10);
  }

  mpu.setGyroRange(MPU6050_RANGE_250_DEG); 
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    
    // --------------------------------------------------
    // 🖱️ 处理鼠标按键逻辑 (BOOT 键)
    // --------------------------------------------------
    // 读取当前按键状态（按下为 LOW，松开为 HIGH）
    bool currentButtonState = digitalRead(BUTTON_LEFT);
    
    // 如果状态发生了改变（说明用户按下或松开了按键）
    if (currentButtonState != lastButtonState) {
      if (currentButtonState == LOW) {
        // 按键被按下
        bleMouse.press(MOUSE_LEFT);
      } else {
        // 按键被松开
        bleMouse.release(MOUSE_LEFT);
      }
      lastButtonState = currentButtonState;
    }

    // --------------------------------------------------
    // 🧭 处理陀螺仪移动逻辑
    // --------------------------------------------------
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float gz = (g.gyro.z - gyroZ_offset) * -1.0; 
    float gx = (g.gyro.x - gyroX_offset); 

    int moveX = 0, moveY = 0;

    if (abs(gz) > deadzone) moveX = (int)(gz * sensitivity);
    if (abs(gx) > deadzone) moveY = (int)(gx * sensitivity);

    if (moveX != 0 || moveY != 0) {
      bleMouse.move(moveX, moveY);
    }

    // OLED 刷新
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 200) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.println("CONNECTED");
      display.printf("X: %d  Y: %d\n", moveX, moveY);
      
      // 在屏幕上显示左键状态
      if (currentButtonState == LOW) {
        display.println("[L-CLICK PRESSED]");
      } else {
        display.println("[L-CLICK RELEASED]");
      }
      
      display.display();
      lastUpdate = millis();/Users/lipeng/Downloads/arduino-littlefs-upload-1.6.3.vsix
    }
  }
  
  // 10ms 延迟，既控制了陀螺仪刷新率，也起到了按键防抖的作用
  delay(10); 
}