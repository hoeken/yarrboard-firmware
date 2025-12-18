/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_CONTROLLER_H
#define YARR_BRINEOMATIC_CONTROLLER_H

#include "adchelper.h"
#include "config.h"
#include <Arduino.h>

#include "Brineomatic.h"
#include "controllers/BaseController.h"
#include <ArduinoJson.h>
#include <PsychicMqttClient.h>

class YarrboardApp;
class ConfigManager;

class BrineomaticController : public BaseController
{
  public:
    BrineomaticController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void generateConfigHook(JsonVariant config) override;
    void generateUpdateHook(JsonVariant output) override;
    void generateFastUpdateHook(JsonVariant output) override;
    void generateStatsHook(JsonVariant output) override;

  private:
    Brineomatic wm;
    uint32_t lastOutput;

    // The actual state machine logic
    void stateMachine();

    // The static wrapper that FreeRTOS can call
    static void stateMachineTask(void* pvParameters);
};

#endif /* !YARR_BRINEOMATIC_CONTROLLER_H */