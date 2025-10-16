/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_FANS

  #include "fans.h"
  #include "pwm_channel.h"
  #include <Arduino.h>

// Lots of code borrowed from: https://github.com/KlausMu/esp32-fan-controller

// use the pwm channel directly after our PWM channels
byte fan_mosfet_pins[YB_FAN_COUNT] = YB_FAN_MOSFET_PINS;
byte fan_tach_pins[YB_FAN_COUNT] = YB_FAN_TACH_PINS;

static volatile int counter_rpm[YB_FAN_COUNT];
unsigned long last_tacho_measurement[YB_FAN_COUNT];

int fans_last_rpm[YB_FAN_COUNT];

unsigned long lastFanCheckMillis = 0;

// Interrupt counting every rotation of the fan
void IRAM_ATTR rpm_fan_0_low() { counter_rpm[0] = counter_rpm[0] + 1; }
void IRAM_ATTR rpm_fan_1_low() { counter_rpm[1] = counter_rpm[1] + 1; }

void fans_setup()
{
  for (byte i = 0; i < YB_FAN_COUNT; i++) {
    pinMode(fan_mosfet_pins[i], OUTPUT);
    digitalWrite(fan_mosfet_pins[i], LOW);

    pinMode(fan_tach_pins[i], INPUT);

    counter_rpm[i] = 0;
    last_tacho_measurement[i] = 0;
    fans_last_rpm[i] = 0;

    if (i == 0)
      attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0_low, FALLING);
    if (i == 1)
      attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1_low, FALLING);
  }
}

void fans_loop()
{
  // get our rpm numbers
  for (byte i = 0; i < YB_FAN_COUNT; i++)
    measure_fan_rpm(i);

  // get our current averages
  if (millis() - lastFanCheckMillis >= 1000) {
    // calculate our amperages
    float amps_avg = 0;
    float amps_max = 0;
    byte enabled_count = 0;

    for (auto& ch : pwm_channels) {
      // only count enabled channels
      if (ch.isEnabled) {
        amps_avg += ch.amperage;
        amps_max = max(amps_max, ch.amperage);
        enabled_count++;
      }
    }
    amps_avg = amps_avg / enabled_count;

    // one channel on high?
    if (amps_max > YB_FAN_SINGLE_CHANNEL_THRESHOLD) {
      set_fans_state(true);
      // Serial.println("Single channel full blast");
    } else if (amps_avg > YB_FAN_AVERAGE_CHANNEL_THRESHOLD) {
      set_fans_state(true);
      // Serial.println("Average current full blast");
    } else
      set_fans_state(false);

    lastFanCheckMillis = millis();
  }
}

void measure_fan_rpm(byte i)
{
  if (millis() - last_tacho_measurement[i] >= 1000) {
    // detach interrupt while calculating rpm
    // detachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]));

    // calculate rpm
    fans_last_rpm[i] = counter_rpm[i] * 30;

    // reset counter
    counter_rpm[i] = 0;

    // store milliseconds when tacho was measured the last time
    last_tacho_measurement[i] = millis();

    // attach interrupt again
    // if (i == 0)
    //   attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_0_low, FALLING);
    // if (i == 1)
    //   attachInterrupt(digitalPinToInterrupt(fan_tach_pins[i]), rpm_fan_1_low, FALLING);
  }
}

void set_fans_state(bool state)
{
  for (byte i = 0; i < YB_FAN_COUNT; i++) {
    digitalWrite(fan_mosfet_pins[i], state);
  }
}

#endif