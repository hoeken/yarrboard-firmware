/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_RELAY_CHANNELS

  #include "channels/RelayChannel.h"
  #include "controllers/RelayController.h"
  #include <YarrboardDebug.h>
  #include <controllers/MQTTController.h>

void RelayChannel::setup(byte pin)
{
  _pin = pin;
  pinMode(_pin, OUTPUT);
}

void RelayChannel::setupDefaultState()
{
  // update our pin
  this->outputState = this->defaultState;
  this->updateOutput();
}

void RelayChannel::updateOutput()
{
  if (inverted)
    digitalWrite(_pin, !outputState);
  else
    digitalWrite(_pin, outputState);
}

void RelayChannel::setState(const char* state)
{
  if (!strcmp(state, "ON"))
    this->setState(true);
  else if (!strcmp(state, "OFF"))
    this->setState(false);
}

void RelayChannel::setState(bool newState)
{
  if (newState != outputState) {
    // save our output state
    this->outputState = newState;

    // keep track of how many toggles
    this->stateChangeCount++;
  }

  // change our output pin to reflect
  this->updateOutput();
}

bool RelayChannel::getState()
{
  return this->outputState;
}

const char* RelayChannel::getStatus()
{
  if (this->outputState)
    return "ON";
  else
    return "OFF";
}

void RelayChannel::init(uint8_t id)
{
  BaseChannel::init(id);
  this->channel_type = "relay";

  snprintf(this->name, sizeof(this->name), "Relay Channel %d", id);
}

bool RelayChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  strlcpy(this->type, config["type"] | "other", sizeof(this->type));
  this->defaultState = config["defaultState"];
  this->inverted = config["inverted"];

  return true;
}

void RelayChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["type"] = this->type;
  config["defaultState"] = this->defaultState;
  config["inverted"] = this->inverted;
}

void RelayChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["state"] = this->getStatus();
  config["source"] = this->source;
}

#endif