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
#include "driver/ledc.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>

class Servo
{
  public:
    Servo() {}

    void attach(uint8_t pin)
    {
      _pin = pin;
      if (!ledcAttach(_pin, _freq, _resolution))
        Serial.printf("failed to attach %d at %dhz / %d resolution\n", _pin, _freq, _resolution);
    }

    void write(float angle)
    {
      if (angle < 0)
        angle = 0;
      if (angle > 180)
        angle = 180;
      _lastAngle = angle;

      // Map 0–180° → 500–2500 µs
      uint32_t pulse_us = 500 + (angle / 180.0f) * 2000.0f;

      // Convert pulse width to duty cycle
      uint32_t duty = (pulse_us * ((1 << _resolution) - 1) * _freq) / 1000000UL;

      if (!ledcWrite(_pin, duty))
        Serial.printf("Failed to write pin %d at %d duty (%f angle)\n", _pin, duty, angle);
    }

    void disable()
    {
      if (!ledcWrite(_pin, 0))
        Serial.printf("Failed to disable pin %d\n");
    }

    void detach()
    {
      if (!ledcDetach(_pin))
        Serial.printf("Failed to detach pin %d\n");
    }

  private:
    uint8_t _pin = 255;
    uint8_t _resolution = 10;
    uint32_t _freq = 50;
    float _lastAngle = 90;
};

class ServoChannel : public BaseChannel
{
  protected:
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
    Servo _servo;
    byte _pin;
};

extern etl::array<ServoChannel, YB_SERVO_CHANNEL_COUNT> servo_channels;

void servo_channels_setup();
void servo_channels_loop();

#endif /* !YARR_SERVO_CHANNEL_H */