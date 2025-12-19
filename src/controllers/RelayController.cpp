/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#ifdef YB_HAS_RELAY_CHANNELS

  #include "controllers/RelayController.h"
  #include <ConfigManager.h>
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>
  #include <controllers/ProtocolController.h>

RelayController* RelayController::_instance = nullptr;

RelayController::RelayController(YarrboardApp& app) : ChannelController(app, "relay")
{
}

bool RelayController::setup()
{
  _app.protocol.registerCommand(ADMIN, "config_relay", this, &RelayController::handleConfigCommand);
  _app.protocol.registerCommand(GUEST, "set_relay_channel", this, &RelayController::handleSetCommand);
  _app.protocol.registerCommand(GUEST, "toggle_relay_channel", this, &RelayController::handleToggleCommand);

  // intitialize our channel
  for (auto& ch : _channels) {
    ch.setup(_pins[ch.id - 1]);
    ch.setupDefaultState();
  }

  return true;
}

void RelayController::loop()
{
}

void RelayController::handleSetCommand(JsonVariantConst input, JsonVariant output)
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // is it enabled?
  if (!ch->isEnabled)
    return _app.protocol.generateErrorJSON(output, "Channel is not enabled.");

  // change state
  if (input["state"].is<String>()) {
    // source is required
    if (!input["source"].is<String>())
      return _app.protocol.generateErrorJSON(output, "'source' is a required parameter");

    // check the length
    char error[50];
    if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
      sprintf(error, "Maximum source length is %s characters.", YB_HOSTNAME_LENGTH - 1);
      return _app.protocol.generateErrorJSON(output, error);
    }

    // get our data
    strlcpy(ch->source, input["source"] | _cfg.local_hostname, sizeof(ch->source));

    // okay, set our state
    char state[10];
    strlcpy(state, input["state"] | "OFF", sizeof(state));

    // update our relay channel
    ch->setState(state);

    //  get that update out ASAP... if its our own update
    if (!strcmp(ch->source, _cfg.local_hostname))
      ch->sendFastUpdate = true;
  }
}

void RelayController::handleToggleCommand(JsonVariantConst input, JsonVariant output)
{
  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // source is required
  if (!input["source"].is<String>())
    return _app.protocol.generateErrorJSON(output, "'source' is a required parameter");

  // check the length
  char error[50];
  if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
    sprintf(error, "Maximum source length is %s characters.", YB_HOSTNAME_LENGTH - 1);
    return _app.protocol.generateErrorJSON(output, error);
  }

  // save our source
  strlcpy(ch->source, input["source"] | _cfg.local_hostname, sizeof(ch->source));

  // relays are simple on/off.
  if (!strcmp(ch->getStatus(), "ON"))
    ch->setState("OFF");
  else
    ch->setState("ON");
}

void RelayController::handleConfigCommand(JsonVariantConst input, JsonVariant output)
{
  ChannelController::handleConfigCommand(input, output);
}

void RelayController::handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  if (_instance) {
    _instance->handleHACommandCallback(topic, payload, retain, qos, dup);
  }
}

void RelayController::handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  for (auto& ch : _channels) {
    ch.haHandleCommand(topic, payload);
  }
}

#endif