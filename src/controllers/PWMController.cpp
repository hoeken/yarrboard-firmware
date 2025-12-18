/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_PWM_CHANNELS

  #include "PWMController.h"
  #include <ConfigManager.h>
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>
  #include <controllers/ProtocolController.h>

PWMController* PWMController::_instance = nullptr;

PWMController::PWMController(YarrboardApp& app) : ChannelController(app, "pwm")
{
}

bool PWMController::setup()
{
  _app.protocol.registerCommand(GUEST, "set_pwm_channel", this, &PWMController::handleSetPWMChannel);
  _app.protocol.registerCommand(GUEST, "toggle_pwm_channel", this, &PWMController::handleTogglePWMChannel);
  _app.protocol.registerCommand(ADMIN, "config_pwm_channel", this, &PWMController::handleConfigPWMChannel);

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  _adcCurrentMCP3564 = new MCP3564(YB_PWM_CHANNEL_CURRENT_ADC_CS, &SPI, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_SCK);

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_CURRENT_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_CURRENT_ADC_SCK, YB_PWM_CHANNEL_CURRENT_ADC_MISO, YB_PWM_CHANNEL_CURRENT_ADC_MOSI, YB_PWM_CHANNEL_CURRENT_ADC_CS);
  if (!_adcCurrentMCP3564->begin(0, 3.3)) {
    YBP.println("failed to initialize current MCP3564");
  } else {
    YBP.println("MCP3564 ok.");
  }

  _adcCurrentMCP3564->singleEndedMode();
  _adcCurrentMCP3564->setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcCurrentMCP3564->setAveraging(MCP3x6x::osr::OSR_1024);

  adcCurrentHelper = new MCP3564Helper(3.3, _adcCurrentMCP3564, 50, 500);
  adcCurrentHelper->attachReadyPinInterrupt(YB_PWM_CHANNEL_CURRENT_ADC_IRQ, FALLING);
  #endif

  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
  _adcVoltageMCP3564 = new MCP3564(YB_PWM_CHANNEL_VOLTAGE_ADC_CS, &SPI, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_SCK);

  // turn on our pullup on the IRQ pin.
  pinMode(YB_PWM_CHANNEL_VOLTAGE_ADC_IRQ, INPUT_PULLUP);

  // start up our SPI and ADC objects
  SPI.begin(YB_PWM_CHANNEL_VOLTAGE_ADC_SCK, YB_PWM_CHANNEL_VOLTAGE_ADC_MISO, YB_PWM_CHANNEL_VOLTAGE_ADC_MOSI, YB_PWM_CHANNEL_VOLTAGE_ADC_CS);
  if (!_adcVoltageMCP3564->begin(0, 3.3)) {
    YBP.println("failed to initialize voltage MCP3564");
  } else {
    YBP.println("MCP3564 ok.");
  }

  _adcVoltageMCP3564->singleEndedMode();
  _adcVoltageMCP3564->setConversionMode(MCP3x6x::conv_mode::ONESHOT_STANDBY);
  _adcVoltageMCP3564->setAveraging(MCP3x6x::osr::OSR_1024);

  adcVoltageHelper = new MCP3564Helper(3.3, _adcVoltageMCP3564);
  adcVoltageHelper->attachReadyPinInterrupt(YB_PWM_CHANNEL_VOLTAGE_ADC_IRQ, FALLING);
  #endif

  // intitialize our channel
  for (auto& ch : _channels) {

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
    ch.amperageHelper = adcCurrentHelper;
    ch._adcAmperageChannel = ch.id - 1;
  #endif

  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
    ch.voltageHelper = adcVoltageHelper;
    ch._adcVoltageChannel = ch.id - 1;
  #endif

    // pass on our pointers
    ch._cfg = &_cfg;
    ch.busVoltage = busVoltage;
    ch.rgb = rgb;
    ch.mqtt = mqtt;

    ch.setup();
    ch.setupLedc();

    strlcpy(ch.source, _cfg.local_hostname, sizeof(ch.source));
  }

  for (auto& ch : _channels) {
    // ch.setupOffset();
    ch.setupDefaultState();
  }

  #ifdef YB_PWM_CHANNEL_ENABLE_PIN
  pinMode(YB_PWM_CHANNEL_ENABLE_PIN, OUTPUT);
  digitalWrite(YB_PWM_CHANNEL_ENABLE_PIN, LOW);
  #endif

  return true;
}

