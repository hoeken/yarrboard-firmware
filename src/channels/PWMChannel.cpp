/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_PWM_CHANNELS

  #include "channels/PWMChannel.h"
  #include "controllers/PWMController.h"
  #include "soc/gpio_struct.h" // Defines the GPIO struct and the global 'GPIO' variable
  #include <Arduino.h>
  #include <ConfigManager.h>
  #include <YarrboardDebug.h>

  #define YB_INA226_FACTOR 10000.0

void PWMChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our duty cycle
  if (this->isDimmable) {
    sprintf(prefIndex, "pwmDuty%d", this->id);
    if (_cfg->preferences.isKey(prefIndex))
      this->dutyCycle = _cfg->preferences.getFloat(prefIndex);
    else
      this->dutyCycle = 1.0;
  } else
    this->dutyCycle = 1.0;
  this->lastDutyCycle = this->dutyCycle;

  // soft fuse trip count
  sprintf(prefIndex, "pwmTripCount%d", this->id);
  if (_cfg->preferences.isKey(prefIndex))
    this->softFuseTripCount = _cfg->preferences.getUInt(prefIndex);
  else
    this->softFuseTripCount = 0;

  // overheat trip count
  sprintf(prefIndex, "pwmOhtCount%d", this->id);
  if (_cfg->preferences.isKey(prefIndex))
    this->overheatCount = _cfg->preferences.getUInt(prefIndex);
  else
    this->overheatCount = 0;

  #ifdef YB_PWM_CHANNEL_HAS_INA226
  this->setupINA226();
  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
  lm75 = new TI_LM75B(_lm75_addresses[this->id - 1]);
  #endif
}

  #ifdef YB_PWM_CHANNEL_HAS_INA226
void PWMChannel::setupINA226()
{
    #ifdef YB_PWM_CHANNEL_INA226_ALERT
  // YBP.printf("INA226 CH %d Setup\n", this->id);

  ina226 = new INA226(_ina226_addresses[this->id - 1]);
  if (!ina226->begin())
    YBP.printf("INA226 CH%d could not connect.", this->id);
  // else
  //   YBP.printf("INA226 %d OK");

  // YBP.print("MAN:\t");
  // YBP.println(ina226->getManufacturerID(), HEX);
  // YBP.print("DIE:\t");
  // YBP.println(ina226->getDieID(), HEX);

  ina226->setBusVoltageConversionTime(INA226_140_us);
  ina226->setShuntVoltageConversionTime(INA226_140_us);
  ina226->setAverage(INA226_128_SAMPLES);

  static const uint32_t ina226_timing_us[] = {140, 204, 332, 588, 1100, 2100, 4200, 8300};
  static const uint16_t ina226_samples[] = {1, 4, 16, 64, 128, 256, 512, 1024};

  uint32_t busUs = ina226_timing_us[ina226->getBusVoltageConversionTime()];
  uint32_t shuntUs = ina226_timing_us[ina226->getShuntVoltageConversionTime()];
  uint16_t samples = ina226_samples[ina226->getAverage()];

  voltageUpdateInterval = (busUs * samples) / 1000 + 1;
  amperageUpdateInterval = (shuntUs * samples) / 1000 + 1;

  if (this->id == 1) {
    YBP.printf("Bus Voltage Conversion Time: %d us\n", busUs);
    YBP.printf("Shunt Voltage Conversion Time: %d us\n", shuntUs);
    YBP.printf("Averages: %d samples\n", samples);
    YBP.printf("Bus Voltage Update Interval: %dms\n", voltageUpdateInterval);
    YBP.printf("Shunt Voltage Update Interval: %dms\n", amperageUpdateInterval);
  }

  int x = ina226->setMaxCurrentShunt(YB_PWM_CHANNEL_SHORT_CIRCUIT_AMPS, YB_PWM_CHANNEL_INA226_SHUNT);

  if (this->id == 1) {
    YBP.println("normalized = true (default)");
    YBP.println(x);
    YBP.print("LSB:\t");
    YBP.println(ina226->getCurrentLSB(), 10);
    YBP.print("LSB_uA:\t");
    YBP.println(ina226->getCurrentLSB_uA(), 3);
    YBP.print("shunt:\t");
    YBP.println(ina226->getShunt(), 3);
    YBP.print("maxCur:\t");
    YBP.println(ina226->getMaxCurrent(), 3);
    YBP.println();
  }

  // Configure the ALERT pin to fire when current exceeds YB_PWM_CHANNEL_MAX_AMPS.
  // The INA226 has no direct overcurrent alert, but shunt voltage is proportional
  // to current (V = I * R), so we use the Shunt Over Voltage alert mode instead.
  //
  // Shunt voltage at the overcurrent threshold:
  //   V_shunt = MAX_AMPS * SHUNT_OHMS
  //           = YB_PWM_CHANNEL_MAX_AMPS * YB_PWM_CHANNEL_INA226_SHUNT
  //
  // The Alert Limit register uses a fixed 2.5 µV/LSB (hardware constant, matches
  // the shunt voltage register resolution), so:
  //   alert_limit = V_shunt / 2.5e-6
  //
  // Latch mode is enabled so the ALERT pin stays asserted until the MCU reads the
  // Mask/Enable register — prevents the interrupt from re-firing before it is serviced.
  ina226->setAlertRegister(INA226_SHUNT_OVER_VOLTAGE | INA226_ALERT_LATCH_ENABLE_FLAG);
  float shuntVoltageLimit = YB_PWM_CHANNEL_SHORT_CIRCUIT_AMPS * YB_PWM_CHANNEL_INA226_SHUNT;
  ina226->setAlertLimit((uint16_t)(shuntVoltageLimit / 2.5e-6));

  if (this->id == 1) {
    YBP.printf("Shunt Voltage Limit %.4fV\n", shuntVoltageLimit);
  }

  pinMode(_ina226_alert_pins[this->id - 1], INPUT);
  attachInterruptArg(
    digitalPinToInterrupt(_ina226_alert_pins[this->id - 1]),
    PWMChannel::ina226AlertHandler,
    this,
    FALLING);
    #endif
}

