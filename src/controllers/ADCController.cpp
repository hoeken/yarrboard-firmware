/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "controllers/ADCController.h"
#include <ConfigManager.h>
#include <YarrboardApp.h>
#include <YarrboardDebug.h>
#include <controllers/ProtocolController.h>

#ifdef YB_HAS_ADC_CHANNELS

ADCController::ADCController(YarrboardApp& app) : ChannelController(app, "adc"),
                                                  _adcVoltageADS1115_1(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1),
                                                  _adcVoltageADS1115_2(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2)
{
}

bool ADCController::setup()
{
  _app.protocol.registerCommand(ADMIN, "config_adc", this, &ADCController::handleConfigADC);

  #if defined(YB_I2C_SDA_PIN) && defined(YB_I2C_SCL_PIN)
  Wire.begin(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
  #else
  Wire.begin(); // fallback to defaults
  #endif
  Wire.setClock(YB_I2C_SPEED);

  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    YBP.println("Voltage ADS115 #1 OK");
  else
    YBP.println("Voltage ADS115 #1 Not Found");

  // BASIC CONFIG
  _adcVoltageADS1115_1.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_1.setGain(YB_ADC_GAIN);
  _adcVoltageADS1115_1.setDataRate(4);

  adcHelper1 = new ADS1115Helper(YB_ADC_VREF, &_adcVoltageADS1115_1);
  adcHelper1->attachReadyPinInterrupt(YB_ADS1115_READY_PIN_1, FALLING);

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    YBP.println("Voltage ADS115 #2 OK");
  else
    YBP.println("Voltage ADS115 #2 Not Found");

  // BASIC CONFIG
  _adcVoltageADS1115_2.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_2.setGain(YB_ADC_GAIN);
  _adcVoltageADS1115_2.setDataRate(4);

  adcHelper2 = new ADS1115Helper(YB_ADC_VREF, &_adcVoltageADS1115_2);
  adcHelper2->attachReadyPinInterrupt(YB_ADS1115_READY_PIN_2, FALLING);

  // setup our channels
  for (auto& ch : _channels) {
    if (ch.id <= 4) {
      ch.adcHelper = adcHelper1;
      ch._adcChannel = ch.id - 1;
    } else {
      ch.adcHelper = adcHelper2;
      ch._adcChannel = ch.id - 5;
    }
    ch.setup();
  }

  return true;
}

void ADCController::loop()
{
  adcHelper1->onLoop();
  adcHelper2->onLoop();
}

void ADCController::handleConfigADC(JsonVariantConst input, JsonVariant output)
{
  char error[128] = "Unknown";

  // load our channel
  auto* ch = lookupChannel(input, output);
  if (!ch)
    return;

  // channel name
  if (input["name"].is<String>()) {
    // is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH - 1) {
      sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH - 1);
      return ProtocolController::generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(ch->name, input["name"] | "ADC ?", sizeof(ch->name));
  }

  // enabled
  if (input["enabled"].is<bool>()) {
    // save right nwo.
    bool enabled = input["enabled"];
    ch->isEnabled = enabled;
  }

  // channel type
  if (input["type"].is<String>()) {
    // is it too long?
    if (strlen(input["type"]) > YB_TYPE_LENGTH - 1) {
      sprintf(error, "Maximum channel type length is %s characters.", YB_CHANNEL_NAME_LENGTH - 1);
      return ProtocolController::generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(ch->type, input["type"] | "other", sizeof(ch->type));
  }

  // channel type
  if (input["display_decimals"].is<int>()) {
    int8_t display_decimals = input["display_decimals"];

    if (display_decimals < 0 || display_decimals > 4)
      return ProtocolController::generateErrorJSON(output, "Must be between 0 and 4");

    // change our channel.
    ch->displayDecimals = display_decimals;
  }

  // useCalibrationTable
  if (input["useCalibrationTable"].is<bool>()) {
    // save right nwo.
    bool enabled = input["useCalibrationTable"];
    ch->useCalibrationTable = enabled;
  }

  // are we using a calibration table?
  if (ch->useCalibrationTable) {
    // calibratedUnits
    if (input["calibratedUnits"].is<String>()) {
      // is it too long?
      if (strlen(input["calibratedUnits"]) > YB_ADC_UNIT_LENGTH - 1) {
        sprintf(error, "Maximum calibrated units length is %s characters.", YB_ADC_UNIT_LENGTH - 1);
        return ProtocolController::generateErrorJSON(output, error);
      }

      // save to our storage
      strlcpy(ch->calibratedUnits, input["calibratedUnits"] | "", sizeof(ch->calibratedUnits));
    }

    // calibratedUnits
    if (input["add_calibration"].is<JsonArrayConst>()) {
      JsonArrayConst pair = input["add_calibration"].as<JsonArrayConst>();
      if (pair.size() != 2)
        return ProtocolController::generateErrorJSON(output, "Each calibration entry must have exactly 2 elements [v, y]");

      float v = pair[0].as<float>();
      float y = pair[1].as<float>();

      // add it in
      if (!ch->addCalibrationValue({v, y}))
        return ProtocolController::generateErrorJSON(output, "Failed to add calibration entry");
    }

    // calibratedUnits
    if (input["remove_calibration"].is<JsonArrayConst>()) {
      JsonArrayConst pair = input["remove_calibration"].as<JsonArrayConst>();
      if (pair.size() != 2)
        return ProtocolController::generateErrorJSON(output, "Each calibration entry must have exactly 2 elements [v, y]");

      float v = pair[0].as<float>();
      float y = pair[1].as<float>();

      // Use std::isfinite(v) if <cmath> is available; otherwise isfinite(v) with <math.h>
      if (!std::isfinite(v) || !std::isfinite(y))
        return ProtocolController::generateErrorJSON(output, "Non-finite number in table");

      // remove any matching elements
      for (auto it = ch->calibrationTable.begin(); it != ch->calibrationTable.end();) {
        if (it->voltage == v && it->calibrated == y) {
          // erase returns the next valid iterator
          it = ch->calibrationTable.erase(it);
        } else {
          ++it;
        }
      }
    }
  }

  // write it to file
  if (!_cfg.saveConfig(error, sizeof(error)))
    return ProtocolController::generateErrorJSON(output, error);
}

#endif