/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#ifdef YB_HAS_SERVO_CHANNELS

  #include "controllers/ServoController.h"
  #include <ConfigManager.h>
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>
  #include <controllers/ProtocolController.h>

ServoController::ServoController(YarrboardApp& app) : ChannelController(app, "servo")
{
}

bool ServoController::setup()
{
  _app.protocol.registerCommand(ADMIN, "config_servo", this, &ServoController::handleConfigCommand);
  _app.protocol.registerCommand(GUEST, "set_servo_channel", this, &ServoController::handleSetCommand);

  for (auto& ch : _channels) {
    ch.setup(_pins[ch.id - 1]);
  }

  return true;
}

void ServoController::loop()
{
  for (auto& ch : _channels) {
    ch.autoDisable();
  }
}

void ServoController::handleConfigCommand(JsonVariantConst input, JsonVariant output)
{
  ChannelController::handleConfigCommand(input, output);
}

void ServoController::handleSetCommand(JsonVariantConst input, JsonVariant output)
{
  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // is it enabled?
  if (!ch->isEnabled)
    return _app.protocol.generateErrorJSON(output, "Channel is not enabled.");

  if (input["angle"].is<JsonVariantConst>()) {
    float angle = input["angle"];

    if (angle >= 0 && angle <= 180)
      ch->write(angle);
    else
      return _app.protocol.generateErrorJSON(output, "'angle' must be between 0 and 180");
  } else
    return _app.protocol.generateErrorJSON(output, "'angle' parameter is required.");
}

#endif