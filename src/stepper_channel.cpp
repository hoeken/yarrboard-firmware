/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_STEPPER_CHANNELS

  #include "debug.h"
  #include "stepper_channel.h"

byte _step_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_STEP_PINS;
byte _dir_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_DIR_PINS;
byte _enable_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_ENABLE_PINS;

  #ifdef YB_STEPPER_DIAG_PINS
byte _diag_pins[YB_STEPPER_CHANNEL_COUNT] = YB_STEPPER_DIAG_PINS;
  #endif

// the main star of the event
etl::array<StepperChannel, YB_STEPPER_CHANNEL_COUNT> stepper_channels;

FastAccelStepperEngine engine = FastAccelStepperEngine();

void stepper_channels_setup()
{
  engine.init();

  // intitialize our channel
  for (auto& ch : stepper_channels) {
    ch.setup();
  }
}

void stepper_channels_loop()
{
  // // check if any steppers need turning off
  // for (auto& ch : stepper_channels) {
  //   ch.autoDisable();
  // }
}

void StepperChannel::init(uint8_t id)
{
  BaseChannel::init(id);

  this->_step_pin = _step_pins[id - 1];
  pinMode(_step_pin, OUTPUT);

  this->_dir_pin = _dir_pins[id - 1];
  pinMode(_dir_pin, OUTPUT);

  this->_enable_pin = _enable_pins[id - 1];
  pinMode(_enable_pin, OUTPUT);

  #ifdef YB_STEPPER_DIAG_PINS
  this->_diag_pin = _diag_pins[id - 1];
  pinMode(_diag_pin, INPUT);
  #endif

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
  config["speed"] = this->getSpeed();
}

void StepperChannel::setup()
{
  // setup our TMC2209 parameters
  #ifdef YB_STEPPER_DRIVER_TMC2209
  // setup our serial port
  _serial = Serial2;
  _tmc2209.setup(_serial, 115200, TMC2209::SERIAL_ADDRESS_0, YB_STEPPER_RX_PIN, YB_STEPPER_TX_PIN);

  // setup our chip
  _tmc2209.setMicrostepsPerStep(YB_STEPPER_MICROSTEPS);
  _tmc2209.setStandstillMode(_tmc2209.FREEWHEELING);
  _tmc2209.setRunCurrent(_run_current);
  _tmc2209.setHoldCurrent(_hold_current);
  _tmc2209.setStallGuardThreshold(_stall_guard);
  _tmc2209.enableAutomaticCurrentScaling();
  _tmc2209.enableAutomaticGradientAdaptation();
  _tmc2209.enableStealthChop();
  _tmc2209.enable();

    // printDebug(0);

  #endif

  // setup our actual stepper controller
  _stepper = engine.stepperConnectToPin(_step_pin);
  if (_stepper) {
    _stepper->setDirectionPin(_dir_pin);
    _stepper->setEnablePin(_enable_pin);
    _stepper->setAutoEnable(true);

    // If auto enable/disable need delays, just add (one or both):
    _stepper->setDelayToEnable(50);
    _stepper->setDelayToDisable(1000);

    setSpeed(_home_fast_speed_rpm);
    _stepper->setAcceleration(_acceleration);
  }
}

