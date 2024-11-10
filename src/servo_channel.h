/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVO_CHANNEL_H
#define YARR_SERVO_CHANNEL_H

#include "config.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>
#include <ESP32Servo.h>

class ServoChannel
{
  protected:
    byte _pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;

  public:
    byte id = 0;
    char name[YB_CHANNEL_NAME_LENGTH];
    bool isEnabled = false;

    Servo servo;

    void setup();
    void write(int value);
};

extern ServoChannel servo_channels[YB_SERVO_CHANNEL_COUNT];

void servo_channels_setup();
void servo_channels_loop();
bool isValidServoChannel(byte cid);

#endif /* !YARR_SERVO_CHANNEL_H */