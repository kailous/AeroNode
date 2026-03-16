# Gyro 固件 WebUI 调参选项清单

通过对 `gyro` 固件（包含 `UserConfig.h/cpp`、`gyro.ino` 和 `Calibration.h`）的代码分析，可以将适合放入 WebUI 供用户动态调节的参数划分为以下几个主要类别。这些参数可以按不同的层级（如“基础设置”、“传感器校准”、“高级设置”）展示，以防页面过于复杂。

## 1. 基础网络设置 (Network Settings)
这些是最基础的连接配置，目前在 `WebConfig` 中已经部分实现。
- **WiFi 名称 (SSID)**: `wifiSSID` (字符串，当前硬编码默认值如 `"Kailous_2.4G"`)
- **WiFi 密码 (Password)**: `wifiPass` (字符串)
- **UDP 通信端口 (UDP Port)**: `udpPort` (整数，当前默认 `26760`)

## 2. 空中鼠标设置 (Air Mouse / BleMouse Settings)
这些参数直接影响空中鼠标的手感，是用户最常调整的功能性参数：
- **灵敏度 (Sensitivity)**: `sensitivity` (浮点数，默认 `30.0`，控制指针移动的速度)
- **死区大小 (Deadzone)**: `deadzone` (浮点数，默认 `0.05`，用于过滤手部微小的抖动，避免光标漂移)
- **Z轴偏移校准 (Gyro Z Offset)**: `gyroZ_offset` (浮点数，影响偏航/左右移动)
- **X轴偏移校准 (Gyro X Offset)**: `gyroX_offset` (浮点数，影响俯仰/上下移动)

## 3. 传感器硬件校准 (Sensor Calibration - MPU6050)
这部分通常比较硬核，可作为“高级校准 (Advanced Calibration)”页面放入 WebUI：
- **6轴零偏数据 (Offsets)**: 对应 `Calibration.h` 中的 `offsetTable`。当更换具体的 MPU6050 芯片后，用于彻底消除原始静态误差。
  - `AccX` / `AccY` / `AccZ` 零偏
  - `GyroX` / `GyroY` / `GyroZ` 零偏
- **静态漂移补偿 (Drift Calibration)**: 对应 `gyrOffYF`，用于手柄静止时消除 Y 轴的微量漂移（针对特定的协议映射）。
- **陀螺仪量程 (Gyro Sensitivity)**: `gyroSens`（对应 `0~3`，即 `250~2000 deg/s`。当前固定为 `2`，即 `1000 deg/s`。影响动作捕捉的极限与精度）。
- **传感器安装方向 (Axis Remapping & Sign)**:
  - **轴向映射 (Swap Table)**: 允许用户在 WebUI 中根据模块实际安装在手柄内部的方向重新绑定物理轴到逻辑轴。
  - **各轴反转 (Sign Table)**: 用于修正某个轴的正负方向 (`true`/`false`)。
- **采样数据输出率 (Sample Rate Divider)**: `mpu.setRate(9)` 对应 100Hz，有特殊需求的用户可以改这个值调节刷新率。

## 4. 系统与电源管理设置 (System & Power Management)
这部分可用于适配不同的硬件版本或电池：
- **满电电压阈值 (Battery Full Threshold)**: `batteryVoltageFull` (浮点，默认 `5.1f`)
- **空电关机阈值 (Battery Empty Threshold)**: `batteryVoltageEmpty` (浮点，默认 `4.0f`)
- **ADC分压比例 (Battery Divider Ratio)**: `batteryDividerRatio` (浮点，默认 `2.0f`。根据硬件实际分压电阻微调电量测量的准确性)。
- **无操作关机超时 (Data Request Timeout)**: `dataRequestTimeout` (当前默认为 `0` 禁用。可让用户设置几分钟没有连接 UDP 就进入深度休眠 (Deep Sleep) 省电)。
- **串口调试/绘图仪 (Serial Plotter Debug)**: `debugLog` 或 `plotter(false)` 开关，用户能在 WebUI 开启日志以便连接电脑时获取诊断输出。
