/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_SERVO_CHANNELS

  #include "servo_channel.h"

// the main star of the event
ServoChannel servo_channels[YB_SERVO_CHANNEL_COUNT];

void servo_channels_setup()
{
  // intitialize our channel
  for (short i = 0; i < YB_SERVO_CHANNEL_COUNT; i++) {
    servo_channels[i].id = i;
    servo_channels[i].setup();
  }
}

void servo_channels_loop()
{
}

bool isValidServoChannel(byte cid)
{
  if (cid < 0 || cid >= YB_SERVO_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void ServoChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our name
  sprintf(prefIndex, "srvName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "Servo #%d", this->id);

  // enabled or no
  sprintf(prefIndex, "srvEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  // init our class
  servo.setTimerWidth(20);
  servo.setPeriodHertz(50); // Standard 50hz servo
  servo.attach(_pins[id], 500, 2500);
  servo.writeMicroseconds(1500);
}

#endif