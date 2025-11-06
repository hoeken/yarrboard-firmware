/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_PWM_CHANNELS

  // #include "INA226.h"
  #include "debug.h"
  #include "pwm_channel.h"
  #include "rgb.h"

// INA226 INA(0x40);
// unsigned long previousINA226UpdateMillis = 0;

unsigned long previousHAUpdateMillis = 0;

// the main star of the event
etl::array<PWMChannel, YB_PWM_CHANNEL_COUNT> pwm_channels;

// our channel pins
byte _pwm_pins[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_PINS;

/* Setting PWM Properties */
const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION)) - 1;

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
MCP3564 _adcCurrentMCP3564(YB_PWM_CHANNEL_CURRENT_ADC_CS, &SPI, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_SCK);
MCP3564Helper* adcCurrentHelper;
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
ADS1115 _adcVoltageADS1115_1(YB_PWM_CHANNEL_VOLTAGE_I2C_ADDRESS_1);
ADS1115 _adcVoltageADS1115_2(YB_PWM_CHANNEL_VOLTAGE_I2C_ADDRESS_2);
ADS1115Helper* adcVoltageHelper1;
ADS1115Helper* adcVoltageHelper2;
    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
MCP3564 _adcVoltageMCP3564(YB_PWM_CHANNEL_VOLTAGE_ADC_CS, &SPI, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_SCK);
MCP3564Helper* adcVoltageHelper;
    #endif
  #endif

void pwm_channels_setup()
{
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_CURRENT_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_CURRENT_ADC_SCK, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_CS);
  if (!_adcCurrentMCP3564.begin(0, 3.3)) {
    YBP.println("failed to initialize current MCP3564");
  } else {
    YBP.println("MCP3564 ok.");
  }

  _adcCurrentMCP3564.singleEndedMode();
  _adcCurrentMCP3564.setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcCurrentMCP3564.setAveraging(MCP3x6x::osr::OSR_1024);

  // _adcCurrentMCP3564.printConfig();

  // YBP.print("VDD: ");
  // YBP.println(_adcCurrentMCP3564.analogRead(MCP_AVDD));

  // YBP.print("TEMP: ");
  // YBP.println(_adcCurrentMCP3564.analogRead(MCP_TEMP));

  adcCurrentHelper = new MCP3564Helper(3.3, &_adcCurrentMCP3564, 50, 500);
  adcCurrentHelper->attachReadyPinInterrupt(YB_PWM_CHANNEL_CURRENT_ADC_IRQ, FALLING);

  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115

  Wire.begin();
  Wire.setClock(YB_I2C_SPEED);

  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    YBP.println("Voltage ADS115 #1 OK");
  else
    YBP.println("Voltage ADS115 #1 Not Found");

  _adcVoltageADS1115_1.setMode(ADS1X15_MODE_SINGLE); //  SINGLE SHOT MODE
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(4);

  adcVoltageHelper1 = new ADS1115Helper(4.096, &_adcVoltageADS1115_1, 50, 500);

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    YBP.println("Voltage ADS115 #2 OK");
  else
    YBP.println("Voltage ADS115 #2 Not Found");

  _adcVoltageADS1115_2.setMode(ADS1X15_MODE_SINGLE); //  SINGLE SHOT MODE
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(4);

  adcVoltageHelper2 = new ADS1115Helper(4.096, &_adcVoltageADS1115_2, 50, 500);

    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_VOLTAGE_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_VOLTAGE_ADC_SCK, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_CS);
  if (!_adcVoltageMCP3564.begin(0, 3.3)) {
    YBP.println("failed to initialize voltage MCP3564");
  } else {
    YBP.println("MCP3564 ok.");
  }

  _adcVoltageMCP3564.singleEndedMode();
  _adcVoltageMCP3564.setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcVoltageMCP3564.setAveraging(MCP3x6x::osr::OSR_1024);

  // _adcVoltageMCP3564.printConfig();

  // YBP.print("VDD: ");
  // YBP.println(_adcVoltageMCP3564.analogRead(MCP_AVDD));

  // YBP.print("TEMP: ");
  // YBP.println(_adcVoltageMCP3564.analogRead(MCP_TEMP));

  adcVoltageHelper = new MCP3564Helper(3.3, &_adcVoltageMCP3564);

    #endif

  #endif

  // the init here needs to be done in a specific way, otherwise it will hang or
  // get caught in a crash loop if the board finished a fade during the last
  // crash based on this issue: https://github.com/espressif/esp-idf/issues/5167

  // intitialize our channel
  for (auto& ch : pwm_channels) {
    ch.setup();
    ch.setupLedc();

    strlcpy(ch.source, local_hostname, sizeof(ch.source));
  }

  for (auto& ch : pwm_channels) {
    ch.setupOffset();
    ch.setupDefaultState();
  }

  // if (!INA.begin())
  //   YBP.println("INA226 could not connect.");
  // else
  //   YBP.println("INA226 OK");

  // YBP.print("MAN:\t");
  // YBP.println(INA.getManufacturerID(), HEX);
  // YBP.print("DIE:\t");
  // YBP.println(INA.getDieID(), HEX);

  // INA.setBusVoltageConversionTime(7);
  // INA.setShuntVoltageConversionTime(7);

  // int x = INA.setMaxCurrentShunt(20, 0.002);
  // YBP.println("normalized = true (default)");
  // YBP.println(x);

  // YBP.print("LSB:\t");
  // YBP.println(INA.getCurrentLSB(), 10);
  // YBP.print("LSB_uA:\t");
  // YBP.println(INA.getCurrentLSB_uA(), 3);
  // YBP.print("shunt:\t");
  // YBP.println(INA.getShunt(), 3);
  // YBP.print("maxCur:\t");
  // YBP.println(INA.getMaxCurrent(), 3);
  // YBP.println();
}

