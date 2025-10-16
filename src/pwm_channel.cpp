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

unsigned long previousHAUpdateMillis = 0;

// the main star of the event
etl::array<PWMChannel, YB_PWM_CHANNEL_COUNT> pwm_channels;

// our channel pins
byte _pins[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_PINS;

/* Setting PWM Properties */
// ledc library range is a little bit quirky:
// https://github.com/espressif/arduino-esp32/issues/5089
// const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION) -
// 1);
const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION));

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
MCP3564 _adcCurrentMCP3564(YB_PWM_CHANNEL_CURRENT_ADC_CS, &SPI, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_SCK);
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
ADS1115 _adcVoltageADS1115_1(YB_PWM_CHANNEL_VOLTAGE_I2C_ADDRESS_1);
ADS1115 _adcVoltageADS1115_2(YB_PWM_CHANNEL_VOLTAGE_I2C_ADDRESS_2);
    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
MCP3564 _adcVoltageMCP3564(YB_PWM_CHANNEL_VOLTAGE_ADC_CS, &SPI, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_SCK);
    #endif
  #endif

void mcp_wrapper()
{
  Serial.print(".");

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  _adcCurrentMCP3564.IRQ_handler();
  #endif

  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
  _adcVoltageMCP3564.IRQ_handler();
  #endif
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
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115

  Wire.begin();
  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    Serial.println("Voltage ADS115 #1 OK");
  else
    Serial.println("Voltage ADS115 #1 Not Found");

  _adcVoltageADS1115_1.setMode(1); //  SINGLE SHOT MODE
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(7);
  _adcVoltageADS1115_1.requestADC(0); // trigger first read

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    Serial.println("Voltage ADS115 #2 OK");
  else
    Serial.println("Voltage ADS115 #2 Not Found");

  _adcVoltageADS1115_2.setMode(1); //  SINGLE SHOT MODE
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(7);
  _adcVoltageADS1115_2.requestADC(0); // trigger first read

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
}

void pwm_channels_loop()
{
  // do we need to send an update?
  bool doSendFastUpdate = false;

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
}

void PWMChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our duty cycle
  sprintf(prefIndex, "pwmDuty%d", this->id);
  if (preferences.isKey(prefIndex))
    this->dutyCycle = preferences.getFloat(prefIndex);
  else
    this->dutyCycle = 1.0;
  this->lastDutyCycle = this->dutyCycle;

  // soft fuse trip count
  sprintf(prefIndex, "pwmTripCount%d", this->id);
  if (preferences.isKey(prefIndex))
    this->softFuseTripCount = preferences.getUInt(prefIndex);
  else
    this->softFuseTripCount = 0;

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  this->amperageHelper = new MCP3564Helper(3.3, this->id - 1, &_adcCurrentMCP3564);
  #endif

  #ifdef YB_HAS_CHANNEL_VOLTAGE
    #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
  if (this->id <= 4)
    this->voltageHelper = new ADS1115Helper(3.3, this->id - 1, &_adcVoltageADS1115_1);
  else
    this->voltageHelper = new ADS1115Helper(3.3, this->id - 5, &_adcVoltageADS1115_2);
    #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
  this->voltageHelper = new MCP3564Helper(3.3, this->id - 1, &_adcVoltageMCP3564);
    #endif
  #endif
}

void PWMChannel::setupLedc()
{
  // track our fades
  this->isFading = false;

  // deinitialize our pin.
  ledcDetach(this->pin);

  // initialize our PWM channels
  ledcAttach(this->pin, YB_PWM_CHANNEL_FREQUENCY, YB_PWM_CHANNEL_RESOLUTION);
  ledcWrite(this->pin, 0);
}

void PWMChannel::setupOffset()
{
  // make sure we're off for testing.
  this->outputState = false;
  this->status = Status::OFF;
  this->updateOutput(false);

  // voltage zero state
  this->voltageOffset = 0.0;
  float v = this->getVoltage();
  if (v < (30.0 * 0.05))
    this->voltageOffset = v;

  // amperage zero state
  this->amperageOffset = 0.0;
  float a = this->getAmperage();
  if (a < (20.0 * 0.05))
    this->amperageOffset = a;

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
  if (!this->isFading) {
    ledcWrite(this->pin, pwm);
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
    rgb_set_pixel_color(this->id, 0, 255, 0); // green
  else if (this->status == Status::OFF)
    rgb_set_pixel_color(this->id, 0, 0, 0); // off
  else if (this->status == Status::TRIPPED)
    rgb_set_pixel_color(this->id, 255, 255, 0); // yellow
  else if (this->status == Status::BLOWN)
    rgb_set_pixel_color(this->id, 255, 0, 0); // red
  else if (this->status == Status::BYPASSED)
    rgb_set_pixel_color(this->id, 0, 0, 255); // blue
  else
    rgb_set_pixel_color(this->id, 0, 0, 0); // off
  #endif
}

float PWMChannel::getVoltage()
{
  // queue up our voltage reading.
  if (this->id <= 4)
    this->voltageHelper->requestReading(this->id - 1);
  else
    this->voltageHelper->requestReading(this->id - 5);

  // get our latest voltage reading.
  while (!this->voltageHelper->isReady())
    vTaskDelay(1);

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
    for (byte i = 0; i < 25; i++) {
      if (this->voltage > 8.0)
        return;

      vTaskDelay(1);
      this->voltage = this->getVoltage();
    }

    this->status = Status::BLOWN;
    this->outputState = false;
    this->updateOutput(false);
  }
}

