/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_SERVO_CHANNELS

  #include "servo_channel.h"

Servo _servo = Servo();

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

bool ServoChannel::loadConfigFromJSON(JsonVariantConst config, char* error)
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

  return true;
}

void ServoChannel::setup()
{
  // init our servo
  channel = _servo.attach(_pins[id]);
  _servo.setFrequency(_pins[id], 50); // standard 50hz
  // write(90);
  disable();
}

void ServoChannel::write(float angle)
{
  currentAngle = angle;
  _servo.resume(channel);
  _servo.write(_pins[id], currentAngle);
}

void ServoChannel::disable()
{
  _servo.pause(channel);
}

float ServoChannel::getAngle()
{
  return currentAngle;
}

#endif