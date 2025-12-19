/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#ifdef YB_HAS_STEPPER_CHANNELS

  #include "controllers/StepperController.h"
  #include <ConfigManager.h>
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>
  #include <controllers/ProtocolController.h>

StepperController::StepperController(YarrboardApp& app) : ChannelController(app, "stepper")
{
}

bool StepperController::setup()
{
  _app.protocol.registerCommand(ADMIN, "config_stepper", this, &StepperController::handleConfigCommand);
  _app.protocol.registerCommand(GUEST, "set_stepper_channel", this, &StepperController::handleSetCommand);

  engine.init();

  // intitialize our channel
  for (auto& ch : _channels) {
    ch.setup(&engine, _step_pins[ch.id - 1], _dir_pins[ch.id - 1], _enable_pins[ch.id - 1], _diag_pins[ch.id - 1]);
  }

  return true;
}

void StepperController::loop()
{
}

void StepperController::handleConfigCommand(JsonVariantConst input, JsonVariant output)
{
  ChannelController::handleConfigCommand(input, output);
}

void StepperController::handleSetCommand(JsonVariantConst input, JsonVariant output)
{
  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // is it enabled?
  if (!ch->isEnabled)
    return _app.protocol.generateErrorJSON(output, "Channel is not enabled.");

  // update our rpm
  if (input["rpm"]) {
    float rpm = input["rpm"];
    ch->setSpeed(rpm);
  }

  // start a homing operation
  if (input["home"]) {
    ch->home();
    return;
  }

  // move to an angle
  if (input["angle"].is<JsonVariantConst>()) {
    float angle = input["angle"];
    ch->gotoAngle(angle);
  }
}

#endif