void pwm_channels_loop()
{
  // do we need to send an update?
  bool doSendFastUpdate = false;

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
  adcVoltageHelper1->onLoop();
  adcVoltageHelper2->onLoop();
    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
  adcVoltageHelper->onLoop();
    #endif
  #endif

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  adcCurrentHelper->onLoop();
  #endif

  // maintenance on our channels.
  for (auto& ch : pwm_channels) {
    ch.checkStatus();
    ch.saveThrottledDutyCycle();
    ch.checkIfFadeOver();

    ch.updateOutputLED();

    // flag for update?
    if (ch.sendFastUpdate) {
      doSendFastUpdate = true;

      // publish our home assistant state
      ch.haPublishState();
    }
  }

  // let the client know immediately.
  if (doSendFastUpdate) {
    sendFastUpdate();
  }

  // periodically update our HomeAssistant status
  unsigned int messageDelta = millis() - previousHAUpdateMillis;
  if (messageDelta >= 1000) {
    for (auto& ch : pwm_channels) {
      ch.haPublishAvailable();
      ch.haPublishState();

      previousHAUpdateMillis = millis();
    }
  }

  // if (millis() - previousINA226UpdateMillis > 1000) {
  //   float v = INA.getBusVoltage();
  //   float s = INA.getShuntVoltage_mV();
  //   float a = INA.getCurrent();
  //   float p = INA.getPower();

  //   YBP.printf("V: %.3fv | S: %.3fmV | A: %.3fA | P: %.3fW\n", v, s, a, p);

  //   previousINA226UpdateMillis = millis();
  // }
}

void PWMChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our duty cycle
  if (this->isDimmable) {
    sprintf(prefIndex, "pwmDuty%d", this->id);
    if (preferences.isKey(prefIndex))
      this->dutyCycle = preferences.getFloat(prefIndex);
    else
      this->dutyCycle = 1.0;
  } else
    this->dutyCycle = 1.0;
  this->lastDutyCycle = this->dutyCycle;

  // soft fuse trip count
  sprintf(prefIndex, "pwmTripCount%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseTripCount = preferences.getUInt(prefIndex);
  else
    this->softFuseTripCount = 0;

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  this->amperageHelper = adcCurrentHelper;
  _adcAmperageChannel = this->id - 1;
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
  if (this->id <= 4) {
    this->voltageHelper = adcVoltageHelper1;
    _adcVoltageChannel = this->id - 1;
  } else {
    this->voltageHelper = adcVoltageHelper2;
    _adcVoltageChannel = this->id - 5;
  }

    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
  this->voltageHelper = adcVoltageHelper;
  _adcVoltageChannel = this->id - 1;
    #endif
  #endif
}

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
  // float tv = 0;
  // for (byte i = 0; i < readings; i++)
  //   tv += this->voltageHelper->getNewVoltage(_adcVoltageChannel);
  // float v = this->toVoltage(tv / readings);

  // // low enough value to be an offset?
  // this->voltageOffset = 0.0;
  // if (v < (30.0 * 0.05))
  //   this->voltageOffset = v;

  // average a bunch of readings
  float ta = 0;
  for (byte i = 0; i < readings; i++)
    ta += this->amperageHelper->getNewVoltage(_adcAmperageChannel);
  float a = this->toAmperage(ta / readings);

  // amperage zero state
  this->amperageOffset = 0.0;
  if (a < (YB_PWM_CHANNEL_MAX_AMPS * 0.05))
    this->amperageOffset = a;

  // YBP.printf("CH%d Voltage Offset: %0.3f / Amperage Offset: %0.3f\n", this->id, this->voltageOffset, this->amperageOffset);
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

  // our first state change too
  this->lastStateChange = millis();

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
  // first of all, if its tripped or blown zero it out.
  if (this->status == Status::TRIPPED || this->status == Status::BLOWN) {
    this->outputState = false;
    this->writePWM(0);
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

    // follow our global brightness command
    duty = min(duty, globalBrightness);

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
  float amps = (voltage - (3.3 * 0.1)) / (YB_PWM_CHANNEL_V_PER_AMP);

  amps = amps - this->amperageOffset;

  // our floor is zero amps
  amps = max((float)0.0, amps);

  return amps;
}

