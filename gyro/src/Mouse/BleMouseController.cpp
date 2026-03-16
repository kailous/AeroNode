#include "BleMouseController.h"

BleMouseController::BleMouseController() 
    : _bleMouse(nullptr), _lastButtonState(HIGH), _lastPrintTime(0) {}

BleMouseController::~BleMouseController() {
    if (_bleMouse != nullptr) {
        delete _bleMouse;
    }
}

void BleMouseController::begin(const char* deviceName, const char* deviceManufacturer, uint8_t batteryLevel) {
    if (_bleMouse != nullptr) {
        delete _bleMouse;
    }
    _bleMouse = new BleMouse(deviceName, deviceManufacturer, batteryLevel);
    _bleMouse->begin();
}

bool BleMouseController::isConnected() {
    return _bleMouse && _bleMouse->isConnected();
}

void BleMouseController::update(float gyrPF, float gyrRF, bool buttonPressed, 
                                float deadzone, float sensitivity, 
                                float gyroX_offset, float gyroZ_offset) {
    if (!isConnected()) {
        return;
    }

    // 1. 处理鼠标按键逻辑
    // 假设 buttonPressed 传进来的是数字引脚的物理电平状态
    // 低电平 (LOW = false) 代表按下, 高电平 (HIGH = true) 代表释放
    if (buttonPressed != _lastButtonState) {
        if (!buttonPressed) { // LOW
            _bleMouse->press(MOUSE_LEFT);
        } else {
            _bleMouse->release(MOUSE_LEFT);
        }
        _lastButtonState = buttonPressed;
    }

    // 2. 取自 Adafruit_MPU6050 的真实弧度/秒 (rad/s)，无需再做 57.3 的放缩换算
    // 完全复刻 m.ino (备份文件) 的映射姿势
    // gz 对应绕 Z 轴的旋转幅度，控制鼠标的水平移动 (moveX)
    float gz = (gyrRF - gyroZ_offset) * -1.0f;
    // gx 对应绕 X 轴的旋转幅度，控制鼠标的垂直移动 (moveY)
    float gx = (gyrPF - gyroX_offset);

    int moveX = 0;
    int moveY = 0;

    // 3. 应用死区与灵敏度
    if (abs(gz) > deadzone) {
        moveX = (int)(gz * sensitivity);
    }
    if (abs(gx) > deadzone) {
        moveY = (int)(gx * sensitivity);
    }

    // 4. 发送移动指令和调试分发
    if (moveX != 0 || moveY != 0) {
        _bleMouse->move(moveX, moveY);
    }

    // 可选：限流调试输出 (每 500ms 打印一次移动)
    if (millis() - _lastPrintTime > 500) {
        _lastPrintTime = millis();
        Serial.printf("BleMouse -> gz:%.2f gx:%.2f | db:%.2f sens:%.2f | moveX:%d moveY:%d\n", 
                      gz, gx, deadzone, sensitivity, moveX, moveY);
    }
}
