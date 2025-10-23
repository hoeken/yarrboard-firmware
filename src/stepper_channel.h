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
    void setAngle(float angle, uint32_t speed);
    void setPosition(int32_t position, uint32_t speed);
    float getAngle();
    int32_t getPosition();
    void disable();

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
};

extern etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup();
void stepper_channels_loop();

#endif /* !YARR_STEPPER_CHANNEL_H */