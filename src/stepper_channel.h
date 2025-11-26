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
    uint32_t autoDisableMillis = 10000;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup();
    void setSpeed(float rpm);
    void setStepsPerDegree(float steps);
    float getSpeed();
    float getAngle();
    int32_t getPosition();
    void gotoAngle(float angle, float rpm = -1);
    void gotoPosition(int32_t position, float rpm = -1);
    bool home();
    bool home(float rpm);
    bool homeWithSpeed(float rpm);
    void waitUntilStopped();

    bool isEndstopHit();
    void disable();

    void printDebug();

  private:
    unsigned long lastUpdateMillis = 0;

    TMC2209 _tmc2209;
    byte _diag_pin;
    uint8_t _run_current = 67;
    uint8_t _hold_current = 20;
    uint8_t _stall_guard = 90;

    volatile bool _endstopTriggered = false;

    FastAccelStepper* _stepper = NULL;
    byte _step_pin;
    byte _dir_pin;
    byte _enable_pin;

    float _steps_per_degree = 200 * YB_STEPPER_MICROSTEPS;
    uint32_t _acceleration = _steps_per_degree * 720; // steps/s^2
    uint32_t _backoff_steps = 15 * _steps_per_degree; // release distance
    float _default_speed_rpm = 10.0;                  // typical homing speed
    float _home_speed_rpm = 35.0;                     // homing speed
    uint32_t _timeout_ms = 15000;                     // homing timeout

    static void ARDUINO_ISR_ATTR stallGuardISR(void* arg)
    {
      auto* self = static_cast<StepperChannel*>(arg);
      self->_endstopTriggered = true;
    }
};

extern etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup();
void stepper_channels_loop();

#endif /* !YARR_STEPPER_CHANNEL_H */