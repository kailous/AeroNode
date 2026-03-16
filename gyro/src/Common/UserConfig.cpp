#include "UserConfig.h"

Preferences prefs;

// Defaults
char wifiSSID[32] = "Kailous_2.4G";
char wifiPass[64] = "Kailous309999811";
uint16_t udpPort = 26760;

const int OLED_SDA = 5;
const int OLED_SCL = 4;
const int OLED_RST = 16;

const int MPU_INT = 25;
const int MPU_SCL = 26;
const int MPU_SDA = 27;
const int MPU_VCC = 14;
const int MPU_GND = 12;

const uint8_t mpuAddr = 0x68;

const int BUTTON_LEFT = 0;

char bleName[32] = "IMU_Air_Mouse";
char customMac[18] = "00:00:00:00:00:00"; // Default: use system MAC
float sensitivity = 30.0;
float deadzone = 0.05;
float gyroZ_offset = 0.0;
float gyroX_offset = 0.0;

int16_t offsetTable[6] = {-2930, -23, 1562, 44, -10, 39};
float gyrOffYF = 0;

float batteryVoltageFull = 4.2f;
float batteryVoltageEmpty = 3.0f;
float batteryDividerRatio = 2.0f;
bool debugLog = false;

OledDisplay display(OLED_SDA, OLED_SCL);
SerialPlotter plotter(false);
WebConfig webConfig;

void loadConfig() {
  prefs.begin("imupod", false);

  String savedSSID = prefs.getString("ssid", wifiSSID);
  String savedPass = prefs.getString("pass", wifiPass);
  strncpy(wifiSSID, savedSSID.c_str(), sizeof(wifiSSID) - 1);
  wifiSSID[sizeof(wifiSSID) - 1] = '\0';
  strncpy(wifiPass, savedPass.c_str(), sizeof(wifiPass) - 1);
  wifiPass[sizeof(wifiPass) - 1] = '\0';

  udpPort = prefs.getUShort("udp", udpPort);

  String savedBleName = prefs.getString("ble_name", bleName);
  String savedMac = prefs.getString("mac", customMac);
  strncpy(bleName, savedBleName.c_str(), sizeof(bleName) - 1);
  bleName[sizeof(bleName) - 1] = '\0';
  strncpy(customMac, savedMac.c_str(), sizeof(customMac) - 1);
  customMac[sizeof(customMac) - 1] = '\0';

  sensitivity = prefs.getFloat("sens", sensitivity);
  deadzone = prefs.getFloat("dead", deadzone);
  gyroX_offset = prefs.getFloat("gx_off", gyroX_offset);
  gyroZ_offset = prefs.getFloat("gz_off", gyroZ_offset);

  offsetTable[0] = prefs.getShort("ax_off", offsetTable[0]);
  offsetTable[1] = prefs.getShort("ay_off", offsetTable[1]);
  offsetTable[2] = prefs.getShort("az_off", offsetTable[2]);
  offsetTable[3] = prefs.getShort("kx_off", offsetTable[3]);
  offsetTable[4] = prefs.getShort("ky_off", offsetTable[4]);
  offsetTable[5] = prefs.getShort("kz_off", offsetTable[5]);

  gyrOffYF = prefs.getFloat("gy_drift", gyrOffYF);

  batteryVoltageFull = prefs.getFloat("bat_f", batteryVoltageFull);
  batteryVoltageEmpty = prefs.getFloat("bat_e", batteryVoltageEmpty);
  batteryDividerRatio = prefs.getFloat("bat_div", batteryDividerRatio);
  debugLog = prefs.getBool("debug", debugLog);

  prefs.end();
}

void saveConfig() {
  prefs.begin("imupod", false);

  prefs.putString("ssid", wifiSSID);
  prefs.putString("pass", wifiPass);
  prefs.putUShort("udp", udpPort);

  prefs.putString("ble_name", bleName);
  prefs.putString("mac", customMac);

  prefs.putFloat("sens", sensitivity);
  prefs.putFloat("dead", deadzone);
  prefs.putFloat("gx_off", gyroX_offset);
  prefs.putFloat("gz_off", gyroZ_offset);

  prefs.putShort("ax_off", offsetTable[0]);
  prefs.putShort("ay_off", offsetTable[1]);
  prefs.putShort("az_off", offsetTable[2]);
  prefs.putShort("kx_off", offsetTable[3]);
  prefs.putShort("ky_off", offsetTable[4]);
  prefs.putShort("kz_off", offsetTable[5]);

  prefs.putFloat("gy_drift", gyrOffYF);

  prefs.putFloat("bat_f", batteryVoltageFull);
  prefs.putFloat("bat_e", batteryVoltageEmpty);
  prefs.putFloat("bat_div", batteryDividerRatio);
  prefs.putBool("debug", debugLog);

  prefs.end();
}