float PWMChannel::getAmperage()
{
  return this->toAmperage(this->amperageHelper->getAverageVoltage(this->_adcAmperageChannel));
}

float PWMChannel::getWattage()
{
  if (this->dutyCycle == 1.0)
    return this->getVoltage() * this->getAmperage();
  else
    return getBusVoltage() * this->getAmperage();
}

float PWMChannel::toVoltage(float adcVoltage)
{
  float v = adcVoltage / (YB_PWM_CHANNEL_VOLTAGE_R2 / (YB_PWM_CHANNEL_VOLTAGE_R2 + YB_PWM_CHANNEL_VOLTAGE_R1));
  v = v - this->voltageOffset;

  // our floor is zero volts
  v = max((float)0.0, v);

  return v;
}

void PWMChannel::checkStatus()
{
  this->checkSoftFuse();
  this->checkFuseBlown();
  this->checkFuseBypassed();
}

void PWMChannel::updateOutputLED()
{
  #if (YB_STATUS_RGB_COUNT > 1)
  if (this->status == Status::ON)
    rgb_set_pixel_color(this->id, CRGB::Green);
  else if (this->status == Status::OFF)
    rgb_set_pixel_color(this->id, CRGB::Black);
  else if (this->status == Status::TRIPPED)
    rgb_set_pixel_color(this->id, CRGB::Yellow);
  else if (this->status == Status::BLOWN)
    rgb_set_pixel_color(this->id, CRGB::Red);
  else if (this->status == Status::BYPASSED)
    rgb_set_pixel_color(this->id, CRGB::Blue);
  else
    rgb_set_pixel_color(this->id, CRGB::Black);
  #endif
}

