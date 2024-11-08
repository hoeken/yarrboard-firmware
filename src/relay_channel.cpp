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
RelayChannel relay_channels[YB_RELAY_CHANNEL_COUNT];

void relay_channels_setup()
{
  // intitialize our channel
  for (short i = 0; i < YB_RELAY_CHANNEL_COUNT; i++) {
    relay_channels[i].id = i;
    relay_channels[i].setup();
    relay_channels[i].setupDefaultState();
  }
}

void relay_channels_loop()
{
}

bool isValidRelayChannel(byte cid)
{
  if (cid < 0 || cid >= YB_RELAY_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void RelayChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our name
  sprintf(prefIndex, "rlyName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "Relay #%d", this->id);

  // enabled or no
  sprintf(prefIndex, "rlyEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  // type
  sprintf(prefIndex, "rlyType%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->type, preferences.getString(prefIndex).c_str(), sizeof(this->type));
  else
    sprintf(this->type, "other", this->id);

  // default state
  sprintf(prefIndex, "rlyDefault%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->defaultState, preferences.getString(prefIndex).c_str(), sizeof(this->defaultState));
  else
    sprintf(this->defaultState, "OFF", this->id);
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
  this->updateOutput(true);
}

void RelayChannel::updateOutput(bool check_status)
{
  digitalWrite(_pins[id], check_status);
}

void RelayChannel::setState(const char* state)
{
  DUMP(state);

  if (!strcmp(state, "ON"))
    this->status = Status::ON;
  else if (!strcmp(state, "OFF"))
    this->status = Status::OFF;

  if (!strcmp(state, "ON"))
    this->setState(true);
  else
    this->setState(false);
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

    // change our output pin to reflect
    this->updateOutput(true);
  }
}

const char* RelayChannel::getStatus()
{
  if (this->status == Status::ON)
    return "ON";
  else
    return "OFF";
}

#endif