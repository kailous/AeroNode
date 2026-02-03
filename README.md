# ESP8266-MPU6050-DSU

本仓库基于一个开源库进行改造，作为 **ESP8266 + MPU6050** 的 DSU 项目脚手架。
当前处于重构阶段：保留可用说明，逐步替换旧实现。

> 说明：原始完整代码已移入本地 `backup/`，该目录在 `.gitignore` 中被忽略，不会上传。

## 重构说明
- 这是“在原库基础上重构”的工程，不是完全重写。
- **硬件接线保持不变**（下方接线说明仍然有效）。
- 旧说明中可复用的内容会保留，但部分细节可能会随着重构调整。

## 目录结构
```
.
├─ calibrate/            # 校准工具（串口交互）
├─ gyro/                 # 采样 + UDP 发送示例
├─ WebPlugins/           # Web 相关插件/资源
├─ podtest/              # Node.js WebSocket 测试服务端
├─ tools/                # 辅助工具与脚本
├─ assistant-notes.md    # AI 协作笔记
└─ README.md
```

## 硬件与接线（仍有效）
**硬件**
- ESP8266 开发板（如 NodeMCU/WeMos D1 mini）
- MPU6050
- 0.96" I2C OLED（SSD1306，可选）

**默认接线**
- OLED: `SDA=D2(GPIO4)` / `SCL=D1(GPIO5)`
- MPU: `SDA=D5(GPIO14)` / `SCL=D6(GPIO12)`

> 如 MPU6050 无法识别，可用 `tools` 或 `backup` 中的 I2C 扫描示例检查线序，必要时对调 SDA/SCL。

## 使用说明（简要）
- **校准**：打开 `calibrate/calibrate.ino`，按串口提示完成校准。
- **发送数据**：打开 `gyro/gyro.ino`，配置 WiFi/端口并烧录。

## 字体
- 项目使用自定义字体 `U8g2CustomFont.h`，确保已包含在项目中。
- 字体文件 `u8g2_font_35006991a13a986eaa75172c1a9cb70e.h` 已被替换为 `U8g2CustomFont.h`。
- 字体转换工具：https://stonez56.com/u8g2/index.php#converter

## 本地备份策略
- `backup/`：历史代码与草稿，仅本地保存，不参与提交。
- `libraries/`：Arduino 本地库目录，同样已忽略。

## 故障排除 (Troubleshooting)

如果 OLED 屏幕显示错误页面，请参考以下错误代码进行排查：

| 错误代码 (Code) | 错误信息 (Message) | 可能原因 (Possible Causes) | 解决方案 (Solutions) |
| :--- | :--- | :--- | :--- |
| **E-01** | MPU Error | **IMU 传感器连接失败**<br>1. MPU6050 模块未连接<br>2. SDA/SCL 接线松动或接反<br>3. 模块供电异常 | 1. 检查 D1 (SCL) 和 D2 (SDA) 接线是否牢固<br>2. 确认 MPU6050 有 3.3V/5V 供电<br>3. 尝试按下 Reset 键重启设备 |
| **(无)** | WiFi Timeout | **WiFi 连接超时**<br>1. SSID 或密码错误<br>2. 路由器信号太弱 | 1. 检查 `UserConfig.cpp` 中的 WiFi 配置<br>2. 靠近路由器重试 |
| **(无)** | AP Config | **配网模式**<br>设备无法连接 WiFi，已自动开启热点 | 1. 电脑/手机连接屏幕显示的 SSID<br>2. 浏览器访问显示的 IP 地址<br>3. 进入 WebUI 配置网络 |

## AI 协作笔记
- 建议在 `assistant-notes.md` 里记录接线、校准结果、改动历史等关键上下文，方便 AI 助手快速接手。
