/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_RELAY_CONTROLLER_H
#define YARR_RELAY_CONTROLLER_H

#include "config.h"
#ifdef YB_HAS_RELAY_CHANNELS

  #include "channels/RelayChannel.h"
  #include <controllers/ChannelController.h>
  #include <controllers/ProtocolController.h>

class YarrboardApp;
class RelayController : public ChannelController<RelayChannel, YB_RELAY_CHANNEL_COUNT>
{
  public:
    RelayController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void handleConfigCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSetCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleToggleCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);

    // void mqttUpdateHook(MQTTController* mqtt) override;

    // blank to disable HA hooks.
    void haUpdateHook(MQTTController* mqtt) override {};
    void haGenerateDiscoveryHook(JsonVariant components, const char* uuid, MQTTController* mqtt) override {};

  private:
    const byte _pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;
};

#endif
#endif /* !YARR_RELAY_CONTROLLER_H */