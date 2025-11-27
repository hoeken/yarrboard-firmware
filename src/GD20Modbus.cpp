#include "GD20Modbus.h"
#include "debug.h"

ModbusMaster GD20Modbus::node;

GD20Modbus::GD20Modbus(uint8_t slaveID, HardwareSerial& serial, int rxPin, int txPin)
    : _slaveID(slaveID), _serial(serial), _rxPin(rxPin), _txPin(txPin) {}

void GD20Modbus::begin()
{
  pinMode(YB_MODBUS_DERE_PIN, OUTPUT);
  digitalWrite(YB_MODBUS_DERE_PIN, LOW);

  _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
  node.begin(_slaveID, _serial);

  node.preTransmission([]() {
    digitalWrite(YB_MODBUS_DERE_PIN, HIGH);
  });

  node.postTransmission([]() {
    digitalWrite(YB_MODBUS_DERE_PIN, LOW);
  });

  YBP.println("GD20 Modbus initialized");
}

void GD20Modbus::setFrequency(float freqHz)
{
  uint16_t value = (uint16_t)(freqHz * 100);
  node.writeSingleRegister(REG_FREQ_SET, value);
  YBP.printf("Set frequency: %.2f Hz\n", freqHz);
}

void GD20Modbus::runMotor()
{
  node.writeSingleRegister(REG_CONTROL, 0x0001);
  YBP.println("Motor RUN");
}

void GD20Modbus::stopMotor()
{
  node.writeSingleRegister(REG_CONTROL, 0x0000);
  YBP.println("Motor STOP");
}

float GD20Modbus::readOutputFreq()
{
  uint8_t result = node.readHoldingRegisters(REG_FREQ_FB, 1);
  if (result == node.ku8MBSuccess) {
    return node.getResponseBuffer(0) / 100.0;
  } else {
    YBP.println("⚠️ Read frequency error");
    return -1;
  }
}

uint16_t GD20Modbus::readStatusWord()
{
  uint8_t result = node.readHoldingRegisters(REG_STATUS, 1);
  if (result == node.ku8MBSuccess) {
    return node.getResponseBuffer(0);
  } else {
    YBP.println("⚠️ Read status error");
    return 0xFFFF;
  }
}

void GD20Modbus::decodeStatus(uint16_t status)
{
  YBP.printf("Status Word: 0x%04X\n", status);

  if (status & 0x0001)
    YBP.println(" - Drive Ready");
  if (status & 0x0002)
    YBP.println(" - Running");
  if (status & 0x0004)
    YBP.println(" - Fault Active");
  if (status & 0x0008)
    YBP.println(" - Reverse Running");
  if (status & 0x0010)
    YBP.println(" - Frequency Attained");
  if (status & 0x0020)
    YBP.println(" - Current Limit Active");
  if (status & 0x0040)
    YBP.println(" - Frequency Limit Active");
  if (status & 0x0080)
    YBP.println(" - Acc/Dec Running");
}