void PWMChannel::readINA226()
{
  if (millis() - lastVoltageUpdate > voltageUpdateInterval) {
    lastVoltageUpdate = millis();
    float voltage = ina226->getBusVoltage();
    if (voltage < 0)
      voltage = 0;
    voltageAverage.add(voltage * YB_INA226_FACTOR);
  }

  if (millis() - lastAmperageUpdate > amperageUpdateInterval) {
    lastAmperageUpdate = millis();
    float amperage = ina226->getCurrent();
    if (amperage < 0)
      amperage = 0;
    amperageAverage.add(amperage * YB_INA226_FACTOR);
  }
}

void IRAM_ATTR PWMChannel::ina226AlertHandler(void* arg)
{
  PWMChannel* ch = static_cast<PWMChannel*>(arg);

    // Kill the output immediately via direct GPIO register write.  This bypasses
    // ledcWrite() (not IRAM-safe) and cuts the pin within a single CPU cycle.
    //
    // ledcOutputInvert() flips the LEDC signal in hardware, which means the
    // physical pin level for "off" is the opposite of a non-inverted channel:
    //   Normal  : pin LOW  = output off → write-1-to-clear (w1tc)
    //   Inverted: pin HIGH = output off → write-1-to-set   (w1ts)
    //
    // GPIO.out_w1tc / GPIO.out1_w1tc cover pins 0-31 / 32+.
    // Note: direct GPIO writes bypass LEDC inversion, so LEDC will re-assert
    // on its next PWM cycle.  ina226TripPending causes checkSoftFuse() to call
    // ledcWrite(0) promptly, clamping any glitch to at most one PWM period (~1ms).
    #ifdef YB_PWM_CHANNEL_INVERTED
  if (ch->pin < 32)
    GPIO.out_w1ts = (1UL << ch->pin); // plain uint32_t on ESP32-S3
  else
    GPIO.out1_w1ts.val = (1UL << (ch->pin - 32)); // struct with .val for pins 32+
    #else
  if (ch->pin < 32)
    GPIO.out_w1tc = (1UL << ch->pin);
  else
    GPIO.out1_w1tc.val = (1UL << (ch->pin - 32));
    #endif

  // Signal the main loop to run the full trip sequence (ledcWrite, status
  // update, buzzer, MQTT, etc.) — none of those are safe to call from an ISR.
  ch->ina226TripPending = true;
}

void PWMChannel::handleINA226Trip()
{
  if (!this->ina226TripPending)
    return;

  this->ina226TripPending = false;

  YBP.printf("CH%d TRIPPED (INA226 Interrupt)\n", this->id, this->getStatus());

  this->status = Status::TRIPPED;
  this->outputState = false;

  // Make LEDC agree with the GPIO state the ISR already forced.
  this->updateOutput(false);

  // Reading the Mask/Enable register clears the alert latch on the INA226,
  // re-arming it for the next overcurrent event.
  ina226->getAlertRegister();

  this->softFuseTripCount = this->softFuseTripCount + 1;
  this->sendFastUpdate = true;
  strlcpy(this->source, _cfg->local_hostname, sizeof(this->source));

  char prefIndex[YB_PREF_KEY_LENGTH];
  sprintf(prefIndex, "pwmTripCount%d", this->id);
  _cfg->preferences.putUInt(prefIndex, this->softFuseTripCount);

  if (buzzer)
    buzzer->playMelodyByName(this->trippedMelody.c_str());
}

  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
void PWMChannel::readLM75()
{
  if (millis() - lastTemperatureUpdate > 1000) {
    lastTemperature = lm75->readTemperatureC();
    lastTemperatureUpdate = millis();
  }
}
  #endif

void PWMChannel::setupLedc()
{
  // track our fades
  this->isFading = false;
  this->fadeOver = false;

  // now attach ledc
  if (!ledcAttach(this->pin, YB_PWM_CHANNEL_FREQUENCY, YB_PWM_CHANNEL_RESOLUTION))
    YBP.printf("PWM CH%d error attaching to LEDC\n");

  #ifdef YB_PWM_CHANNEL_INVERTED
  ledcOutputInvert(this->pin, true);
  #endif

  this->writePWM(0);
}

