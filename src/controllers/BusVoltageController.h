/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BUS_VOLTAGE_H
#define YARR_BUS_VOLTAGE_H

#include "adchelper.h"
#include "config.h"
#include <Arduino.h>

#include "controllers/BaseController.h"
#include <ArduinoJson.h>
#include <PsychicMqttClient.h>

class YarrboardApp;
class ConfigManager;

class BusVoltageController : public BaseController
{
  public:
    BusVoltageController(YarrboardApp& app);

    bool setup() override;
    void loop() override;
    float getBusVoltage();

    void generateConfigHook(JsonVariant config) override;
    void generateUpdateHook(JsonVariant output) override;
    void generateFastUpdateHook(JsonVariant output) override;
    void generateStatsHook(JsonVariant output) override;

    byte address = 0x68;
    float r1 = 1.0;
    float r2 = 1.0;

  private:
    unsigned long lastBusVoltageCheckMillis = 0;

    MCP342x* _adcMCP3425;
    MCP3425Helper* busADC = nullptr;
};

#endif /* !YARR_BUS_VOLTAGE_H */