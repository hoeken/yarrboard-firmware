/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_RELAY_CHANNELS

  #include "relay_channel.h"

// the main star of the event
etl::array<RelayChannel, YB_RELAY_CHANNEL_COUNT> relay_channels;

void relay_channels_setup()
{
  // intitialize our channel
  for (auto& ch : relay_channels) {
    ch.setup();
    ch.setupDefaultState();
  }
}

void relay_channels_loop()
{
}

void RelayChannel::setup()
{
  // init!
  pinMode(_pins[id], OUTPUT);
  digitalWrite(_pins[id], LOW);
}

void RelayChannel::setupDefaultState()
{
  // load our default status.
  if (!strcmp(this->defaultState, "ON")) {
    this->outputState = true;
    this->status = Status::ON;
  } else {
    this->outputState = false;
    this->status = Status::OFF;
  }

  // update our pin
  this->updateOutput();
}

void RelayChannel::updateOutput()
{
  digitalWrite(_pins[id], outputState);
}

void RelayChannel::setState(const char* state)
{
  if (!strcmp(state, "ON")) {
    this->status = Status::ON;
    this->setState(true);
  } else if (!strcmp(state, "OFF")) {
    this->status = Status::OFF;
    this->setState(false);
  }
}

void RelayChannel::setState(bool newState)
{
  if (newState != outputState) {
    // save our output state
    this->outputState = newState;

    // keep track of how many toggles
    this->stateChangeCount++;

    // what is our new status?
    if (newState)
      this->status = Status::ON;
    else
      this->status = Status::OFF;
  }

  // change our output pin to reflect
  this->updateOutput();
}

const char* RelayChannel::getStatus()
{
  if (this->status == Status::ON)
    return "ON";
  else
    return "OFF";
}

bool RelayChannel::loadConfigFromJSON(JsonVariantConst config, char* error)
{
  const char* value;

  if (config["id"])
    this->id = config["id"];
  else {
    // todo: error
  }

  // enabled.  missing defaults to true
  this->isEnabled = config["enabled"] | true;

  snprintf(this->name, sizeof(this->name), "Channel %d", this->id);
  if (config["name"])
    strlcpy(this->name, config["name"], sizeof(this->name));

  strlcpy(this->type, config["type"] | "other", sizeof(this->type));
  strlcpy(this->defaultState, config["defaultState"] | "OFF", sizeof(this->defaultState));

  return true;
}

#endif