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
byte _pins[YB_RELAY_CHANNEL_COUNT] = YB_RELAY_CHANNEL_PINS;

  #ifdef YB_RELAY_DRIVER_TCA9554
TCA9554 TCA(YB_RELAY_DRIVER_TCA9554_ADDRESS);
  #endif

void relay_channels_setup()
{
  #ifdef YB_RELAY_DRIVER_TCA9554
  Wire.begin();
  Wire.setPins(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
  Wire.setClock(YB_I2C_SPEED);
  TCA.begin();

  // todo: connect and config driver
  #endif
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
  _pin = _pins[id - 1];

  #ifdef YB_RELAY_DRIVER_TCA9554
  TCA.pinMode1(_pin, OUTPUT);
  TCA.write1(_pin, LOW);
  #else
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
  #endif
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
  #ifdef YB_RELAY_DRIVER_TCA9554
  TCA.write1(_pin, outputState);
  #else
  digitalWrite(_pin, outputState);
  #endif
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

void RelayChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  snprintf(this->name, sizeof(this->name), "Relay Channel %d", id);
}

bool RelayChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  strlcpy(this->type, config["type"] | "other", sizeof(this->type));
  strlcpy(this->defaultState, config["defaultState"] | "OFF", sizeof(this->defaultState));

  return true;
}

void RelayChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["type"] = this->type;
  config["enabled"] = this->isEnabled;
  config["defaultState"] = this->defaultState;
}

void RelayChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["state"] = this->getStatus();
  config["source"] = this->source;
}

#endif