void PWMChannel::checkFuseBypassed()
{
  if (this->status != Status::ON && this->status != Status::BYPASSED) {
    for (byte i = 0; i < 10; i++) {

      // voltage needs to be over 90%... otherwise we get false readings when shutting off motors as the voltage collapses
      if (this->voltage < busVoltage * 0.90)
        return;

      vTaskDelay(1);
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
  if (!this->isFading) {
    // dutyCycle is a default - will be non-zero when state is off
    if (this->outputState)
      this->fadeDutyCycleStart = this->dutyCycle;
    else
      this->fadeDutyCycleStart = 0.0;

    // fading turns on the channel.
    this->outputState = true;
    this->status = Status::ON;

    // some vars for tracking.
    this->isFading = true;

    this->fadeDutyCycleEnd = duty;
    this->fadeStartTime = millis();
    this->fadeDuration = max_fade_time_ms + 100;

    // call our hardware fader
    const int ch = this->id - 1;
    int target_duty = duty * MAX_DUTY_CYCLE;
    const uint32_t start_duty = ledcRead(this->pin); // current duty counts

    // kicks off fade and registers our ISR + arg (the channel index)
    bool ok = ledcFadeWithInterruptArg(
      this->pin,
      start_duty,
      target_duty,
      max_fade_time_ms,
      &PWMChannel::onFadeISR,
      this);

    // if it failed, clear flag and handle your error path (log, retry, etc.)
    if (!ok) {
      this->isFading = false;
    }
  }
}

void PWMChannel::checkIfFadeOver()
{
  // has our fade ended?
  if (this->isFading) {
    if (millis() - this->fadeStartTime >= this->fadeDuration) {
      // this is a potential bug fix.. had the board "lock" into a fade.
      // it was responsive but wouldnt toggle some pins.  I think it was this
      // flag not getting cleared
      if (this->isFading) {
        this->isFading = false;
        Serial.println("error fading");
      }

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

void PWMChannel::haGenerateDiscovery(JsonVariant doc)
{
  // generate our id / topics
  sprintf(ha_uuid, "%s_pwm_%s", uuid, this->key);
  sprintf(ha_topic_cmd_state, "yarrboard/%s/pwm/%s/ha_set", local_hostname, this->key);
  sprintf(ha_topic_state_state, "yarrboard/%s/pwm/%s/ha_state", local_hostname, this->key);
  sprintf(ha_topic_avail, "yarrboard/%s/pwm/%s/ha_availability", local_hostname, this->key);
  sprintf(ha_topic_cmd_brightness, "yarrboard/%s/pwm/%s/ha_brightness/set", local_hostname, this->key);
  sprintf(ha_topic_state_brightness, "yarrboard/%s/pwm/%s/ha_brightness/state", local_hostname, this->key);
  sprintf(ha_topic_voltage, "yarrboard/%s/pwm/%s/voltage", local_hostname, this->key);
  sprintf(ha_topic_current, "yarrboard/%s/pwm/%s/current", local_hostname, this->key);

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
  obj["p"] = "light";
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

void PWMChannel::haPublishAvailable()
{
  mqtt_publish(ha_topic_avail, "online", false);
}

void PWMChannel::haPublishState()
{
  if (this->status == Status::ON)
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

  this->pin = _pins[id - 1]; // pins are zero indexed, we are 1 indexed

  snprintf(this->name, sizeof(this->name), "PWM Channel %d", id);
}

bool PWMChannel::loadConfig(JsonVariantConst config, char* error, size_t len)
{
  // make our parent do the work.
  if (!BaseChannel::loadConfig(config, error, len))
    return false;

  isDimmable = config["isDimmable"] | true;

  softFuseAmperage = config["softFuse"] | YB_PWM_CHANNEL_MAX_AMPS;
  softFuseAmperage = max(0.0f, softFuseAmperage);
  softFuseAmperage = min((float)YB_PWM_CHANNEL_MAX_AMPS, softFuseAmperage);

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
  config["voltage"] = round2(this->voltage);
  config["current"] = round2(this->amperage);
  config["aH"] = round3(this->ampHours);
  config["wH"] = round3(this->wattHours);
}

#endif