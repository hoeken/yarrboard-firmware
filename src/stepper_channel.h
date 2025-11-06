/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_STEPPER_CHANNEL_H
#define YARR_STEPPER_CHANNEL_H

#include "FastAccelStepper.h"
#include "config.h"
#ifdef YB_STEPPER_DRIVER_TMC2209
  #include "TMC2209.h"
#endif
#include "base_channel.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>

class StepperChannel : public BaseChannel
{
  protected:
  public:
    float currentAngle = 0.0;
    float currentSpeed = 0.0;
    uint32_t currentPosition = 0;
    float autoDisableMillis = 5000;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup();
    void setSpeed(float rpm);
    float getSpeed();
    float getAngle();
    int32_t getPosition();
    void gotoAngle(float angle, float rpm = -1);
    void gotoPosition(int32_t position, float rpm = -1);
    bool home();
    bool homeWithSpeed(float rpm);

    bool isEndstopHit();
    void disable();

    void printDebug(unsigned int milliDelay);

  private:
    unsigned long lastUpdateMillis = 0;

    HardwareSerial& _serial = Serial2;
    TMC2209 _tmc2209;
    byte _diag_pin;
    uint8_t _run_current = 50;
    uint8_t _hold_current = 20;
    uint8_t _stall_guard = 25;

    FastAccelStepper* _stepper = NULL;
    byte _step_pin;
    byte _dir_pin;
    byte _enable_pin;

    float _steps_per_degree = (float)YB_STEPPER_STEPS_PER_REVOLUTION / 360.0;
    uint32_t _acceleration = _steps_per_degree * 720; // steps/s^2
    float _default_speed_rpm = 100.0;                 // typical homing speed
    float _home_fast_speed_rpm = 50.0;                // fast homing speed
    float _home_slow_speed_rpm = 25.0;                // slow homing speed
    uint32_t _backoff_steps = 45 * _steps_per_degree; // release distance
    uint32_t _timeout_ms = 15000;                     // homing timeout
};

extern etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup();
void stepper_channels_loop();

#endif /* !YARR_STEPPER_CHANNEL_H */