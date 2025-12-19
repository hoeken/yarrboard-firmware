/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PWM_CONTROLLER_H
#define YARR_PWM_CONTROLLER_H

#include "config.h"
#ifdef YB_HAS_PWM_CHANNELS

  #include "channels/PWMChannel.h"

  #include "controllers/ChannelController.h"

class YarrboardApp;
class ConfigManager;

class PWMController : public ChannelController<PWMChannel, YB_PWM_CHANNEL_COUNT>
{
  public:
    BusVoltageController* busVoltage = nullptr;
    RGBControllerInterface* rgb = nullptr;

    PWMController(YarrboardApp& app);

    bool setup() override;
    void loop() override;
    void generateStatsHook(JsonVariant output) override;
    void updateBrightnessHook(float brightness) override;

    float getAverageCurrent();
    float getMaxCurrent();

    static void handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleSetCommand(JsonVariantConst input, JsonVariant output);
    void handleToggleCommand(JsonVariantConst input, JsonVariant output);
    void handleConfigCommand(JsonVariantConst input, JsonVariant output);

  private:
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
    MCP3564Helper* adcCurrentHelper;
  #endif

  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
    MCP3564Helper* adcVoltageHelper;
  #endif

    static PWMController* _instance;
    void handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup);
};

#endif
#endif /* !YARR_PWM_CONTROLLER_H */