# ESP8266-MPU6050-DSU

一个基于 **ESP8266 + MPU6050 + OLED** 的 DSU 体感开发项目，提供 200Hz 姿态数据采样，通过 UDP 向支持 Cemuhook/DSU 协议的客户端输出。

## 功能特性
- MPU6050 姿态数据采集（加速度 + 陀螺仪）
- 200Hz UDP 输出，兼容 Cemuhook/DSU 客户端
- OLED 状态显示（连接/校准/运行）
- Web 配置面板（WiFi、方向反转、偏移、DLPF 降噪、校准）
- 失败自动切 AP 模式，方便首配

## 硬件与接线
**硬件**
- ESP8266 开发板（如 NodeMCU/WeMos D1 mini）
- MPU6050
- 0.96" I2C OLED (SSD1306)

**默认接线（见 `cemuhook/Config.h`）**
- OLED: `SDA=D2(GPIO4)` / `SCL=D1(GPIO5)`
- MPU: `SDA=D5(GPIO14)` / `SCL=D6(GPIO12)`

> 如 MPU6050 无法识别，可使用 `I2C_Scanner_Start/I2C_Scanner_Start.ino` 自动检测线序，必要时把 MPU 的 SDA/SCL 对调并修改 `Config.h`。

## 依赖库
请在 Arduino IDE 或 PlatformIO 中安装：
- **ESP8266 core**
- **MPU6050**（I2Cdevlib 版本）
- **Adafruit GFX**
- **Adafruit SSD1306**
- **CRC32**

> 这些库默认放在本地 `libraries/`，该目录已加入 `.gitignore`，不会被提交。

## 目录结构
```
.
├─ cemuhook/                 # 主固件 (DSU 输出 + Web 配置)
│  ├─ cemuhook.ino
│  ├─ Config.h
│  ├─ Hardware.h
│  ├─ Network.h
│  └─ WebConfig.h
├─ I2C_Scanner_Start/         # I2C 线序检测工具
│  └─ I2C_Scanner_Start.ino
└─ README.md
```

## 烧录与使用
1. 打开 `cemuhook/cemuhook.ino`，选择正确的 ESP8266 开发板和端口。
2. 编译并烧录。
3. 设备启动后：
   - **已保存 WiFi**：自动连接并显示 IP。
   - **未保存/连接失败**：自动开启 AP。
     - SSID: `Phoenix_AP`
     - 密码: `12345678`
     - Web 配置地址: `http://192.168.4.1`
4. Web 页面可配置方向反转、偏移、DLPF、校准等。
5. 在支持 DSU 的客户端中填写设备 IP，端口为 **26760**。

## Web 配置说明
- **WiFi**：SSID/Pass 留空则不修改；保存后自动重启。
- **降噪(DLPF)**：不同等级对应不同滤波强度。
- **反转/偏移**：用于修正轴向与零漂。
- **重新校准**：固定设备后点击可重新校准陀螺仪。

## 常见问题
- **MPU6050 无数据**：先用 `I2C_Scanner_Start` 检查是否接反 SDA/SCL。
- **Web 进不去**：确认是否在同一 WiFi/热点内，AP 模式用 `192.168.4.1`。
- **DSU 无数据**：确认客户端设置了正确 IP 和端口 `26760`，并有握手请求。

## 免责声明
本项目用于学习与开发用途，请勿用于需要严格安全/精度保障的场景。
