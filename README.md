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

## 本地备份策略
- `backup/`：历史代码与草稿，仅本地保存，不参与提交。
- `libraries/`：Arduino 本地库目录，同样已忽略。

## AI 协作笔记
- 建议在 `assistant-notes.md` 里记录接线、校准结果、改动历史等关键上下文，方便 AI 助手快速接手。
