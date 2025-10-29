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
    uint32_t currentPosition = 0;
    float autoDisableMillis = 5000;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup();
    void setSpeed(float rpm);
    float getAngle();
    int32_t getPosition();
    void gotoAngle(float angle, float rpm = -1);
    void gotoPosition(int32_t position, float rpm = -1);
    bool home();
    bool homeWithSpeed(float rpm, bool debounce = true);

    bool isEndstopHit();
    void disable();

    void printDebug(unsigned int milliDelay);

  private:
    unsigned long lastUpdateMillis = 0;

    TMC2209 _tmc2209;
    byte _diag_pin;
    uint8_t _run_current = 50;
    uint8_t _hold_current = 25;
    uint8_t _stall_guard = 30;

    FastAccelStepper* _stepper = NULL;
    byte _step_pin;
    byte _dir_pin;
    byte _enable_pin;

    uint32_t _steps_per_degree = YB_STEPPER_STEPS_PER_REVOLUTION / 360;
    uint32_t _acceleration = 8000;                    // steps/s^2
    uint32_t _move_speed_hz = 2000;                   // regular moving speed
    float _home_fast_speed_rpm = 5;                   // fast homing speed
    float _home_slow_speed_rpm = 1;                   // slow homing speed
    uint32_t _backoff_steps = 10 * _steps_per_degree; // release distance
    uint32_t _debounce_ms = 5;                        // switch debounce
    uint32_t _timeout_ms = 15000;                     // homing timeout
};

extern etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup();
void stepper_channels_loop();

#endif /* !YARR_STEPPER_CHANNEL_H */