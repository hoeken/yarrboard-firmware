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

byte ads1_channel = 0;
byte ads2_channel = 0;

// //  two interrupt flags
// volatile bool RDY_1 = false;
// volatile bool RDY_2 = false;

// //  catch interrupt and set flag device 1
// void IRAM_ATTR adsReady_1()
// {
//   RDY_1 = true;
// }

// //  catch interrupt and set flag device 2
// void IRAM_ATTR adsReady_2()
// {
//   RDY_2 = true;
// }

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
      // Wire.setClock(400000); // 400 kHz I2C to cut overhead
    #else
  Wire.begin(); // fallback to defaults
    #endif
  _adcVoltageADS1115_1.begin();
  if (_adcVoltageADS1115_1.isConnected())
    Serial.println("Voltage ADS115 #1 OK");
  else
    Serial.println("Voltage ADS115 #1 Not Found");

  // // //  SET ALERT RDY PIN
  // _adcVoltageADS1115_1.setComparatorPolarity(0); // 0 = active LOW
  // _adcVoltageADS1115_1.setComparatorLatch(0);    // 0 = non-latching
  // _adcVoltageADS1115_1.setComparatorMode(0);     // 0 = traditional
  // _adcVoltageADS1115_1.setComparatorThresholdLow(0x0000);
  // _adcVoltageADS1115_1.setComparatorThresholdHigh(0x8000);
  // _adcVoltageADS1115_1.setComparatorQueConvert(0);

  // //  SET INTERRUPT HANDLER TO CATCH CONVERSION READY
  // pinMode(YB_ADS1115_READY_PIN_1, INPUT);
  // attachInterrupt(digitalPinToInterrupt(YB_ADS1115_READY_PIN_1), adsReady_1, FALLING);

  // BASIC CONFIG
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(7);
  _adcVoltageADS1115_1.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_1.requestADC(0); //  0 == default channel,  trigger first read

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    Serial.println("Voltage ADS115 #2 OK");
  else
    Serial.println("Voltage ADS115 #2 Not Found");

  // //  SET ALERT RDY PIN
  // _adcVoltageADS1115_2.setComparatorPolarity(0); // 0 = active LOW
  // _adcVoltageADS1115_2.setComparatorLatch(0);    // 0 = non-latching
  // _adcVoltageADS1115_2.setComparatorMode(0);     // 0 = traditional
  // _adcVoltageADS1115_2.setComparatorThresholdLow(0x0000);
  // _adcVoltageADS1115_2.setComparatorThresholdHigh(0x8000);
  // _adcVoltageADS1115_2.setComparatorQueConvert(0);

  // //  SET INTERRUPT HANDLER TO CATCH CONVERSION READY
  // pinMode(YB_ADS1115_READY_PIN_2, INPUT);
  // attachInterrupt(digitalPinToInterrupt(YB_ADS1115_READY_PIN_2), adsReady_2, FALLING);

  // BASIC CONFIG
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(7);
  _adcVoltageADS1115_2.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_2.requestADC(0); //  0 == default channel,  trigger first read

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
  #ifdef YB_ADC_DRIVER_ADS1115

  if (!_adcVoltageADS1115_1.isBusy()) {
    adc_channels[ads1_channel].adcHelper->getReading();

    // next channel
    ads1_channel = (ads1_channel + 1) & 0x03; // 0..3

    adc_channels[ads1_channel].adcHelper->requestReading(ads1_channel);
  }

  if (!_adcVoltageADS1115_2.isBusy()) {
    adc_channels[ads2_channel + 4].adcHelper->getReading();

    // next channel
    ads2_channel = (ads2_channel + 1) & 0x03; // 0..3

    adc_channels[ads2_channel + 4].adcHelper->requestReading(ads2_channel);
  }

    // uint16_t value = 0;
    // if (adc_channels[current_ads1115_channel].adcHelper->isReady() && adc_channels[current_ads1115_channel + 4].adcHelper->isReady()) {
    //   adc_channels[current_ads1115_channel].update();
    //   adc_channels[current_ads1115_channel + 4].update();

    //   // update to our channel index
    //   current_ads1115_channel++;
    //   if (current_ads1115_channel == 4)
    //     current_ads1115_channel = 0;

    //   // request new conversion
    //   adc_channels[current_ads1115_channel].adcHelper->requestReading(current_ads1115_channel);
    //   adc_channels[current_ads1115_channel + 4].adcHelper->requestReading(current_ads1115_channel);
    // }

  #else
  // maintenance on our channels.
  for (byte id = 0; id < YB_ADC_CHANNEL_COUNT; id++)
    adc_channels[id].update();
  #endif
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