void PWMChannel::setupOffset()
{
  // make sure we're off for testing.
  this->outputState = false;
  this->status = Status::OFF;
  this->updateOutput(false);

  // how many?
  byte readings = 10;

  // // average a bunch of readings
  float tv = 0;
  #ifdef YB_HAS_CHANNEL_VOLTAGE
  for (byte i = 0; i < readings; i++) {
    tv += this->voltageHelper->getNewVoltage(_adcVoltageChannel);
  }
  float v = this->toVoltage(tv / readings);
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  for (byte i = 0; i < readings; i++) {
    if (ina226->waitConversionReady())
      tv += ina226->getBusVoltage();
  }
  float v = tv / readings;
  #endif

  // low enough value to be an offset?
  this->voltageOffset = 0.0;
  if (v < (30.0 * 0.05))
    this->voltageOffset = v;

  // average a bunch of readings
  float ta = 0;
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  for (byte i = 0; i < readings; i++) {
    ta += this->amperageHelper->getNewVoltage(_adcAmperageChannel);
  }
  float a = this->toAmperage(ta / readings);
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  for (byte i = 0; i < readings; i++) {
    if (ina226->waitConversionReady())
      ta += ina226->getCurrent();
  }
  float a = ta / readings;
  #endif

  // amperage zero state
  this->amperageOffset = 0.0;
  if (a < (YB_PWM_CHANNEL_MAX_AMPS * 0.05))
    this->amperageOffset = a;

  YBP.printf("CH%d Voltage Offset: %0.3f / Amperage Offset: %0.3f\n", this->id, this->voltageOffset, this->amperageOffset);
}

void PWMChannel::setupDefaultState()
{
  // load our default status.
  if (!strcmp(this->defaultState, "ON")) {
    this->outputState = true;
    this->status = Status::ON;
  } else {
    this->outputState = false;
    this->status = Status::OFF;
  }

  // update our pin, but dont check it
  this->updateOutput(false);
}

void PWMChannel::saveThrottledDutyCycle()
{
  // after 5 secs of no activity, we can save it.
  if (this->dutyCycleIsThrottled && millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT)
    this->setDuty(this->dutyCycle);
}

float PWMChannel::getCurrentDutyCycle()
{
  unsigned int pwm = ledcRead(this->pin);
  return (float)pwm / (float)MAX_DUTY_CYCLE;
}

void PWMChannel::updateOutput(bool check_status)
{
  // turn off disabled channels and ignore them.
  if (!this->isEnabled) {
    this->writePWM(0);
    this->outputState = false;
    return;
  }

  // first of all, if its tripped or blown zero it out.
  if (this->status == Status::TRIPPED || this->status == Status::BLOWN || this->status == Status::OVERHEAT) {
    this->writePWM(0);
    this->outputState = false;
    return;
  }

  // regular on/off outputs just do it.
  if (!this->isDimmable) {
    if (this->outputState)
      this->writePWM(MAX_DUTY_CYCLE);
    else
      this->writePWM(0);
    return;
  }

  // for non lights, its simple...just output our duty
  if (strcmp(this->type, "light")) {
    int pwm = this->dutyCycle * MAX_DUTY_CYCLE;
    if (this->outputState)
      this->writePWM(pwm);
    else
      this->writePWM(0);
    return;
  }
  // otherwise for lights, we need to deal with brightness, fading, etc.
  else {
    float duty = this->dutyCycle;

    // duty constraints 0....1
    duty = max(0.0f, duty);
    duty = min(1.0f, duty);

    // apply a gamma correction to our brightness.
    duty = powf(duty, 2.2);

    // okay, set our pin state.
    if (!this->isFading) {
      // do we want it on or off?
      if (this->outputState) {
        // ramp or no?
        if (this->rampOnMillis)
          this->startFade(duty, rampOnMillis);
        else
          this->writePWM(duty * MAX_DUTY_CYCLE);
      }
      // okay, turn it off.
      else {
        if (this->rampOffMillis)
          this->startFade(0, rampOffMillis);
        else
          this->writePWM(0);
      }
    }
  }

  // see what we're working with.
  if (check_status)
    this->checkStatus();
}

float PWMChannel::toAmperage(float voltage)
{
  #ifdef YB_PWM_CHANNEL_V_PER_AMP
  float amps = (voltage - (3.3 * 0.1)) / (YB_PWM_CHANNEL_V_PER_AMP);

  amps = amps - this->amperageOffset;

  // our floor is zero amps
  amps = max((float)0.0, amps);

  return amps;
  #else
  return -1;
  #endif
}

float PWMChannel::getLatestVoltage()
{
  #ifdef YB_HAS_CHANNEL_VOLTAGE
  return this->toVoltage(this->voltageHelper->getAverageVoltage(_adcVoltageChannel));
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  return voltageAverage.latest() / YB_INA226_FACTOR;
  #else
  return -1;
  #endif
}

float PWMChannel::getAverageVoltage()
{
  #ifdef YB_HAS_CHANNEL_VOLTAGE
  return this->toVoltage(this->voltageHelper->getAverageVoltage(_adcVoltageChannel));
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  return voltageAverage.average() / YB_INA226_FACTOR;
  #else
  return -1;
  #endif
}

float PWMChannel::getLatestAmperage()
{
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  return this->toAmperage(this->amperageHelper->getAverageVoltage(this->_adcAmperageChannel));
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  return amperageAverage.latest() / YB_INA226_FACTOR;
  #else
  return -1;
  #endif
}

