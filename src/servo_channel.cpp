/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_SERVO_CHANNELS

  #include "servo_channel.h"

byte _pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;

// the main star of the event
etl::array<ServoChannel, YB_SERVO_CHANNEL_COUNT> servo_channels;

void servo_channels_setup()
{
  // intitialize our channel
  for (auto& ch : servo_channels) {
    ch.setup();
  }
}

void servo_channels_loop()
{
}

void ServoChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  this->_pin = _pins[id - 1];

  snprintf(this->name, sizeof(this->name), "Servo Channel %d", id);
}

bool ServoChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  return true;
}

void ServoChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);
}

void ServoChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["angle"] = this->getAngle();
}

void ServoChannel::setup()
{
  TRACE();
  // if (this->_pin == 39)
  //   ledcDetach(this->_pin);
  TRACE();
  _servo.attach(this->_pin);
  TRACE();
}

void ServoChannel::write(float angle)
{
  currentAngle = angle;
  DUMP(this->_pin);
  DUMP(currentAngle);
  _servo.write(currentAngle);
}

void ServoChannel::disable()
{
  _servo.disable();
}

float ServoChannel::getAngle()
{
  return currentAngle;
}

#endif