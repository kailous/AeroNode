#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <Arduino.h>

// 引用主程序中定义的姿态变量，以便 swapTable 可以获取它们的地址
extern int16_t accXI, accYI, accZI, gyrPI, gyrYI, gyrRI;

// =======================================================================================
// 校准参数配置
// =======================================================================================

// 1. 轴向映射 (Swap Table)
// 调整 MPU6050 的轴向以匹配控制器的实际安装方向
// 数组顺序对应 MPU6050 原始输出: [AccX, AccY, AccZ, GyroX, GyroY, GyroZ]
// 指针指向目标变量: &accXI, &accYI, &accZI, &gyrPI, &gyrYI, &gyrRI
static int16_t* swapTable[] =
{
  &accXI, // MPU AccX -> accXI
  &accZI, // MPU AccY -> accZI
  &accYI, // MPU AccZ -> accYI
  &gyrPI, // MPU GyroX -> gyrPI
  &gyrRI, // MPU GyroY -> gyrRI
  &gyrYI, // MPU GyroZ -> gyrYI
};

// 2. 符号校准 (Sign Table)
// 为 true 时取反，用于修正轴向的正负方向
// 对应顺序: [accXI, accYI, accZI, gyrPI, gyrYI, gyrRI]
static const bool signTable[] =
{
  false, // accXI
  true,  // accYI
  true,  // accZI
  true,  // gyrPI
  true,  // gyrYI
  true,  // gyrRI
};

// 3. 零偏校准 (Offset Table)
// 对应顺序: [accXI, accYI, accZI, gyrPI, gyrYI, gyrRI]
static const int16_t offsetTable[] =
{
  -2930, // accXI
  -23,   // accYI
  1562,  // accZI
  44,    // gyrPI
  -10,   // gyrYI
  39,    // gyrRI
};

// 4. 漂移校准
// 可选：手柄静止时填入 PadTest 的 Gyro Y 来消除漂移
static const float gyrOffYF = 0;

#endif // CALIBRATION_H