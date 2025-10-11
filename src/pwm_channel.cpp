/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_PWM_CHANNELS

  #include "pwm_channel.h"
  #include "rgb.h"

// the main star of the event
PWMChannel pwm_channels[YB_PWM_CHANNEL_COUNT];

// flag for hardware fade status
static volatile bool isChannelFading[YB_FAN_COUNT];

/* Setting PWM Properties */
// ledc library range is a little bit quirky:
// https://github.com/espressif/arduino-esp32/issues/5089
// const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION) -
// 1);
const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION));

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
MCP3564 _adcCurrentMCP3564(YB_PWM_CHANNEL_CURRENT_ADC_CS, &SPI, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_SCK);
  #elif YB_PWM_CHANNEL_ADC_DRIVER_MCP3208
MCP3208 _adcCurrentMCP3208;
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
ADS1115 _adcVoltageADS1115_1(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1);
ADS1115 _adcVoltageADS1115_2(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2);
    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
MCP3564 _adcVoltageMCP3564(YB_PWM_CHANNEL_VOLTAGE_ADC_CS, &SPI, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_SCK);
    #endif
  #endif

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static bool cb_ledc_fade_end_event(const ledc_cb_param_t* param,
  void* user_arg)
{
  portBASE_TYPE taskAwoken = pdFALSE;

  if (param->event == LEDC_FADE_END_EVT) {
    isChannelFading[(int)user_arg] = false;
  }

  return (taskAwoken == pdTRUE);
}

void mcp_wrapper()
{
  Serial.print(".");
  _adcCurrentMCP3564.IRQ_handler();
  _adcVoltageMCP3564.IRQ_handler();
}

void pwm_channels_setup()
{
  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_CURRENT_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_CURRENT_ADC_SCK, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_CS);
  if (!_adcCurrentMCP3564.begin(0, 3.3)) {
    Serial.println("failed to initialize current MCP3564");
  } else {
    Serial.println("MCP3564 ok.");
  }

  _adcCurrentMCP3564.singleEndedMode();
  _adcCurrentMCP3564.setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcCurrentMCP3564.setAveraging(MCP3x6x::osr::OSR_1024);

  _adcCurrentMCP3564.printConfig();

  Serial.print("VDD: ");
  Serial.println(_adcCurrentMCP3564.analogRead(MCP_AVDD));

  Serial.print("TEMP: ");
  Serial.println(_adcCurrentMCP3564.analogRead(MCP_TEMP));

  #elif YB_PWM_CHANNEL_ADC_DRIVER_MCP3208

  _adcCurrentMCP3208.begin(YB_PWM_CHANNEL_ADC_CS);
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115

  Wire.begin();
  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    Serial.println("Voltage ADS115 #1 OK");
  else
    Serial.println("Voltage ADS115 #1 Not Found");

  _adcVoltageADS1115_1.setMode(1); //  SINGLE SHOT MODE
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(7);

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    Serial.println("Voltage ADS115 #2 OK");
  else
    Serial.println("Voltage ADS115 #2 Not Found");

  _adcVoltageADS1115_2.setMode(1); //  SINGLE SHOT MODE
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(7);

    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_VOLTAGE_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_VOLTAGE_ADC_SCK, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_CS);
  if (!_adcVoltageMCP3564.begin(0, 3.3)) {
    Serial.println("failed to initialize voltage MCP3564");
  } else {
    Serial.println("MCP3564 ok.");
  }

  _adcVoltageMCP3564.singleEndedMode();
  _adcVoltageMCP3564.setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcVoltageMCP3564.setAveraging(MCP3x6x::osr::OSR_1024);

  _adcVoltageMCP3564.printConfig();

  Serial.print("VDD: ");
  Serial.println(_adcVoltageMCP3564.analogRead(MCP_AVDD));

  Serial.print("TEMP: ");
  Serial.println(_adcVoltageMCP3564.analogRead(MCP_TEMP));

    #elif YB_PWM_CHANNEL_ADC_DRIVER_MCP3208

  _adcCurrentMCP3208.begin(YB_PWM_CHANNEL_ADC_CS);
    #endif

  #endif

  // the init here needs to be done in a specific way, otherwise it will hang or
  // get caught in a crash loop if the board finished a fade during the last
  // crash based on this issue: https://github.com/espressif/esp-idf/issues/5167

  // intitialize our channel
  for (short i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    pwm_channels[i].id = i;
    pwm_channels[i].setup();
    pwm_channels[i].setupLedc();
  }

  // fade function
  ledc_fade_func_uninstall();
  ledc_fade_func_install(0);

  for (short i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    pwm_channels[i].setupInterrupt(); // intitialize our interrupts for fading
    pwm_channels[i].setupOffset();
    pwm_channels[i].setupDefaultState();
  }
}

