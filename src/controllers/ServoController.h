/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVO_CONTROLLER_H
#define YARR_SERVO_CONTROLLER_H

#include "channels/ServoChannel.h"
#include "controllers/ChannelController.h"

#ifdef YB_HAS_SERVO_CHANNELS

class YarrboardApp;
class ServoController : public ChannelController<ServoChannel, YB_SERVO_CHANNEL_COUNT>
{
  public:
    ServoController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void handleConfigCommand(JsonVariantConst input, JsonVariant output);
    void handleSetCommand(JsonVariantConst input, JsonVariant output);

  private:
    const byte _pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;
};

#endif
#endif /* !YARR_SERVO_CONTROLLER_H */