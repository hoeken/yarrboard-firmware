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
class RelayController;
class ServoController;
class StepperController;

class BrineomaticController : public BaseController
{
  public:
    BrineomaticController(YarrboardApp& app, RelayController& relays, ServoController& servos, StepperController& steppers);

    bool setup() override;
    void loop() override;

    bool loadConfigHook(JsonVariant config, char* error, size_t len) override;
    void generateConfigHook(JsonVariant config) override;
    void generateUpdateHook(JsonVariant output) override;
    void generateStatsHook(JsonVariant output) override;

    void handleStartWatermaker(JsonVariantConst input, JsonVariant output);
    void handleFlushWatermaker(JsonVariantConst input, JsonVariant output);
    void handlePickleWatermaker(JsonVariantConst input, JsonVariant output);
    void handleDepickleWatermaker(JsonVariantConst input, JsonVariant output);
    void handleStopWatermaker(JsonVariantConst input, JsonVariant output);
    void handleIdleWatermaker(JsonVariantConst input, JsonVariant output);
    void handleManualWatermaker(JsonVariantConst input, JsonVariant output);
    void handleSetWatermaker(JsonVariantConst input, JsonVariant output);
    void handleSaveGeneralConfig(JsonVariantConst input, JsonVariant output);
    void handleSaveHardwareConfig(JsonVariantConst input, JsonVariant output);
    void handleSaveSafeguardsConfig(JsonVariantConst input, JsonVariant output);

  private:
    Brineomatic wm;

    uint32_t lastOutput;

    // The actual state machine logic
    void stateMachine();

    // The static wrapper that FreeRTOS can call
    static void stateMachineTask(void* pvParameters);
};

#endif /* !YARR_BRINEOMATIC_CONTROLLER_H */