void pwm_channels_loop()
{
  // do we need to send an update?
  bool doSendFastUpdate = false;

  // maintenance on our channels.
  for (byte id = 0; id < YB_PWM_CHANNEL_COUNT; id++) {
    pwm_channels[id].checkStatus();
    pwm_channels[id].saveThrottledDutyCycle();
    pwm_channels[id].checkIfFadeOver();

    pwm_channels[id].updateOutputLED();

    // flag for update?
    if (pwm_channels[id].sendFastUpdate)
      doSendFastUpdate = true;
  }

  // let the client know immediately.
  if (doSendFastUpdate) {
    TRACE();
    DUMP(pwm_channels[0].getStatus());
    sendFastUpdate();
  }
}

bool isValidPWMChannel(byte cid)
{
  if (cid < 0 || cid >= YB_PWM_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void PWMChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our name
  sprintf(prefIndex, "pwmName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "PWM #%d", this->id);

  // enabled or no
  sprintf(prefIndex, "pwmEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  // lookup our duty cycle
  sprintf(prefIndex, "pwmDuty%d", this->id);
  if (preferences.isKey(prefIndex))
    this->dutyCycle = preferences.getFloat(prefIndex);
  else
    this->dutyCycle = 1.0;
  this->lastDutyCycle = this->dutyCycle;

  // dimmability.
  sprintf(prefIndex, "pwmDimmable%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isDimmable = preferences.getBool(prefIndex);
  else
    this->isDimmable = true;

  // soft fuse
  sprintf(prefIndex, "pwmSoftFuse%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseAmperage = preferences.getFloat(prefIndex);
  else
    this->softFuseAmperage = YB_PWM_CHANNEL_MAX_AMPS;

  // soft fuse trip count
  sprintf(prefIndex, "pwmTripCount%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseTripCount = preferences.getUInt(prefIndex);
  else
    this->softFuseTripCount = 0;

  // type
  sprintf(prefIndex, "pwmType%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->type, preferences.getString(prefIndex).c_str(), sizeof(this->type));
  else
    sprintf(this->type, "other", this->id);

  // default state
  sprintf(prefIndex, "pwmDefault%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->defaultState, preferences.getString(prefIndex).c_str(), sizeof(this->defaultState));
  else
    sprintf(this->defaultState, "OFF", this->id);

  #ifdef YB_PWM_CHANNEL_ADC_DRIVER_MCP3564
  this->amperageHelper = new MCP3564Helper(3.3, this->id, &_adcCurrentMCP3564);
  #elif YB_PWM_CHANNEL_ADC_DRIVER_MCP3208
  this->amperageHelper = new MCP3208Helper(3.3, this->id, &_adcCurrentMCP3208);
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
  if (this->id < 4)
    this->voltageHelper = new ADS1115Helper(3.3, this->id, &_adcVoltageADS1115_1);
  else
    this->voltageHelper = new ADS1115Helper(3.3, this->id - 4, &_adcVoltageADS1115_2);
    #endif
  #endif
}

void PWMChannel::setupLedc()
{
  // deinitialize our pin.
  // ledc_fade_func_uninstall();
  ledcDetachPin(this->_pins[this->id]);

  // initialize our PWM channels
  ledcSetup(this->id, YB_PWM_CHANNEL_FREQUENCY, YB_PWM_CHANNEL_RESOLUTION);
  ledcAttachPin(this->_pins[this->id], this->id);
  ledcWrite(this->id, 0);
}

void PWMChannel::setupInterrupt()
{
  int channel = this->id;
  isChannelFading[this->id] = false;

  ledc_cbs_t callbacks = {.fade_cb = cb_ledc_fade_end_event};

  // this is our callback handler for fade end.
  ledc_cb_register(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, &callbacks, (void*)channel);
}

void PWMChannel::setupOffset()
{
  // make sure we're off for testing.
  this->outputState = false;
  this->status = Status::OFF;
  this->updateOutput(false);

  Serial.print("s");

  // voltage zero state
  this->voltageOffset = 0.0;
  float v = this->getVoltage();
  if (v < (30.0 * 0.05))
    this->voltageOffset = v;
  Serial.print("v");

  // amperage zero state
  this->amperageOffset = 0.0;
  float a = this->getAmperage();
  if (a < (20.0 * 0.05))
    this->amperageOffset = a;
  Serial.print("a");

  Serial.printf("CH%d Voltage Offset: %0.3f / Amperage Offset: %0.3f\n", this->id, this->voltageOffset, this->amperageOffset);
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

  // update our pin
  this->updateOutput(true);
}

void PWMChannel::saveThrottledDutyCycle()
{
  // after 5 secs of no activity, we can save it.
  if (this->dutyCycleIsThrottled &&
      millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT)
    this->setDuty(this->dutyCycle);
}

void PWMChannel::updateOutput(bool check_status)
{
  // what PWM do we want?
  int pwm = 0;
  if (this->isDimmable) {
    // also adjust for global brightness
    // TODO: we should add an "output_type" and check if its an LED here?
    // if (this->type == "light")
    float duty = min(this->dutyCycle, globalBrightness);

    // okay now get our actual duty
    pwm = duty * MAX_DUTY_CYCLE;
  } else
    pwm = MAX_DUTY_CYCLE;

  // if its off, zero it out
  if (!this->outputState)
    pwm = 0;

  // if its tripped, zero it out.
  if (this->status == Status::TRIPPED || this->status == Status::BLOWN) {
    this->outputState = false;
    pwm = 0;
  }

  // okay, set our pin state.
  if (!isChannelFading[this->id])
    ledcWrite(this->id, pwm);

  // see what we're working with.
  if (check_status)
    this->checkStatus();
}

float PWMChannel::toAmperage(float voltage)
{
  float amps = (voltage - (3.3 * 0.1)) / (0.132); // ACS725LLCTR-20AU

  amps = amps - this->amperageOffset;

  // our floor is zero amps
  amps = max((float)0.0, amps);

  return amps;
}

float PWMChannel::getAmperage()
{
  return this->toAmperage(
    this->amperageHelper->toVoltage(this->amperageHelper->getReading()));
}

void PWMChannel::checkAmperage()
{
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
  this->voltage = this->getVoltage();
  this->amperage = this->getAmperage();

  this->checkSoftFuse();
  this->checkFuseBlown();
  this->checkFuseBypassed();
}

void PWMChannel::updateOutputLED()
{
  #if (YB_STATUS_WS2818_COUNT > 1)
  if (this->status == Status::ON)
    rgb_set_pixel_color(this->id + 1, 0, 255, 0); // green
  else if (this->status == Status::OFF)
    rgb_set_pixel_color(this->id + 1, 0, 0, 0); // off
  else if (this->status == Status::TRIPPED)
    rgb_set_pixel_color(this->id + 1, 255, 255, 0); // yellow
  else if (this->status == Status::BLOWN)
    rgb_set_pixel_color(this->id + 1, 255, 0, 0); // red
  else if (this->status == Status::BYPASSED)
    rgb_set_pixel_color(this->id + 1, 0, 0, 255); // blue
  else
    rgb_set_pixel_color(this->id + 1, 0, 0, 0); // off
  #endif
}

float PWMChannel::getVoltage()
{
  float voltage = this->toVoltage(this->voltageHelper->toVoltage(this->voltageHelper->getReading()));
  return voltage;
}

void PWMChannel::checkVoltage()
{
}

void PWMChannel::checkFuseBlown()
{
  if (this->status == Status::ON) {
    // try to "debounce" the on state
    for (byte i = 0; i < 10; i++) {
      if (i > 0) {
        DUMP(i);
        DUMP(this->voltage);
      }
      if (this->voltage > 8.0)
        return;

      delay(1);
      this->voltage = this->getVoltage();
    }

    TRACE();
    DUMP(this->voltage);
    this->status = Status::BLOWN;
    this->outputState = false;
    this->updateOutput(false);
    // } else if (this->status == Status::BLOWN) {
    //   if (this->voltage > 8.0) {
    //     this->status = Status::OFF;
    //     this->outputState = false;
    //     this->updateOutput(false);
    //   }
  }
}

void PWMChannel::checkFuseBypassed()
{
  if (this->status != Status::ON && this->status != Status::BYPASSED) {
    for (byte i = 0; i < 10; i++) {
      // if (i > 0) {
      //   DUMP(i);
      //   DUMP(this->voltage);
      // }

      // voltage needs to be over 90%... otherwise we get false readings when shutting off motors as the voltage collapses
      if (this->voltage < busVoltage * 0.90)
        return;

      delay(1);
      this->voltage = this->getVoltage();
    }

    // dont change our outputState here... bypass can be temporary
    this->status = Status::BYPASSED;
  } else if (this->status == Status::BYPASSED) {
    if (!this->outputState && this->voltage < 2.0) {
      this->status = Status::OFF;
    }
  }
}

void PWMChannel::checkSoftFuse()
{
  // only trip once....
  if (this->status != Status::TRIPPED) {
    // Check our soft fuse, and our max limit for the board.
    if (abs(this->amperage) >= this->softFuseAmperage ||
        abs(this->amperage) >= YB_PWM_CHANNEL_MAX_AMPS) {
      DUMP("TRIPPED");
      DUMP(this->amperage);

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

void PWMChannel::setFade(float duty, int max_fade_time_ms)
{
  // is our earlier hardware fade over yet?
  if (!isChannelFading[this->id]) {
    // dutyCycle is a default - will be non-zero when state is off
    if (this->outputState)
      this->fadeDutyCycleStart = this->dutyCycle;
    else
      this->fadeDutyCycleStart = 0.0;

    TRACE();

    // fading turns on the channel.
    this->outputState = true;
    this->status = Status::ON;

    // some vars for tracking.
    isChannelFading[this->id] = true;
    this->fadeRequested = true;

    this->fadeDutyCycleEnd = duty;
    this->fadeStartTime = millis();
    this->fadeDuration = max_fade_time_ms + 100;

    // call our hardware fader
    int target_duty = duty * MAX_DUTY_CYCLE;
    ledc_set_fade_time_and_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)this->id, target_duty, max_fade_time_ms, LEDC_FADE_NO_WAIT);
  }
}

void PWMChannel::checkIfFadeOver()
{
  // we're looking to see if the fade is over yet
  if (this->fadeRequested) {
    // has our fade ended?
    if (millis() - this->fadeStartTime >= this->fadeDuration) {
      // this is a potential bug fix.. had the board "lock" into a fade.
      // it was responsive but wouldnt toggle some pins.  I think it was this
      // flag not getting cleared
      if (isChannelFading[this->id]) {
        isChannelFading[this->id];
        Serial.println("error fading");
      }

      this->fadeRequested = false;

      // ignore the zero duty cycle part
      if (fadeDutyCycleEnd > 0)
        this->setDuty(fadeDutyCycleEnd);
      else
        this->dutyCycle = fadeDutyCycleStart;

      if (fadeDutyCycleEnd == 0) {
        this->outputState = false;
        this->status = Status::OFF;
      } else {
        this->outputState = true;
        this->status = Status::ON;
        TRACE();
      }
    }
    // okay, update our duty cycle as we go for good UI
    else {
      unsigned long nowDelta = millis() - this->fadeStartTime;
      float dutyDelta = this->fadeDutyCycleEnd - this->fadeDutyCycleStart;

      if (this->fadeDuration > 0) {
        float currentDuty =
          this->fadeDutyCycleStart +
          ((float)nowDelta / (float)this->fadeDuration) * dutyDelta;
        this->dutyCycle = currentDuty;
      }

      if (this->dutyCycle == 0) {
        this->outputState = false;
        this->status = Status::OFF;
      } else {
        this->outputState = true;
        this->status = Status::ON;
        TRACE();
      }
    }
  }
}

void PWMChannel::setDuty(float duty)
{
  this->dutyCycle = duty;

  // it only makes sense to change it to non-zero.
  if (this->dutyCycle > 0) {
    if (this->dutyCycle != this->lastDutyCycle) {
      // we don't want to swamp the flash
      if (millis() - this->lastDutyCycleUpdate > YB_DUTY_SAVE_TIMEOUT) {
        char prefIndex[YB_PREF_KEY_LENGTH];
        sprintf(prefIndex, "pwmDuty%d", this->id);
        preferences.putFloat(prefIndex, duty);
        this->dutyCycleIsThrottled = false;
        Serial.printf("saving %s: %f\n", prefIndex, this->dutyCycle);

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
  // this->voltage = this->toVoltage(this->voltageHelper->getAverageVoltage());
  // this->voltageHelper->resetAverage();
  // this->amperage = this->toAmperage(this->amperageHelper->getAverageVoltage());
  // this->amperageHelper->resetAverage();

  // record our total consumption
  if (this->amperage > 0) {
    this->ampHours += this->amperage * ((float)delta / 3600000.0);

    // only use our voltage if we're not pwming.
    if (this->voltage && this->dutyCycle == 1.0)
      this->wattHours += this->amperage * this->voltage * ((float)delta / 3600000.0);
    else
      this->wattHours += this->amperage * busVoltage * ((float)delta / 3600000.0);
  }
}

void PWMChannel::setState(const char* state)
{
  DUMP(state);

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
    DUMP(this->getStatus());
    DUMP(newState);

    // save our output state
    this->outputState = newState;

    // keep track of how many toggles
    this->stateChangeCount++;

    // what is our new status?
    if (newState)
      this->status = Status::ON;
    else
      this->status = Status::OFF;

    // change our output pin to reflect
    this->updateOutput(true);

    // flag for update to clients
    this->sendFastUpdate = true;
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
  else
    return "OFF";
}

#endif