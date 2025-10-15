/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVO_CHANNEL_H
#define YARR_SERVO_CHANNEL_H

#include "base_channel.h"
#include "config.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>
#include <Servo.h>

class ServoChannel : public BaseChannel
{
  protected:
    byte _pins[YB_SERVO_CHANNEL_COUNT] = YB_SERVO_CHANNEL_PINS;

  public:
    float currentAngle = 0.0;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup();
    void write(float angle);
    float getAngle();
    void disable();

  private:
    byte channel = 0;
};

extern etl::array<ServoChannel, YB_SERVO_CHANNEL_COUNT> servo_channels;

void servo_channels_setup();
void servo_channels_loop();

#endif /* !YARR_SERVO_CHANNEL_H */