float PWMChannel::getAverageAmperage()
{
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  return this->toAmperage(this->amperageHelper->getAverageVoltage(this->_adcAmperageChannel));
  #elifdef YB_PWM_CHANNEL_HAS_INA226
  return amperageAverage.average() / YB_INA226_FACTOR;
  #else
  return -1;
  #endif
}

float PWMChannel::getLatestWattage()
{
  if (this->dutyCycle == 1.0)
    return this->getLatestVoltage() * this->getLatestAmperage();
  else
    return busVoltage->getBusVoltage() * this->getLatestAmperage();
}

float PWMChannel::getAverageWattage()
{
  if (this->dutyCycle == 1.0)
    return this->getAverageVoltage() * this->getAverageAmperage();
  else
    return busVoltage->getBusVoltage() * this->getAverageAmperage();
}

float PWMChannel::toVoltage(float adcVoltage)
{
  #ifdef YB_PWM_CHANNEL_VOLTAGE_R1
  float v = adcVoltage / (YB_PWM_CHANNEL_VOLTAGE_R2 / (YB_PWM_CHANNEL_VOLTAGE_R2 + YB_PWM_CHANNEL_VOLTAGE_R1));
  v = v - this->voltageOffset;

  // our floor is zero volts
  v = max((float)0.0, v);

  return v;
  #else
  return -1;
  #endif
}

float PWMChannel::getTemperature()
{
  #ifdef YB_PWM_CHANNEL_HAS_LM75
  return lastTemperature;
  #else
  return -1;
  #endif
}

void PWMChannel::checkStatus()
{
  this->checkFuseBypassed();
  this->checkSoftFuse();
  this->checkFuseBlown();
  this->checkOverheat();
}

void PWMChannel::updateOutputLED()
{
  #if (YB_STATUS_RGB_COUNT > 1)
  if (this->status == Status::ON)
    rgb->setPixelColor(this->id, CRGB::Green);
  else if (this->status == Status::OFF)
    rgb->setPixelColor(this->id, CRGB::Black);
  else if (this->status == Status::TRIPPED)
    rgb->setPixelColor(this->id, CRGB::Yellow);
  else if (this->status == Status::BLOWN)
    rgb->setPixelColor(this->id, CRGB::Red);
  else if (this->status == Status::BYPASSED)
    rgb->setPixelColor(this->id, CRGB::Blue);
  else if (this->status == Status::OVERHEAT)
    rgb->setPixelColor(this->id, CRGB::Orange);
  else
    rgb->setPixelColor(this->id, CRGB::Black);
  #endif
}

void PWMChannel::checkFuseBlown()
{
  // how should we treat our duty cycle?
  float duty;
  if (this->isDimmable)
    duty = getCurrentDutyCycle();
  else
    duty = 1.0;

  // we cant really detect much at low levels
  // const float dutyFloor = 0.10;
  // if (duty < dutyFloor)
  //   return;

  // determine what our floor for a tripped fuse should be.
  float bv = busVoltage->getBusVoltage();
  float minVoltage = bv * duty * 0.70;

  // so hard to measure that low accurately over time and not false
  if (minVoltage <= 0.10)
    return;

  // dimming lights need more time.
  unsigned long firstCheckTime = 1000;
  if (isDimmable && !strcmp(this->type, "light"))
    firstCheckTime += rampOnMillis;

  // fades are also really hard to measure.
  if (this->isFading)
    return;

  // we need bus voltage for our calculations.
  // it takes a little bit to populate on boot.
  if (busVoltage->getBusVoltage() > 0) {
    if (this->status == Status::ON) {
      // grace period when changing state or duty
      if (millis() - lastStateChange > firstCheckTime && millis() - lastDutyCycleUpdate > firstCheckTime) {

        float voltage = this->getAverageVoltage();

        // if our voltage is at a high enough level, we're fine.
        if (voltage >= minVoltage)
          return;

        YBP.printf("CH%d BLOWN: %.3f < %.3f | DUTY: %.2f | isDimmable: %d | BUS VOLTAGE: %.3f\n", this->id, voltage, minVoltage, duty, isDimmable, bv);
  #ifdef YB_HAS_CHANNEL_VOLTAGE
        this->voltageHelper->printDebug(this->id - 1, true);
  #endif
        this->status = Status::BLOWN;
        this->outputState = false;
        this->updateOutput(false);

        if (buzzer)
          buzzer->playMelodyByName(this->blownMelody.c_str());
      }
    }
  }
}

void PWMChannel::checkFuseBypassed()
{
  if (this->status != Status::ON && this->status != Status::BYPASSED && !this->isFading) {

    // dimming lights are a special case...
    unsigned long firstCheckTime = 1000;
    if (isDimmable && !strcmp(this->type, "light"))
      firstCheckTime += rampOffMillis;

    // grace period when changing state or duty
    if (millis() - lastStateChange > firstCheckTime && millis() - lastDutyCycleUpdate > firstCheckTime) {

      float voltage = this->getAverageVoltage();
      float bv = busVoltage->getBusVoltage();
      float minVoltage = bv * 0.90;

      if (voltage < minVoltage)
        return;

      YBP.printf("CH%d BYPASSED: %.3f >= %.3f | STATUS: %s | BUS VOLTAGE: %.3f\n", this->id, voltage, minVoltage, this->getStatus(), bv);
  #ifdef YB_HAS_CHANNEL_VOLTAGE
      this->voltageHelper->printDebug(this->id - 1, true);
  #endif
      // dont change our outputState here... bypass can be temporary
      this->status = Status::BYPASSED;

      if (buzzer)
        buzzer->playMelodyByName(this->bypassMelody.c_str());
    }
  } else if (this->status == Status::BYPASSED) {
    if (!this->outputState && this->getAverageVoltage() < 2.0) {
      this->status = Status::OFF;
    }
  }
}

