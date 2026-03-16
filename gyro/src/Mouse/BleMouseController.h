#ifndef BLE_MOUSE_CONTROLLER_H
#define BLE_MOUSE_CONTROLLER_H

#include <BleMouse.h>
#include <Arduino.h>

class BleMouseController {
public:
    BleMouseController();
    ~BleMouseController();

    // 初始化蓝牙鼠标并指定名称
    void begin(const char* deviceName, const char* deviceManufacturer = "Lipeng_Design", uint8_t batteryLevel = 100);

    // 检查是否已连接设备
    bool isConnected();

    // 核心更新方法：每帧调用，传入当前的陀螺仪姿态和按键状态
    void update(float gyrPF, float gyrYF, bool buttonPressed, 
                float deadzone, float sensitivity, 
                float gyroX_offset, float gyroZ_offset);

private:
    BleMouse* _bleMouse;
    bool _lastButtonState;
    uint32_t _lastPrintTime;
};

#endif // BLE_MOUSE_CONTROLLER_H
