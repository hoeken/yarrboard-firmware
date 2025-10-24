/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_STEPPER_CHANNEL_H
#define YARR_STEPPER_CHANNEL_H

#include "FastAccelStepper.h"
#include "TMC2209.h"
#include "base_channel.h"
#include "config.h"
#include "driver/ledc.h"
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
    void setSpeed(uint32_t speed);
    float getAngle();
    int32_t getPosition();
    void gotoAngle(float angle, uint32_t speed = 0);
    void gotoPosition(int32_t position, uint32_t speed = 0);
    void disable();

    bool home();
    bool isEndstopHit();

    void printDebug(unsigned int milliDelay);
    //    void autoDisable();

  private:
    unsigned long lastUpdateMillis = 0;
    FastAccelStepper* _stepper = NULL;
    TMC2209 _stepperConfig;
    byte _step_pin;
    byte _dir_pin;
    byte _enable_pin;
    byte _diag_pin;
    uint32_t _steps_per_degree;

    uint32_t _acceleration = 8000;       // steps/s^2
    uint32_t _home_fast_speed_hz = 4000; // fast seek speed
    uint32_t _home_slow_speed_hz = 800;  // slow seek speed
    uint32_t _backoff_steps = 800;       // release distance
    uint32_t _homing_travel = 200000;    // "very far" move to guarantee hit
    uint32_t _debounce_ms = 5;           // switch debounce
    uint32_t _timeout_ms = 15000;        // homing timeout
};

extern etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup();
void stepper_channels_loop();

#endif /* !YARR_STEPPER_CHANNEL_H */