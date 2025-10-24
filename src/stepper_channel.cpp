/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_STEPPER_CHANNELS

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
}

void StepperChannel::setup()
{
  // setup our TMC2209 parameters
  #ifdef YB_STEPPER_DRIVER_TMC2209
  HardwareSerial& serial_stream = Serial2;
  Serial2.setPins(YB_STEPPER_RX_PIN, YB_STEPPER_TX_PIN);
  _tmc2209.setup(serial_stream);
  _tmc2209.setMicrostepsPerStep(YB_STEPPER_MICROSTEPS);
  _tmc2209.setStandstillMode(_tmc2209.FREEWHEELING);
  _tmc2209.setRunCurrent(_run_current);
  _tmc2209.setHoldCurrent(_hold_current);
  _tmc2209.setStallGuardThreshold(_stall_guard);
  _tmc2209.enable();
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

    _stepper->setSpeedInHz(_home_fast_speed_hz);
    _stepper->setAcceleration(_acceleration);
  }
}

void StepperChannel::printDebug(unsigned int milliDelay)
{
  #ifdef YB_STEPPER_DRIVER_TMC2209
  Serial.println("*************************");
  Serial.println("getSettings()");
  TMC2209::Settings settings = _tmc2209.getSettings();
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
  bool hardware_disabled = _tmc2209.hardwareDisabled();
  Serial.print("hardware_disabled = ");
  Serial.println(hardware_disabled);
  Serial.println("*************************");
  Serial.println();

  Serial.println("*************************");
  Serial.println("getStatus()");
  TMC2209::Status status = _tmc2209.getStatus();
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
  #endif
}

void StepperChannel::setSpeed(uint32_t speed)
{
  _stepper->setSpeedInHz(speed);
}

void StepperChannel::gotoAngle(float angle, uint32_t speed)
{
  lastUpdateMillis = millis();
  currentAngle = angle;

  int32_t position = angle * _steps_per_degree;
  gotoPosition(position, speed);
}

void StepperChannel::gotoPosition(int32_t position, uint32_t speed)
{
  // optional set speed
  if (speed)
    _stepper->setSpeedInHz(speed);

  // giddyup
  _stepper->moveTo(position);
}

void StepperChannel::disable()
{
  _stepper->disableOutputs();
}

float StepperChannel::getAngle()
{
  return this->getPosition() / _steps_per_degree;
}

int32_t StepperChannel::getPosition()
{
  return _stepper->getCurrentPosition();
}

bool StepperChannel::isEndstopHit()
{
  return digitalRead(_diag_pin) == HIGH;
}

bool StepperChannel::home()
{
  if (homeWithSpeed(_home_fast_speed_hz))
    return homeWithSpeed(_home_slow_speed_hz);

  return false;
}

bool StepperChannel::homeWithSpeed(uint32_t speed, bool debounce)
{
  // back off a tiny bit first
  _stepper->setSpeedInHz(speed);
  _stepper->move(_backoff_steps);
  while (!_stepper->isRunning())
    delay(1);

  // ensure released
  if (isEndstopHit())
    return false;

  // fast seek toward negative until endstop triggers
  _stepper->setSpeedInHz(speed);
  _stepper->runBackward();

  // look for endstop with timeout
  uint32_t t1 = millis();
  while (!isEndstopHit()) {
    if (millis() - t1 > _timeout_ms) {
      _stepper->forceStop();
      return false;
    }
    delay(1);
  }

  // double check?
  if (debounce) {
    delay(_debounce_ms);
    if (!isEndstopHit()) {
      _stepper->forceStop();
      return false;
    }
  }

  // okay, zero us.
  _stepper->forceStopAndNewPosition(0);

  return true;
}
#endif