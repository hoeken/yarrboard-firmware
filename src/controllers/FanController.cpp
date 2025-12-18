/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_FANS

  #include "controllers/FanController.h"
  #include "controllers/PWMController.h"
  #include <Arduino.h>
  #include <YarrboardDebug.h>

// Lots of code borrowed from: https://github.com/KlausMu/esp32-fan-controller
// Interrupt counting every rotation of the fan
volatile int FanController::counter_rpm[YB_FAN_COUNT] = {0};

void IRAM_ATTR FanController::rpm_fan_0_low()
{
  counter_rpm[0] = counter_rpm[0] + 1;
}

void IRAM_ATTR FanController::rpm_fan_1_low()
{
  counter_rpm[1] = counter_rpm[1] + 1;
}

FanController::FanController(YarrboardApp& app) : BaseController(app, "fans")
{
}

bool FanController::setup()
{
  for (byte i = 0; i < YB_FAN_COUNT; i++) {
    pinMode(fan_mosfet_pins[i], OUTPUT);
    digitalWrite(fan_mosfet_pins[i], LOW);

    pinMode(fan_tach_pins[i], INPUT);

    counter_rpm[i] = 0;
    last_tacho_measurement[i] = 0;
    fans_last_rpm[i] = 0;

    if (i == 0)
      attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), FanController::rpm_fan_0_low, FALLING);
    if (i == 1)
      attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), FanController::rpm_fan_1_low, FALLING);
  }

  return true;
}

void FanController::loop()
{
  // get our rpm numbers
  for (byte i = 0; i < YB_FAN_COUNT; i++)
    measure_fan_rpm(i);

  // get our current averages
  if (millis() - lastFanCheckMillis >= 1000) {
    float amps_avg = pwm->getAverageCurrent();
    float amps_max = pwm->getMaxCurrent();

    // one channel on high?
    if (amps_max > YB_FAN_SINGLE_CHANNEL_THRESHOLD) {
      set_fans_state(true);
      // YBP.println("Single channel full blast");
    } else if (amps_avg > YB_FAN_AVERAGE_CHANNEL_THRESHOLD) {
      set_fans_state(true);
      // YBP.println("Average current full blast");
    } else
      set_fans_state(false);

    lastFanCheckMillis = millis();
  }
}

void FanController::measure_fan_rpm(uint8_t i)
{
  if (millis() - last_tacho_measurement[i] >= 1000) {
    // calculate rpm
    fans_last_rpm[i] = counter_rpm[i] * 30;

    // reset counter
    counter_rpm[i] = 0;

    // store milliseconds when tacho was measured the last time
    last_tacho_measurement[i] = millis();
  }
}

void FanController::set_fans_state(bool state)
{
  for (byte i = 0; i < YB_FAN_COUNT; i++) {
    digitalWrite(fan_mosfet_pins[i], state);
  }
}

void FanController::generateStatsHook(JsonVariant output)
{
  // info about each of our fans
  for (byte i = 0; i < YB_FAN_COUNT; i++)
    output["fans"][i]["rpm"] = fans_last_rpm[i];
}

#endif