void PWMController::loop()
{
  // do we need to send an update?
  bool doSendFastUpdate = false;

  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564
  adcVoltageHelper->onLoop();
  #endif

  #ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
  adcCurrentHelper->onLoop();
  #endif

  // maintenance on our channels.
  for (auto& ch : _channels) {
  #ifdef YB_PWM_CHANNEL_HAS_INA226
    ch.readINA226();
  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
    ch.readLM75();
  #endif

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
}

void PWMController::handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  if (_instance) {
    _instance->handleHACommandCallback(topic, payload, retain, qos, dup);
  }
}

void PWMController::handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  for (auto& ch : _channels) {
    ch.haHandleCommand(topic, payload);
  }
}

float PWMController::getAverageCurrent()
{
  float amps = 0;
  byte enabled_count = 0;

  for (auto& ch : _channels) {
    // only count enabled channels
    if (ch.isEnabled) {
      amps += ch.getAmperage();
      enabled_count++;
    }
  }

  return amps / enabled_count;
}

float PWMController::getMaxCurrent()
{
  float amps = 0;

  for (auto& ch : _channels) {
    // only count enabled channels
    if (ch.isEnabled) {
      amps = max(amps, ch.getAmperage());
    }
  }

  return amps;
}

void PWMController::handleSetPWMChannel(JsonVariantConst input, JsonVariant output)
{
  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // is it enabled?
  if (!ch->isEnabled)
    return _app.protocol.generateErrorJSON(output, "Channel is not enabled.");

  // our duty cycle
  if (input["duty"].is<float>()) {
    // is it enabled?
    if (!ch->isEnabled)
      return _app.protocol.generateErrorJSON(output, "Channel is not enabled.");

    float duty = input["duty"];

    // what do we hate?  va-li-date!
    if (duty < 0)
      return _app.protocol.generateErrorJSON(output, "Duty cycle must be >= 0");
    else if (duty > 1)
      return _app.protocol.generateErrorJSON(output, "Duty cycle must be <= 1");

    // okay, we're good.
    ch->setDuty(duty);

    // change our output pin to reflect
    ch->updateOutput(true);
  }

  // change state
  if (input["state"].is<String>()) {
    // source is required
    if (!input["source"].is<String>())
      return _app.protocol.generateErrorJSON(output, "'source' is a required parameter");

    // check the length
    char error[50];
    if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
      sprintf(error, "Maximum source length is %s characters.", YB_HOSTNAME_LENGTH - 1);
      return _app.protocol.generateErrorJSON(output, error);
    }

    // get our data
    strlcpy(ch->source, input["source"] | _cfg.local_hostname, sizeof(ch->source));

    // okay, set our state
    char state[10];
    strlcpy(state, input["state"] | "OFF", sizeof(state));

    // update our pwm channel
    ch->setState(state);

    // TODO: trigger a fast update
    //  get that update out ASAP... if its our own update
    //  if (!strcmp(ch->source, local_hostname))
    //    sendFastUpdate();
  }
}

void PWMController::handleConfigPWMChannel(JsonVariantConst input, JsonVariant output)
{
  char error[128];

  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  if (!input["config"].is<JsonObjectConst>()) {
    snprintf(error, sizeof(error), "'config' is required parameter");
    return _app.protocol.generateErrorJSON(output, error);
  }

  if (!ch->loadConfig(input["config"], error, sizeof(error))) {
    return _app.protocol.generateErrorJSON(output, error);
  }

  // write it to file
  if (!_cfg.saveConfig(error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);
}

void PWMController::handleTogglePWMChannel(JsonVariantConst input, JsonVariant output)
{
  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // source is required
  if (!input["source"].is<String>())
    return _app.protocol.generateErrorJSON(output, "'source' is a required parameter");

  // check the length
  char error[50];
  if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
    sprintf(error, "Maximum source length is %s characters.", YB_HOSTNAME_LENGTH - 1);
    return _app.protocol.generateErrorJSON(output, error);
  }

  // save our source
  strlcpy(ch->source, input["source"] | _cfg.local_hostname, sizeof(ch->source));

  // these states should all change to off
  if (!strcmp(ch->getStatus(), "ON") || !strcmp(ch->getStatus(), "TRIPPED") || !strcmp(ch->getStatus(), "BLOWN"))
    ch->setState("OFF");
  // OFF and BYPASS can be turned on.
  else
    ch->setState("ON");
}

#endif