void PWMChannel::checkSoftFuse()
{
  #ifdef YB_PWM_CHANNEL_HAS_INA226
  this->handleINA226Trip();
  #endif

  // grace period when changing state or duty
  unsigned long firstCheckTime = 1000;
  if (millis() - lastStateChange < firstCheckTime)
    return;

  // only trip once....
  if (this->status != Status::TRIPPED) {
    float amperage = abs(this->getAverageAmperage());

    // Check our soft fuse, and our max limit for the board.
    if (amperage >= this->softFuseAmperage || amperage >= YB_PWM_CHANNEL_MAX_AMPS) {

      YBP.printf("CH%d TRIPPED: %.3f >= %.3f (soft) or %.3f (max) | STATUS: %s\n", this->id, amperage, this->softFuseAmperage, YB_PWM_CHANNEL_MAX_AMPS, this->getStatus());
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
      this->amperageHelper->printDebug(this->id - 1, true);
  #endif

      // record some variables
      this->status = Status::TRIPPED;
      this->outputState = false;

      // actually shut it down!
      this->updateOutput(false);

      // our counter
      this->softFuseTripCount = this->softFuseTripCount + 1;

      // we will want to notify
      this->sendFastUpdate = true;

      // this is an internally originating change
      strlcpy(this->source, _cfg->local_hostname, sizeof(this->source));

      // save to our storage
      char prefIndex[YB_PREF_KEY_LENGTH];
      sprintf(prefIndex, "pwmTripCount%d", this->id);
      _cfg->preferences.putUInt(prefIndex, this->softFuseTripCount);

      if (buzzer)
        buzzer->playMelodyByName(this->trippedMelody.c_str());
    }
  }
}

void PWMChannel::checkOverheat()
{
  #ifdef YB_PWM_CHANNEL_HAS_LM75
  // only trip once....
  if (this->status != Status::OVERHEAT) {
    // are we too hot?
    if (this->getTemperature() >= YB_PWM_CHANNEL_MAX_TEMPERATURE) {

      // record some variables
      this->status = Status::OVERHEAT;
      this->outputState = false;

      // actually shut it down!
      this->updateOutput(false);

      // our counter
      this->overheatCount++;

      // we will want to notify
      this->sendFastUpdate = true;

      // this is an internally originating change
      strlcpy(this->source, _cfg->local_hostname, sizeof(this->source));

      // save to our storage
      char prefIndex[YB_PREF_KEY_LENGTH];
      sprintf(prefIndex, "pwmOhtCount%d", this->id);
      _cfg->preferences.putUInt(prefIndex, this->overheatCount);

      if (buzzer)
        buzzer->playMelodyByName(this->overheatMelody.c_str());
    }
  } else if (this->status == Status::OVERHEAT) {
    // are we too hot?
    if (this->getTemperature() <= YB_PWM_CHANNEL_RESET_TEMPERATURE) {

      // record some variables
      this->status = Status::OFF;
      this->outputState = false;

      // actually shut it down!
      this->updateOutput(false);

      // we will want to notify
      this->sendFastUpdate = true;
    }
  }
  #endif
}

void PWMChannel::startFade(float duty, int fade_time)
{
  // is an earlier hardware fade blocking?
  if (!this->isFading) {
    // setup for our hardware fader
    fade_time = max(1, fade_time);
    const uint32_t start_duty = ledcRead(this->pin); // start from where you are
    uint32_t end_duty = (duty * MAX_DUTY_CYCLE);     // end at desired duty

    // nothing happening.
    if (start_duty == end_duty)
      return;

    unsigned int scaled_fade_time;
    if (start_duty > end_duty)
      scaled_fade_time = ((float)(start_duty - end_duty) / (float)MAX_DUTY_CYCLE) * fade_time;
    else
      scaled_fade_time = ((float)(end_duty - start_duty) / (float)MAX_DUTY_CYCLE) * fade_time;

    // track our fade status
    this->isFading = true;

    // kicks off fade and registers our ISR + arg (the channel index)
    // bool ok = ledcFadeWithInterruptArg(
    bool ok = gammaFadeWithInterrupt(
      this->pin,
      start_duty,
      end_duty,
      scaled_fade_time,
      &PWMChannel::onFadeISR,
      this);

    // if it failed, clear flag and handle your error path (log, retry, etc.)
    if (!ok) {
      this->isFading = false;
    }

    // track when we last changed states.
    this->lastStateChange = millis();

    // clear our averages
    voltageAverage.clear();
    amperageAverage.clear();

    // immediately check for trips
    uint32_t start = millis();
    while (millis() - start < 2)
      this->handleINA226Trip();
  }
}

