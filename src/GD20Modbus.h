#ifndef YARR_GD20MODBUS_H
#define YARR_GD20MODBUS_H

#include "config.h"
#include <Arduino.h>

#ifdef YB_HAS_MODBUS

  #include <ModbusMaster.h>

  // --- GD20 register map ---
  #define REG_CONTROL  0x2000
  #define REG_FREQ_SET 0x2001
  #define REG_STATUS   0x2100
  #define REG_FREQ_FB  0x2103

class GD20Modbus
{
  public:
    GD20Modbus(uint8_t slaveID, HardwareSerial& serial, int rxPin, int txPin);
    void begin();
    void setFrequency(float freqHz);
    void runMotor();
    void stopMotor();
    float readOutputFreq();
    uint16_t readStatusWord();
    void decodeStatus(uint16_t status);

  private:
    uint8_t _slaveID;
    int _rxPin;
    int _txPin;
    HardwareSerial& _serial;
    static ModbusMaster node;
};

#endif
#endif