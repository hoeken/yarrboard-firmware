/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_STEPPER_CONTROLLER_H
#define YARR_STEPPER_CONTROLLER_H

#include "config.h"

#ifdef YB_HAS_STEPPER_CHANNELS

  #include "channels/StepperChannel.h"
  #include <controllers/ChannelController.h>
  #include <controllers/ProtocolController.h>

class YarrboardApp;
class StepperController : public ChannelController<StepperChannel, YB_STEPPER_CHANNEL_COUNT>
{
  public:
    StepperController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void handleConfigCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSetCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context);

  private:
    const byte _step_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_STEP_PINS;
    const byte _dir_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_DIR_PINS;
    const byte _enable_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_ENABLE_PINS;

  #ifdef YB_STEPPER_DIAG_PINS
    const byte _diag_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_DIAG_PINS;
  #endif

    FastAccelStepperEngine engine;
};

#endif
#endif /* !YARR_STEPPER_CONTROLLER_H */