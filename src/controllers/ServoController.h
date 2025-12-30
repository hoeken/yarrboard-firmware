/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVO_CONTROLLER_H
#define YARR_SERVO_CONTROLLER_H

#include "config.h"
#ifdef YB_HAS_SERVO_CHANNELS

  #include "channels/ServoChannel.h"
  #include <controllers/ChannelController.h>
  #include <controllers/ProtocolController.h>

class YarrboardApp;
class ServoController : public ChannelController<ServoChannel, YB_SERVO_CHANNEL_COUNT>
{
  public:
    ServoController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void handleConfigCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSetCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);

  private:
    const byte _pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;
};

#endif
#endif /* !YARR_SERVO_CONTROLLER_H */