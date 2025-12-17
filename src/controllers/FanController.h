/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"
#include "controllers/BaseController.h"
#include <Arduino.h>

#ifndef YARR_FANS_H
  #define YARR_FANS_H

class YarrboardApp;
class ConfigManager;
class PWMController;

class FanController : public BaseController
{
  public:
    FanController(YarrboardApp& app);

    bool setup() override;
    void loop() override;

    void measure_fan_rpm(uint8_t i);
    void set_fans_state(bool state);

    void generateStatsHook(JsonVariant output) override;

    PWMController* pwm = nullptr;

  private:
    // use the pwm channel directly after our PWM channels
    byte fan_mosfet_pins[YB_FAN_COUNT] = YB_FAN_MOSFET_PINS;
    byte fan_tach_pins[YB_FAN_COUNT] = YB_FAN_TACH_PINS;

    static volatile int counter_rpm[YB_FAN_COUNT];
    unsigned long last_tacho_measurement[YB_FAN_COUNT];

    int fans_last_rpm[YB_FAN_COUNT];

    unsigned long lastFanCheckMillis = 0;

    static void IRAM_ATTR rpm_fan_0_low();
    static void IRAM_ATTR rpm_fan_1_low();
};

#endif /* !YARR_FANS_H */