bool PWMChannel::gammaFadeWithInterrupt(
  uint8_t pin_,
  uint32_t start_duty,
  uint32_t end_duty,
  int total_ms,
  void (*final_cb)(void*),
  void* final_arg)
{
  constexpr uint8_t SEGMENTS = 16;
  constexpr float GAMMA = 2.2f;
  constexpr uint32_t MAX_DUTY = (1u << YB_PWM_CHANNEL_RESOLUTION) - 1; // 13-bit LEDC

  if (total_ms <= 0)
    total_ms = 1;

  gamma.pin = pin_;
  gamma.idx = 0;
  gamma.user_cb = final_cb;
  gamma.user_arg = final_arg;
  gamma.active = true;
  gamma.owner = this;

  // even timing; remainder to last segment
  gamma.step_ms = (SEGMENTS > 1) ? (total_ms / SEGMENTS) : total_ms;
  if (gamma.step_ms <= 0)
    gamma.step_ms = 1;
  gamma.last_ms = total_ms - gamma.step_ms * (SEGMENTS - 1);
  if (gamma.last_ms <= 0)
    gamma.last_ms = gamma.step_ms;

  // precompute gamma-corrected target duties
  const float b0 = powf((float)start_duty / (float)MAX_DUTY, 1.0f / GAMMA);
  const float b1 = powf((float)end_duty / (float)MAX_DUTY, 1.0f / GAMMA);

  for (uint8_t i = 0; i < SEGMENTS; ++i) {
    const float t = (float)(i + 1) / (float)SEGMENTS;
    float bp = b0 + (b1 - b0) * t;
    if (bp < 0)
      bp = 0;
    else if (bp > 1)
      bp = 1;
    const float pf = powf(bp, GAMMA);
    gamma.targets[i] = (uint32_t)lroundf(pf * (float)MAX_DUTY);
  }

  const uint32_t first_to = gamma.targets[0];
  const int first_ms = gamma.step_ms;

  // start first segment — our ISR chains the rest
  return ledcFadeWithInterruptArg(pin_,
    start_duty,
    first_to,
    first_ms,
    &PWMChannel::gammaISR,
    &gamma);
}

void IRAM_ATTR PWMChannel::gammaISR(void* arg)
{
  auto* S = static_cast<PWMChannel::GammaState*>(arg);
  if (!S || !S->active)
    return;

  BaseType_t hpw = pdFALSE;
  xTimerPendFunctionCallFromISR(&PWMChannel::continueGammaThunk, arg, 0, &hpw);
  if (hpw)
    portYIELD_FROM_ISR();
}

void PWMChannel::continueGammaThunk(void* arg, uint32_t)
{
  auto* S = static_cast<GammaState*>(arg);
  if (!S || !S->active)
    return;

  constexpr uint8_t SEGMENTS = 16;

  if (S->idx + 1 < SEGMENTS) {
    const uint32_t from = S->targets[S->idx];
    const uint32_t to = S->targets[S->idx + 1];
    const bool lastSeg = (S->idx + 1 == SEGMENTS - 1);
    const int dur_ms = lastSeg ? S->last_ms : S->step_ms;
    S->idx++;

    // SAFE here (task context):
    ledcFadeWithInterruptArg(S->pin, from, to, dur_ms, &PWMChannel::gammaISR, S);
  } else {
    S->active = false;
    if (S->user_cb)
      S->user_cb(S->user_arg); // also safe here if you prefer
  }
}

void PWMChannel::checkIfFadeOver()
{
  // has our fade ended?
  if (!this->isFading && this->fadeOver) {
    this->fadeOver = false;
    this->updateOutput(true);
  }
}

void PWMChannel::setDuty(float duty)
{
  this->dutyCycle = duty;

  // it only makes sense to change it to non-zero.
  if (this->dutyCycle > 0) {
    // only if we have changed
    if (this->dutyCycle != this->lastDutyCycle) {
      // we don't want to swamp the flash
      if (millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT) {
        char prefIndex[YB_PREF_KEY_LENGTH];
        sprintf(prefIndex, "pwmDuty%d", this->id);
        _cfg->preferences.putFloat(prefIndex, duty);
        this->dutyCycleIsThrottled = false;
        // YBP.printf("saving %s: %f\n", prefIndex, this->dutyCycle);

        this->lastDutyCycle = this->dutyCycle;
      }
      // make a note so we can save later.
      else
        this->dutyCycleIsThrottled = true;
    }
  }

  // we want the clock to reset every time we change the duty cycle
  // this way, long led fading sessions are only one write.
  this->lastDutyCycleUpdate = millis();
}

void PWMChannel::calculateAverages(unsigned int delta)
{
  // record our total consumption
  if (this->getAverageAmperage() > 0) {
    this->ampHours += this->getAverageAmperage() * ((float)delta / 3600000.0);
    this->wattHours += this->getAverageWattage() * ((float)delta / 3600000.0);
  }
}

void PWMChannel::setState(const char* state)
{
  if (!strcmp(state, "ON")) {
    this->status = Status::ON;
    this->setState(true);

  } else if (!strcmp(state, "OFF")) {
    this->status = Status::OFF;
    this->setState(false);
  }
}

