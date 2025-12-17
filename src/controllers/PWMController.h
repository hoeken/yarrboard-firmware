/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PWM_CONTROLLER_H
#define YARR_PWM_CONTROLLER_H

#include "channels/PWMChannel.h"
#include "controllers/ChannelController.h"

class YarrboardApp;
class ConfigManager;

class PWMController : public ChannelController<PWMChannel, YB_PWM_CHANNEL_COUNT>
{
  public:
    BusVoltageController* busVoltage = nullptr;
    RGBControllerInterface* rgb = nullptr;
    MQTTController* mqtt = nullptr;

    PWMController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    static void handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);

  private:
#ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
    MCP3564* _adcCurrentMCP3564;
    MCP3564Helper* adcCurrentHelper;
#endif

#ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
    MCP3564* _adcVoltageMCP3564;
    MCP3564Helper* adcVoltageHelper;
#endif

    static PWMController* _instance;
    void handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup);
};

#endif /* !YARR_PWM_CONTROLLER_H */