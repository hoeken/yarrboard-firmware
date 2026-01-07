/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#include "BusVoltageController.h"
#include <YarrboardDebug.h>

BusVoltageController::BusVoltageController(YarrboardApp& app) : BaseController(app, "bus_voltage")
{
}

bool BusVoltageController::setup()
{
  _adcMCP3425 = new MCP342x(address);
  MCP342x::Config cfg(MCP342x::channel1, MCP342x::oneShot, MCP342x::resolution16, MCP342x::gain1);
  busADC = new MCP3425Helper(cfg, 2.048f, _adcMCP3425);

#ifdef YB_I2C_SDA_PIN
  Wire.begin(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
#endif
#ifdef YB_I2C_SPEED
  Wire.setClock(YB_I2C_SPEED);
#endif

  return true;
}

void BusVoltageController::loop()
{
  busADC->onLoop();
}

void BusVoltageController::generateConfigHook(JsonVariant config)
{
  config["bus_voltage"] = true;
}

void BusVoltageController::generateUpdateHook(JsonVariant output)
{
  output["bus_voltage"] = getBusVoltage();
};

void BusVoltageController::generateFastUpdateHook(JsonVariant output)
{
  generateUpdateHook(output);
};

void BusVoltageController::generateStatsHook(JsonVariant output)
{
  generateUpdateHook(output);
};

float BusVoltageController::getBusVoltage()
{
  float voltage = busADC->getAverageVoltage(0);

  return voltage / (r2 / (r2 + r1));
}