void PWMChannel::setState(bool newState)
{
  if (newState != outputState) {

    // save our output state
    this->outputState = newState;

    // keep track of how many toggles
    this->stateChangeCount++;

    // what is our new status?
    if (newState)
      this->status = Status::ON;
    else
      this->status = Status::OFF;

      // clear our readings since we want fresh readings.
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
    this->amperageHelper->clearReadings(_adcAmperageChannel);
  #endif
  #ifdef YB_HAS_CHANNEL_VOLTAGE
    this->voltageHelper->clearReadings(_adcVoltageChannel);
  #endif

    // change our output pin to reflect
    this->updateOutput(true);

    // flag for update to clients
    this->sendFastUpdate = true;
  }
}

void PWMChannel::writePWM(uint16_t pwm)
{
  pwm = constrain(pwm, 0, MAX_DUTY_CYCLE);
  ledcWrite(this->pin, pwm);

  // track when we last changed states.
  this->lastStateChange = millis();

  // clear our averages
  voltageAverage.clear();
  amperageAverage.clear();

  // immediately check for trips
  if (pwm > 0) {
    uint32_t start = millis();
    while (millis() - start < 2)
      this->handleINA226Trip();
  }
}

const char* PWMChannel::getStatus()
{
  if (this->status == Status::ON)
    return "ON";
  else if (this->status == Status::OFF)
    return "OFF";
  else if (this->status == Status::TRIPPED)
    return "TRIPPED";
  else if (this->status == Status::BLOWN)
    return "BLOWN";
  else if (this->status == Status::BYPASSED)
    return "BYPASSED";
  else if (this->status == Status::OVERHEAT)
    return "OVERHEAT";
  else
    return "OFF";
}

void PWMChannel::haGenerateDiscovery(JsonVariant doc, const char* uuid, MQTTController* mqtt)
{
  BaseChannel::haGenerateDiscovery(doc, uuid, mqtt);

  // generate our topics
  sprintf(ha_topic_cmd_state, "yarrboard/%s/pwm/%s/ha_set", ha_key, this->key);
  sprintf(ha_topic_state_state, "yarrboard/%s/pwm/%s/ha_state", ha_key, this->key);
  sprintf(ha_topic_cmd_brightness, "yarrboard/%s/pwm/%s/ha_brightness/set", ha_key, this->key);
  sprintf(ha_topic_state_brightness, "yarrboard/%s/pwm/%s/ha_brightness/state", ha_key, this->key);
  sprintf(ha_topic_voltage, "yarrboard/%s/pwm/%s/voltage", ha_key, this->key);
  sprintf(ha_topic_current, "yarrboard/%s/pwm/%s/current", ha_key, this->key);
  sprintf(ha_topic_wattage, "yarrboard/%s/pwm/%s/wattage", ha_key, this->key);

  // Register command-topic callbacks once; haGenerateDiscovery is called on every
  // HA reconnect so we must guard against accumulating duplicate entries.
  if (!_haCallbacksRegistered) {
    mqtt->onTopic(ha_topic_cmd_state, 0, &PWMController::handleHACommandCallbackStatic);
    mqtt->onTopic(ha_topic_cmd_brightness, 0, &PWMController::handleHACommandCallbackStatic);
    _haCallbacksRegistered = true;
  }

  this->haGenerateLightDiscovery(doc);
  this->haGenerateVoltageDiscovery(doc);
  this->haGenerateAmperageDiscovery(doc);
  this->haGenerateWattageDiscovery(doc);
}

