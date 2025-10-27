/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS

  #include "digital_input_channel.h"

// the main star of the event
etl::array<DigitalInputChannel, YB_DIGITAL_INPUT_CHANNEL_COUNT> digital_input_channels;
byte _di_pins[YB_DIGITAL_INPUT_CHANNEL_COUNT] = YB_DIGITAL_INPUT_CHANNEL_PINS;

unsigned long lastInputCheckMillis = 0;

void input_channels_setup()
{
  for (auto& ch : digital_input_channels) {
    ch.setup();
  }
}

void input_channels_loop()
{
  bool doSendFastUpdate = false;

  if (millis() - lastInputCheckMillis >= YB_INPUT_DEBOUNCE_RATE_MS) {

    // maintenance on our channels.
    for (auto& ch : digital_input_channels) {
      ch.update();

      if (ch.sendFastUpdate)
        doSendFastUpdate = true;
    }

    // let the client know immediately.
    if (doSendFastUpdate)
      sendFastUpdate();

    lastInputCheckMillis = millis();
  }
}

void DigitalInputChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  this->pin = _di_pins[id - 1]; // pins are zero indexed, we are 1 indexed

  snprintf(this->name, sizeof(this->name), "Digital Input Channel %d", id);
}

void DigitalInputChannel::setup()
{
  // setup our pin
  pinMode(this->pin, INPUT);
}

void DigitalInputChannel::update()
{
  // load it up!
  this->raw = digitalRead(this->pin);
  bool nextState = this->state;

  // okay, did we actually change?
  if (this->raw != this->originalRaw) {
    // and debounce.  needs steadystate
    if (this->raw == this->lastRaw) {
      // direct is easy
      if (this->mode == DIRECT)
        nextState = this->raw;
      // inverting is easy too
      else if (this->mode == INVERTING)
        nextState = !this->raw;
      // just toggle it on rising
      else if (this->raw && this->mode == TOGGLE_RISING)
        nextState = !this->state;
      // just toggle it on falling
      else if (!this->raw && this->mode == TOGGLE_FALLING)
        nextState = !this->state;

      // our new state
      this->originalRaw = this->raw;
    }
  }

  // save our raw statuses
  this->lastRaw = this->raw;

  // update our state variable
  if (nextState != this->state) {
    this->setState(nextState);
  }
}

bool DigitalInputChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  this->mode = this->getMode(config["switch_mode"].as<const char*>());
  this->defaultState = config["default_state"] | false;
  this->setState(defaultState);

  return true;
}

void DigitalInputChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["switch_mode"] = this->getModeName(this->mode);
  config["default_state"] = this->defaultState;
}

void DigitalInputChannel::generateUpdate(JsonVariant update)
{
  update["state"] = this->getState();
}

void DigitalInputChannel::setState(bool state)
{
  if (this->state != state) {
    this->state = state;
    this->stateChangeCount++;
    this->sendFastUpdate = true;
  }
}

bool DigitalInputChannel::getState()
{
  return this->state;
}

const char* DigitalInputChannel::getModeName(SwitchMode mode)
{
  if (mode == DIRECT)
    return "DIRECT";
  else if (mode == INVERTING)
    return "INVERTING";
  else if (mode == TOGGLE_RISING)
    return "TOGGLE_RISING";
  else if (mode == TOGGLE_FALLING)
    return "TOGGLE_FALLING";
  else if (mode == TOGGLE_FADE)
    return "TOGGLE_FADE";
  else
    return "";
}

DigitalInputChannel::SwitchMode DigitalInputChannel::getMode(const char* mode)
{
  if (!strcmp("DIRECT", mode))
    return DIRECT;
  else if (!strcmp("INVERTING", mode))
    return INVERTING;
  else if (!strcmp("TOGGLE_RISING", mode))
    return TOGGLE_RISING;
  else if (!strcmp("TOGGLE_FALLING", mode))
    return TOGGLE_FALLING;
  else if (!strcmp("TOGGLE_FADE", mode))
    return TOGGLE_FADE;
  else
    return DIRECT;
}

#endif