#ifndef HARDWARE_H
#define HARDWARE_H

#include "Config.h"

void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, config);
  if (config.check_code != 66666) {
    Config d; memset(d.wifi_ssid, 0, 32); 
    d.inv_gx=d.inv_gy=d.inv_gz=0;
    d.offset_gx=d.offset_gy=d.offset_gz=0; 
    d.dlpf_mode=2; d.check_code=66666;
    config = d; EEPROM.put(0, config); EEPROM.commit();
  }
}

void saveConfig() { 
  EEPROM.put(0, config); 
  EEPROM.commit(); 
  mpu.setDLPFMode(config.dlpf_mode); 
}

void initMPU() { 
  Wire.begin(MPU_SDA, MPU_SCL); 
  Wire.setClock(400000); 
  mpu.initialize(); 
  mpu.setDLPFMode(config.dlpf_mode); 
}

void calibrateMPU() {
  long gxs=0, gys=0, gzs=0;
  for (int i=0; i<1000; i++) { 
    mpu.getMotion6(&ax,&ay,&az,&gx,&gy,&gz); 
    gxs+=gx; gys+=gy; gzs+=gz; delay(1); 
  }
  base_gx=gxs/1000; base_gy=gys/1000; base_gz=gzs/1000;
}
#endif