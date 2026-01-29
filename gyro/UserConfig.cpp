#include "UserConfig.h"

char wifiSSID[] = "Kailous_2.4G";
char wifiPass[] = "Kailous309999811";
uint16_t udpPort = 26760;

const uint8_t mpuSda = 14;
const uint8_t mpuScl = 12;
const uint8_t mpuAddr = 0x68;

// OLED display: SDA=D2(GPIO4), SCL=D1(GPIO5)
OledDisplay display(4, 5);

// Serial plotter (disabled by default, set true for debugging)
SerialPlotter plotter(false);
