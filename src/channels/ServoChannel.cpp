/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_SERVO_CHANNELS

  #include "channels/ServoChannel.h"

void ServoChannel::init(uint8_t id)
{
  BaseChannel::init(id);
  this->channel_type = "servo";

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

void ServoChannel::setup(byte pin)
{
  _pin = pin;
  _servo.attach(this->_pin);
  _enabled = false;
}

void ServoChannel::write(float angle)
{
  lastUpdateMillis = millis();
  currentAngle = angle;

  _enabled = true;
  _servo.write(currentAngle);
}

void ServoChannel::disable()
{
  _enabled = false;
  _servo.disable();
}

void ServoChannel::autoDisable()
{
  // shut off our servos after a certain amount of time of inactivity
  // eg. a diverter valve that only moves every couple of hours will just sit there getting hot.
  unsigned int delta = millis() - this->lastUpdateMillis;
  if (this->autoDisableMillis > 0 && this->_enabled && delta >= this->autoDisableMillis)
    this->disable();
}

float ServoChannel::getAngle()
{
  return currentAngle;
}

#endif