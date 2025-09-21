/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_HAS_ADC_CHANNELS

  #include "adc_channel.h"

// the main star of the event
ADCChannel adc_channels[YB_ADC_CHANNEL_COUNT];

  #ifdef YB_ADC_DRIVER_ADS1115
ADS1115 _adcVoltageADS1115_1(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_1);
ADS1115 _adcVoltageADS1115_2(YB_CHANNEL_VOLTAGE_I2C_ADDRESS_2);
  #elif YB_ADC_DRIVER_MCP3208
MCP3208 _adcAnalogMCP3208;
  #endif

void adc_channels_setup()
{
  #ifdef YB_ADC_DRIVER_ADS1115
    #if defined(YB_I2C_SDA_PIN) && defined(YB_I2C_SCL_PIN)
  Wire.begin(YB_I2C_SDA_PIN, YB_I2C_SCL_PIN);
    #else
  Wire.begin(); // fallback to defaults
    #endif
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

  #elif YB_ADC_DRIVER_MCP3208
  _adcAnalogMCP3208.begin(YB_ADC_CS);
    //_adcAnalogMCP3208.begin(YB_ADC_CS, 23, 19, 18);
  #endif

  // intitialize our channel
  for (short i = 0; i < YB_ADC_CHANNEL_COUNT; i++) {
    adc_channels[i].id = i;
    adc_channels[i].setup();
  }
}

void adc_channels_loop()
{
  // maintenance on our channels.
  for (byte id = 0; id < YB_ADC_CHANNEL_COUNT; id++)
    adc_channels[id].update();
}

bool isValidADCChannel(byte cid)
{
  if (cid < 0 || cid >= YB_ADC_CHANNEL_COUNT)
    return false;
  else
    return true;
}

void ADCChannel::setup()
{
  char prefIndex[YB_PREF_KEY_LENGTH];

  // lookup our name
  sprintf(prefIndex, "adcName%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->name, preferences.getString(prefIndex).c_str(), sizeof(this->name));
  else
    sprintf(this->name, "ADC #%d", this->id);

  // enabled or no
  sprintf(prefIndex, "adcEnabled%d", this->id);
  if (preferences.isKey(prefIndex))
    this->isEnabled = preferences.getBool(prefIndex);
  else
    this->isEnabled = true;

  #ifdef YB_ADC_DRIVER_ADS1115
  if (this->id < 4)
    this->adcHelper = new ADS1115Helper(YB_ADS1115_VREF, this->id, &_adcVoltageADS1115_1);
  else
    this->adcHelper = new ADS1115Helper(YB_ADS1115_VREF, this->id - 4, &_adcVoltageADS1115_2);
  #elif YB_ADC_DRIVER_MCP3208
  this->adcHelper = new MCP3208Helper(3.3, this->id, &_adcAnalogMCP3208);
  #endif
}

void ADCChannel::update()
{
  this->adcHelper->getReading();
}

unsigned int ADCChannel::getReading()
{
  return this->adcHelper->getAverageReading();
}

float ADCChannel::getVoltage()
{
  return this->adcHelper->getAverageVoltage();
}

void ADCChannel::resetAverage()
{
  this->adcHelper->resetAverage();
}

#endif