void PWMChannel::haGenerateLightDiscovery(JsonVariant doc)
{
  // configuration object for the individual channel
  JsonObject obj = doc[ha_uuid].to<JsonObject>();
  obj["platform"] = "light";
  obj["name"] = this->name;
  obj["unique_id"] = ha_uuid;
  obj["command_topic"] = ha_topic_cmd_state;
  obj["state_topic"] = ha_topic_state_state;
  obj["payload_on"] = "ON";
  obj["payload_off"] = "OFF";

  if (this->isDimmable) {
    obj["brightness_command_topic"] = ha_topic_cmd_brightness;
    obj["brightness_state_topic"] = ha_topic_state_brightness;
  }

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void PWMChannel::haGenerateVoltageDiscovery(JsonVariant doc)
{
  // configuration object for the channel voltage
  char ha_uuid_voltage[128];
  sprintf(ha_uuid_voltage, "%s_voltage", ha_uuid);
  char ha_voltage_name[128];
  sprintf(ha_voltage_name, "%s Volts", this->name);

  JsonObject obj = doc[ha_uuid_voltage].to<JsonObject>();
  obj["p"] = "sensor";
  obj["name"] = ha_voltage_name;
  obj["unique_id"] = ha_uuid_voltage;
  obj["state_topic"] = ha_topic_voltage;
  obj["unit_of_measurement"] = "V";
  obj["device_class"] = "voltage";
  obj["state_class"] = "measurement";
  obj["entity_category"] = "diagnostic";

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void PWMChannel::haGenerateAmperageDiscovery(JsonVariant doc)
{
  // configuration object for the channel voltage
  char ha_uuid_amperage[128];
  sprintf(ha_uuid_amperage, "%s_amperage", ha_uuid);
  char ha_amperage_name[128];
  sprintf(ha_amperage_name, "%s Amps", this->name);

  JsonObject obj = doc[ha_uuid_amperage].to<JsonObject>();
  obj["p"] = "sensor";
  obj["name"] = ha_amperage_name;
  obj["unique_id"] = ha_uuid_amperage;
  obj["state_topic"] = ha_topic_current;
  obj["unit_of_measurement"] = "A";
  obj["device_class"] = "current";
  obj["state_class"] = "measurement";
  obj["entity_category"] = "diagnostic";

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void PWMChannel::haGenerateWattageDiscovery(JsonVariant doc)
{
  // configuration object for the channel voltage
  char ha_uuid_wattage[128];
  sprintf(ha_uuid_wattage, "%s_wattage", ha_uuid);
  char ha_wattage_name[128];
  sprintf(ha_wattage_name, "%s Wattage", this->name);

  JsonObject obj = doc[ha_uuid_wattage].to<JsonObject>();
  obj["p"] = "sensor";
  obj["name"] = ha_wattage_name;
  obj["unique_id"] = ha_uuid_wattage;
  obj["state_topic"] = ha_topic_wattage;
  obj["unit_of_measurement"] = "W";
  obj["device_class"] = "power";
  obj["state_class"] = "measurement";
  obj["entity_category"] = "diagnostic";

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;
}

void PWMChannel::haPublishState(MQTTController* mqtt)
{
  if (this->status == Status::ON || this->status == Status::BYPASSED)
    mqtt->publish(ha_topic_state_state, "ON", false);
  else
    mqtt->publish(ha_topic_state_state, "OFF", false);

  if (this->isDimmable) {
    char b[8];
    int brightness = (int)roundf(255.0f * this->dutyCycle);
    if (brightness < 0)
      brightness = 0;
    if (brightness > 255)
      brightness = 255;
    snprintf(b, sizeof(b), "%u", brightness);

    mqtt->publish(ha_topic_state_brightness, b, false);
  }
}

void PWMChannel::haHandleCommand(const char* topic, const char* payload)
{
  if (!strcmp(ha_topic_cmd_state, topic)) {
    if (!strcmp(payload, "ON"))
      this->setState(true);
    else
      this->setState(false);
  }

  // set our brightness
  if (!strcmp(ha_topic_cmd_brightness, topic)) {
    // Parse payload into integer (0–255)
    int value = atoi(payload);
    if (value < 0)
      value = 0;
    if (value > 255)
      value = 255;

    // Convert to float (0.0–1.0)
    float duty = (float)value / 255.0f;

    // Set duty cycle
    this->setDuty(duty);

    // turn it on
    this->updateOutput(true);
  }
}

void PWMChannel::init(uint8_t id)
{
  BaseChannel::init(id);
  this->channel_type = "pwm";

  this->pin = _pwm_pins[id - 1]; // pins are zero indexed, we are 1 indexed

  snprintf(this->name, sizeof(this->name), "PWM Channel %d", id);
}

bool PWMChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  isDimmable = config["isDimmable"] | true;

  if (config["softFuse"]) {
    softFuseAmperage = config["softFuse"];
    softFuseAmperage = constrain(softFuseAmperage, 0, YB_PWM_CHANNEL_MAX_AMPS);
  }

  strlcpy(this->type, config["type"] | "other", sizeof(this->type));
  strlcpy(this->defaultState, config["defaultState"] | "OFF", sizeof(this->defaultState));

  this->bypassMelody = config["bypassMelody"] | YB_BYPASS_MELODY;
  this->bypassMelody = config["trippedMelody"] | YB_TRIPPED_MELODY;
  this->bypassMelody = config["blownMelody"] | YB_BLOWN_MELODY;
  this->bypassMelody = config["overheatMelody"] | YB_OVERHEAT_MELODY;

  return true;
}

void PWMChannel::generateConfig(JsonVariant config)
{
  BaseChannel::generateConfig(config);

  config["type"] = this->type;
  config["hasCurrent"] = true;
  config["softFuse"] = round2(this->softFuseAmperage);
  config["isDimmable"] = this->isDimmable;
  config["defaultState"] = this->defaultState;

  config["bypassMelody"] = this->bypassMelody;
  config["trippedMelody"] = this->trippedMelody;
  config["blownMelody"] = this->blownMelody;
  config["overheatMelody"] = this->overheatMelody;
}

void PWMChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["state"] = this->getStatus();
  config["source"] = this->source;
  if (this->isDimmable)
    config["duty"] = round2(this->dutyCycle);
  config["voltage"] = round2(this->getAverageVoltage());
  config["current"] = round2(this->getAverageAmperage());
  config["wattage"] = round2(this->getAverageWattage());
  config["temperature"] = round2(this->getTemperature());
  config["aH"] = round3(this->ampHours);
  config["wH"] = round3(this->wattHours);
}

void PWMChannel::generateStats(JsonVariant output)
{
  output["id"] = id;
  output["name"] = name;
  output["aH"] = ampHours;
  output["wH"] = wattHours;
  output["state_change_count"] = stateChangeCount;
  output["soft_fuse_trip_count"] = softFuseTripCount;
  output["overheat_count"] = overheatCount;
}

#endif