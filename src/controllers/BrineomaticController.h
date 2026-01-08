/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_CONTROLLER_H
#define YARR_BRINEOMATIC_CONTROLLER_H

#include "config.h"

#ifdef YB_IS_BRINEOMATIC
  #include "adchelper.h"

  #include <Arduino.h>

  #include "Brineomatic.h"
  #include <ArduinoJson.h>
  #include <PsychicMqttClient.h>
  #include <controllers/BaseController.h>
  #include <controllers/ProtocolController.h>

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
    void generateCapabilitiesHook(JsonVariant config) override;
    void generateUpdateHook(JsonVariant output) override;
    void generateStatsHook(JsonVariant output) override;
    void mqttUpdateHook(MQTTController* mqtt) override;
    void haUpdateHook(MQTTController* mqtt) override;

    void handleStartWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleFlushWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handlePickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleDepickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleStopWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleIdleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleManualWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSetWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveGeneralConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveHardwareConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveSafeguardsConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);

  private:
    Brineomatic wm;

    uint32_t lastOutput;

    // The actual state machine logic
    void stateMachine();

    // The static wrapper that FreeRTOS can call
    static void stateMachineTask(void* pvParameters);
};

#endif
#endif /* !YARR_BRINEOMATIC_CONTROLLER_H */