float PWMChannel::getVoltage()
{
  return this->toVoltage(this->voltageHelper->getAverageVoltage(_adcVoltageChannel));
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
  if (duty < 0.1)
    return;

  // determine what our floor for a tripped fuse should be.
  float busVoltage = getBusVoltage();
  float minVoltage = busVoltage * duty * 0.1;

  // so hard to measure that low accurately over time and not false
  if (minVoltage <= 0.05)
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
  if (getBusVoltage() > 0) {
    if (this->status == Status::ON) {
      // grace period when changing state
      if (millis() - lastStateChange > firstCheckTime) {

        // if our voltage is at a high enough level, we're fine.
        if (this->getVoltage() >= minVoltage)
          return;

        YBP.printf("CH%d BLOWN: %.3f < %.3f\n", this->id, this->getVoltage(), minVoltage);

        this->status = Status::BLOWN;
        this->outputState = false;
        this->updateOutput(false);
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

    // grace period when changing state
    if (millis() - lastStateChange > firstCheckTime) {

      if (this->getVoltage() < getBusVoltage() * 0.90)
        return;

      // dont change our outputState here... bypass can be temporary
      this->status = Status::BYPASSED;
    }
  } else if (this->status == Status::BYPASSED) {
    if (!this->outputState && this->getVoltage() < 2.0) {
      this->status = Status::OFF;
    }
  }
}

void PWMChannel::checkSoftFuse()
{
  // only trip once....
  if (this->status != Status::TRIPPED) {
    // Check our soft fuse, and our max limit for the board.
    if (abs(this->getAmperage()) >= this->softFuseAmperage ||
        abs(this->getAmperage()) >= YB_PWM_CHANNEL_MAX_AMPS) {

      // record some variables
      this->status = Status::TRIPPED;
      this->outputState = false;

      // actually shut it down!
      this->updateOutput(false);

      // our counter
      this->softFuseTripCount++;

      // we will want to notify
      this->sendFastUpdate = true;

      // this is an internally originating change
      strlcpy(this->source, local_hostname, sizeof(this->source));

      // save to our storage
      char prefIndex[YB_PREF_KEY_LENGTH];
      sprintf(prefIndex, "pwmTripCount%d", this->id);
      preferences.putUInt(prefIndex, this->softFuseTripCount);
    }
  }
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

void ARDUINO_ISR_ATTR PWMChannel::gammaISR(void* arg)
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
        preferences.putFloat(prefIndex, duty);
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
  if (this->getAmperage() > 0) {
    this->ampHours += this->getAmperage() * ((float)delta / 3600000.0);
    this->wattHours += this->getWattage() * ((float)delta / 3600000.0);
  }
}

void PWMChannel::setState(const char* state)
{
  if (!strcmp(state, "ON"))
    this->status = Status::ON;
  else if (!strcmp(state, "OFF"))
    this->status = Status::OFF;
  // these happen elsewhere in the code.
  // else if (!strcmp(state, "TRIPPED"))
  //   this->status = Status::TRIPPED;
  // else if (!strcmp(state, "BLOWN"))
  //   this->status = Status::BLOWN;
  // else if (!strcmp(state, "BYPASSED"))
  //   this->status = Status::BYPASSED;
  // else
  //   this->status = Status::OFF;

  if (!strcmp(state, "ON"))
    this->setState(true);
  else
    this->setState(false);
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
    this->amperageHelper->clearReadings(_adcAmperageChannel);
    this->voltageHelper->clearReadings(_adcVoltageChannel);

    // track when we last changed states.
    this->lastStateChange = millis();

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
  else
    return "OFF";
}

void PWMChannel::haGenerateDiscovery(JsonVariant doc)
{
  BaseChannel::haGenerateDiscovery(doc);

  // generate our topics
  sprintf(ha_topic_cmd_state, "yarrboard/%s/pwm/%s/ha_set", ha_key, this->key);
  sprintf(ha_topic_state_state, "yarrboard/%s/pwm/%s/ha_state", ha_key, this->key);
  sprintf(ha_topic_cmd_brightness, "yarrboard/%s/pwm/%s/ha_brightness/set", ha_key, this->key);
  sprintf(ha_topic_state_brightness, "yarrboard/%s/pwm/%s/ha_brightness/state", ha_key, this->key);
  sprintf(ha_topic_voltage, "yarrboard/%s/pwm/%s/voltage", ha_key, this->key);
  sprintf(ha_topic_current, "yarrboard/%s/pwm/%s/current", ha_key, this->key);

  // our callbacks to the command topics
  mqtt_on_topic(ha_topic_cmd_state, 0, pwm_handle_ha_command);
  mqtt_on_topic(ha_topic_cmd_brightness, 0, pwm_handle_ha_command);

  this->haGenerateLightDiscovery(doc);
  this->haGenerateVoltageDiscovery(doc);
  this->haGenerateAmperageDiscovery(doc);
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
  if (app_use_hostname_as_mqtt_uuid)
    sprintf(ha_uuid_voltage, "%s_voltage", local_hostname);
  else
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
  if (app_use_hostname_as_mqtt_uuid)
    sprintf(ha_uuid_amperage, "%s_amperage", local_hostname);
  else
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

void PWMChannel::haPublishState()
{
  if (this->status == Status::ON || this->status == Status::BYPASSED)
    mqtt_publish(ha_topic_state_state, "ON", false);
  else
    mqtt_publish(ha_topic_state_state, "OFF", false);

  if (this->isDimmable) {
    char b[8];
    int brightness = (int)roundf(255.0f * this->dutyCycle);
    if (brightness < 0)
      brightness = 0;
    if (brightness > 255)
      brightness = 255;
    snprintf(b, sizeof(b), "%u", brightness);

    mqtt_publish(ha_topic_state_state, b, false);
  }
}

void pwm_handle_ha_command(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  for (auto& ch : pwm_channels) {
    ch.haHandleCommand(topic, payload);
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
}

void PWMChannel::generateUpdate(JsonVariant config)
{
  BaseChannel::generateUpdate(config);

  config["state"] = this->getStatus();
  config["source"] = this->source;
  if (this->isDimmable)
    config["duty"] = round2(this->dutyCycle);
  config["voltage"] = round2(this->getVoltage());
  config["current"] = round2(this->getAmperage());
  config["wattage"] = round2(this->getWattage());
  config["aH"] = round3(this->ampHours);
  config["wH"] = round3(this->wattHours);
}

#endif