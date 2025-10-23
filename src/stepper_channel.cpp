/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

// #ifdef trueYB_HAS_STEPPER_CHANNELS

#include "stepper_channel.h"

byte _step_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_CHANNEL_STEP_PINS;
byte _dir_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_CHANNEL_DIR_PINS;
byte _enable_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_CHANNEL_ENABLE_PINS;

// the main star of the event
etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

void stepper_channels_setup()
{
  // intitialize our channel
  for (auto& ch : stepper_channels) {
    ch.setup();
  }
}

void stepper_channels_loop()
{
  // check if any steppers need turning off
  for (auto& ch : stepper_channels) {
    ch.autoDisable();
  }
}

void StepperChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  this->_step_pin = _step_pins[id - 1];
  this->_dir_pin = _dir_pins[id - 1];
  this->_enable_pin = _enable_pins[id - 1];

  snprintf(this->name, sizeof(this->name), "Stepper Channel %d", id);
}

bool StepperChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  return true;
}

void StepperChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);
}

void StepperChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["position"] = this->getPosition();
  config["angle"] = this->getAngle();
}

void StepperChannel::setup()
{
  _enabled = false;

  HardwareSerial& serial_stream = Serial2;
  _stepper.setup(serial_stream);

  _stepper.setMicrostepsPerStepPowerOfTwo(8);
  _stepper.setRunCurrent(50);
  _stepper.setHoldCurrent(10);
  _stepper.setStandstillMode(stepper_driver.FREEWHEELING);
  _stepper.setStallGuardThreshold(25);

  _stepper.enable();
}

void StepperDriver::printDebug(unsigned int milliDelay)
{
  Serial.println("*************************");
  Serial.println("getSettings()");
  TMC2209::Settings settings = _stepper.getSettings();
  Serial.print("settings.is_communicating = ");
  Serial.println(settings.is_communicating);
  Serial.print("settings.is_setup = ");
  Serial.println(settings.is_setup);
  Serial.print("settings.software_enabled = ");
  Serial.println(settings.software_enabled);
  Serial.print("settings.microsteps_per_step = ");
  Serial.println(settings.microsteps_per_step);
  Serial.print("settings.inverse_motor_direction_enabled = ");
  Serial.println(settings.inverse_motor_direction_enabled);
  Serial.print("settings.stealth_chop_enabled = ");
  Serial.println(settings.stealth_chop_enabled);
  Serial.print("settings.standstill_mode = ");
  switch (settings.standstill_mode) {
    case TMC2209::NORMAL:
      Serial.println("normal");
      break;
    case TMC2209::FREEWHEELING:
      Serial.println("freewheeling");
      break;
    case TMC2209::STRONG_BRAKING:
      Serial.println("strong_braking");
      break;
    case TMC2209::BRAKING:
      Serial.println("braking");
      break;
  }
  Serial.print("settings.irun_percent = ");
  Serial.println(settings.irun_percent);
  Serial.print("settings.irun_register_value = ");
  Serial.println(settings.irun_register_value);
  Serial.print("settings.ihold_percent = ");
  Serial.println(settings.ihold_percent);
  Serial.print("settings.ihold_register_value = ");
  Serial.println(settings.ihold_register_value);
  Serial.print("settings.iholddelay_percent = ");
  Serial.println(settings.iholddelay_percent);
  Serial.print("settings.iholddelay_register_value = ");
  Serial.println(settings.iholddelay_register_value);
  Serial.print("settings.automatic_current_scaling_enabled = ");
  Serial.println(settings.automatic_current_scaling_enabled);
  Serial.print("settings.automatic_gradient_adaptation_enabled = ");
  Serial.println(settings.automatic_gradient_adaptation_enabled);
  Serial.print("settings.pwm_offset = ");
  Serial.println(settings.pwm_offset);
  Serial.print("settings.pwm_gradient = ");
  Serial.println(settings.pwm_gradient);
  Serial.print("settings.cool_step_enabled = ");
  Serial.println(settings.cool_step_enabled);
  Serial.print("settings.analog_current_scaling_enabled = ");
  Serial.println(settings.analog_current_scaling_enabled);
  Serial.print("settings.internal_sense_resistors_enabled = ");
  Serial.println(settings.internal_sense_resistors_enabled);
  Serial.println("*************************");
  Serial.println();

  Serial.println("*************************");
  Serial.println("hardwareDisabled()");
  bool hardware_disabled = stepper_driver.hardwareDisabled();
  Serial.print("hardware_disabled = ");
  Serial.println(hardware_disabled);
  Serial.println("*************************");
  Serial.println();

  Serial.println("*************************");
  Serial.println("getStatus()");
  TMC2209::Status status = stepper_driver.getStatus();
  Serial.print("status.over_temperature_warning = ");
  Serial.println(status.over_temperature_warning);
  Serial.print("status.over_temperature_shutdown = ");
  Serial.println(status.over_temperature_shutdown);
  Serial.print("status.short_to_ground_a = ");
  Serial.println(status.short_to_ground_a);
  Serial.print("status.short_to_ground_b = ");
  Serial.println(status.short_to_ground_b);
  Serial.print("status.low_side_short_a = ");
  Serial.println(status.low_side_short_a);
  Serial.print("status.low_side_short_b = ");
  Serial.println(status.low_side_short_b);
  Serial.print("status.open_load_a = ");
  Serial.println(status.open_load_a);
  Serial.print("status.open_load_b = ");
  Serial.println(status.open_load_b);
  Serial.print("status.over_temperature_120c = ");
  Serial.println(status.over_temperature_120c);
  Serial.print("status.over_temperature_143c = ");
  Serial.println(status.over_temperature_143c);
  Serial.print("status.over_temperature_150c = ");
  Serial.println(status.over_temperature_150c);
  Serial.print("status.over_temperature_157c = ");
  Serial.println(status.over_temperature_157c);
  Serial.print("status.current_scaling = ");
  Serial.println(status.current_scaling);
  Serial.print("status.stealth_chop_mode = ");
  Serial.println(status.stealth_chop_mode);
  Serial.print("status.standstill = ");
  Serial.println(status.standstill);
  Serial.println("*************************");
  Serial.println();
}

void StepperChannel::write(float angle)
{
  lastUpdateMillis = millis();
  currentAngle = angle;

  _enabled = true;

  // _servo.write(currentAngle);
}

void StepperChannel::disable()
{
  _enabled = false;
  // _servo.disable();
}

void StepperChannel::autoDisable()
{
  // shut off our servos after a certain amount of time of inactivity
  // eg. a diverter valve that only moves every couple of hours will just sit there getting hot.
  unsigned int delta = millis() - this->lastUpdateMillis;
  if (this->autoDisableMillis > 0 && this->_enabled && delta >= this->autoDisableMillis)
    this->disable();
}

float StepperChannel::getAngle()
{
  return currentAngle;
}

uint32_t StepperChannel::getPosition()
{
  return currentPosition;
}

#endif