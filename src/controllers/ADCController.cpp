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
  _app.protocol.registerCommand(ADMIN, "config_adc_channel", this, &ADCController::handleConfigCommand);

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

void ADCController::handleConfigCommand(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  ChannelController::handleConfigCommand(input, output);
}

#endif