void StepperChannel::printDebug(unsigned int milliDelay)
{
  #ifdef YB_STEPPER_DRIVER_TMC2209
  YBP.println("*************************");
  YBP.println("getSettings()");
  TMC2209::Settings settings = _tmc2209.getSettings();
  YBP.print("settings.is_communicating = ");
  YBP.println(settings.is_communicating);
  YBP.print("settings.is_setup = ");
  YBP.println(settings.is_setup);
  YBP.print("settings.software_enabled = ");
  YBP.println(settings.software_enabled);
  YBP.print("settings.microsteps_per_step = ");
  YBP.println(settings.microsteps_per_step);
  YBP.print("settings.inverse_motor_direction_enabled = ");
  YBP.println(settings.inverse_motor_direction_enabled);
  YBP.print("settings.stealth_chop_enabled = ");
  YBP.println(settings.stealth_chop_enabled);
  YBP.print("settings.standstill_mode = ");
  switch (settings.standstill_mode) {
    case TMC2209::NORMAL:
      YBP.println("normal");
      break;
    case TMC2209::FREEWHEELING:
      YBP.println("freewheeling");
      break;
    case TMC2209::STRONG_BRAKING:
      YBP.println("strong_braking");
      break;
    case TMC2209::BRAKING:
      YBP.println("braking");
      break;
  }
  YBP.print("settings.irun_percent = ");
  YBP.println(settings.irun_percent);
  YBP.print("settings.irun_register_value = ");
  YBP.println(settings.irun_register_value);
  YBP.print("settings.ihold_percent = ");
  YBP.println(settings.ihold_percent);
  YBP.print("settings.ihold_register_value = ");
  YBP.println(settings.ihold_register_value);
  YBP.print("settings.iholddelay_percent = ");
  YBP.println(settings.iholddelay_percent);
  YBP.print("settings.iholddelay_register_value = ");
  YBP.println(settings.iholddelay_register_value);
  YBP.print("settings.automatic_current_scaling_enabled = ");
  YBP.println(settings.automatic_current_scaling_enabled);
  YBP.print("settings.automatic_gradient_adaptation_enabled = ");
  YBP.println(settings.automatic_gradient_adaptation_enabled);
  YBP.print("settings.pwm_offset = ");
  YBP.println(settings.pwm_offset);
  YBP.print("settings.pwm_gradient = ");
  YBP.println(settings.pwm_gradient);
  YBP.print("settings.cool_step_enabled = ");
  YBP.println(settings.cool_step_enabled);
  YBP.print("settings.analog_current_scaling_enabled = ");
  YBP.println(settings.analog_current_scaling_enabled);
  YBP.print("settings.internal_sense_resistors_enabled = ");
  YBP.println(settings.internal_sense_resistors_enabled);
  YBP.println("*************************");
  YBP.println();

  YBP.println("*************************");
  YBP.println("hardwareDisabled()");
  bool hardware_disabled = _tmc2209.hardwareDisabled();
  YBP.print("hardware_disabled = ");
  YBP.println(hardware_disabled);
  YBP.println("*************************");
  YBP.println();

  YBP.println("*************************");
  YBP.println("getStatus()");
  TMC2209::Status status = _tmc2209.getStatus();
  YBP.print("status.over_temperature_warning = ");
  YBP.println(status.over_temperature_warning);
  YBP.print("status.over_temperature_shutdown = ");
  YBP.println(status.over_temperature_shutdown);
  YBP.print("status.short_to_ground_a = ");
  YBP.println(status.short_to_ground_a);
  YBP.print("status.short_to_ground_b = ");
  YBP.println(status.short_to_ground_b);
  YBP.print("status.low_side_short_a = ");
  YBP.println(status.low_side_short_a);
  YBP.print("status.low_side_short_b = ");
  YBP.println(status.low_side_short_b);
  YBP.print("status.open_load_a = ");
  YBP.println(status.open_load_a);
  YBP.print("status.open_load_b = ");
  YBP.println(status.open_load_b);
  YBP.print("status.over_temperature_120c = ");
  YBP.println(status.over_temperature_120c);
  YBP.print("status.over_temperature_143c = ");
  YBP.println(status.over_temperature_143c);
  YBP.print("status.over_temperature_150c = ");
  YBP.println(status.over_temperature_150c);
  YBP.print("status.over_temperature_157c = ");
  YBP.println(status.over_temperature_157c);
  YBP.print("status.current_scaling = ");
  YBP.println(status.current_scaling);
  YBP.print("status.stealth_chop_mode = ");
  YBP.println(status.stealth_chop_mode);
  YBP.print("status.standstill = ");
  YBP.println(status.standstill);
  YBP.println("*************************");
  YBP.println();

  YBP.println("*************************");
  YBP.printf("Step Pin: %d\n", _step_pin);
  YBP.printf("Dir Pin: %d\n", _dir_pin);
  YBP.printf("Enable Pin: %d\n", _enable_pin);
    #ifdef YB_STEPPER_DIAG_PINS
  YBP.printf("Diag Pin: %d\n", _diag_pin);
    #endif
  YBP.printf("Steps per degree: %.2f\n", _steps_per_degree);
  YBP.printf("Acceleratation: %d steps/s^2\n", _acceleration);
  YBP.printf("Default Speed: %.1fRPM\n", _default_speed_rpm);
  YBP.printf("Fast Homing Speed: %.1fRPM\n", _home_fast_speed_rpm);
  YBP.printf("Slow Homing Speed: %.1fRPM\n", _home_slow_speed_rpm);
  YBP.println("*************************");
  YBP.println();
  #endif
}

void StepperChannel::setSpeed(float rpm)
{
  uint32_t hz = (rpm * (float)YB_STEPPER_STEPS_PER_REVOLUTION) / 60.0;
  _stepper->setSpeedInHz(hz);

  currentSpeed = rpm;

  // YBP.printf("CH%d | Set RPM: %.1f | Hz: %d\n", this->id, rpm, hz);
}

float StepperChannel::getSpeed()
{
  return currentSpeed;
}

void StepperChannel::gotoAngle(float angle, float rpm)
{
  lastUpdateMillis = millis();
  currentAngle = angle;

  if (rpm <= 0)
    rpm = currentSpeed;

  int32_t position = angle * _steps_per_degree;

  // YBP.printf("CH%d | Go To Angle: %.1f | Position: %d | RPM: %.1f\n", this->id, angle, position, rpm);
  gotoPosition(position, rpm);
}

void StepperChannel::gotoPosition(int32_t position, float rpm)
{
  // optional set speed
  if (rpm > 0)
    setSpeed(rpm);

  // YBP.printf("CH%d | Go To Position: %d | RPM: %.1f\n", this->id, position, rpm);

  // giddyup
  _stepper->moveTo(position);
}

void StepperChannel::disable()
{
  _stepper->disableOutputs();
}

float StepperChannel::getAngle()
{
  return currentAngle;
  // return this->getPosition() / _steps_per_degree;
}

int32_t StepperChannel::getPosition()
{
  return _stepper->getCurrentPosition();
}

bool StepperChannel::isEndstopHit()
{
  uint16_t stall_guard_result = _tmc2209.getStallGuardResult();
  // DUMP(stall_guard_result);

  if (stall_guard_result < _stall_guard * 2)
    return true;
  else
    return false;
}

bool StepperChannel::home()
{
  if (homeWithSpeed(_home_fast_speed_rpm))
    return homeWithSpeed(_home_slow_speed_rpm);

  return false;
}

bool StepperChannel::homeWithSpeed(float rpm)
{
  // back off a tiny bit first
  setSpeed(rpm);
  _stepper->move(_backoff_steps);
  while (_stepper->isRunning())
    delay(1);

  // seek toward negative until endstop triggers
  _stepper->runBackward();
  delay(100); // give it time to get started.

  // look for endstop with timeout
  uint32_t t1 = millis();
  while (!isEndstopHit()) {
    if (millis() - t1 > _timeout_ms) {
      _stepper->forceStop();
      YBP.println("Stepper homing timeout.");
      return false;
    }
    delay(1);
  }

  // okay, zero us.
  _stepper->forceStopAndNewPosition(0);

  return true;
}
#endif