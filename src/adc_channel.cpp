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

  // BASIC CONFIG
  _adcVoltageADS1115_1.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_1.setGain(1);
  _adcVoltageADS1115_1.setDataRate(5);
  _adcVoltageADS1115_1.requestADC(0); // trigger first read

  _adcVoltageADS1115_2.begin();
  if (_adcVoltageADS1115_2.isConnected())
    Serial.println("Voltage ADS115 #2 OK");
  else
    Serial.println("Voltage ADS115 #2 Not Found");

  // BASIC CONFIG
  _adcVoltageADS1115_2.setMode(ADS1X15_MODE_SINGLE);
  _adcVoltageADS1115_2.setGain(1);
  _adcVoltageADS1115_2.setDataRate(5);
  _adcVoltageADS1115_2.requestADC(0); // trigger first read

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

  // what type are we?
  sprintf(prefIndex, "adcType%d", this->id);
  if (preferences.isKey(prefIndex))
    strlcpy(this->type, preferences.getString(prefIndex).c_str(), sizeof(this->type));
  else
    sprintf(this->type, "raw");

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

float ADCChannel::getTypeValue()
{
  if (!strcmp(this->type, "raw"))
    return this->getVoltage();
  else if (!strcmp(this->type, "digital_switch")) {
    if (this->getVoltage() >= YB_ADC_VCC * 0.7)
      return 1.0;
    else if (this->getVoltage() <= YB_ADC_VCC * 0.3)
      return 0.0;
    else
      return -1.0;
  } else if (!strcmp(this->type, "thermistor")) {
    // what pullup?
    float r_pullup = 10000.0;
    float r_beta = 3950.0;
    float r_thermistor = 10000.0;

    // 2. Calculate thermistor resistance
    // Voltage divider: Vadc = Vcc * (R_ntc / (R_ntc + R_PULLUP))
    float r_ntc = (this->getVoltage() * r_pullup) / (YB_ADS1115_VREF - this->getVoltage());

    // 3. Apply Beta equation to compute temperature in Kelvin
    float inv_T = (1.0 / 298.15) + (1.0 / r_beta) * log(r_ntc / r_thermistor);
    float tempK = 1.0 / inv_T;

    // 4. Convert to Celsius
    float tempC = tempK - 273.15;
    return tempC;
  } else if (!strcmp(this->type, "4-20ma"))
    return (this->getVoltage() / YB_SENDIT_420MA_R1) * 1000;
  else if (!strcmp(this->type, "tank_sensor")) {
    float amperage = (this->getVoltage() / YB_SENDIT_420MA_R1) * 1000;
    return map_generic(amperage, 4.0, 20.0, 0.0, 100.0);
  } else if (!strcmp(this->type, "high_volt_divider"))
    return this->getVoltage() * (YB_SENDIT_HIGH_DIVIDER_R1 + YB_SENDIT_HIGH_DIVIDER_R2) / YB_SENDIT_HIGH_DIVIDER_R2;
  else if (!strcmp(this->type, "low_volt_divider"))
    return this->getVoltage() * (YB_SENDIT_LOW_DIVIDER_R1 + YB_SENDIT_LOW_DIVIDER_R2) / YB_SENDIT_LOW_DIVIDER_R2;
  else if (!strcmp(this->type, "ten_k_pullup")) {
    float r1 = 10000.0;
    if (this->getVoltage() < 0)
      return -1;
    else if (this->getVoltage() >= YB_ADC_VCC * 0.999)
      return -2;
    else
      return (r1 * this->getVoltage()) / (YB_ADC_VCC - this->getVoltage());
  } else
    return -1;
}

const char* ADCChannel::getTypeUnits()
{
  if (!strcmp(this->type, "raw"))
    return "v";
  else if (!strcmp(this->type, "digital_switch"))
    return "bool";
  else if (!strcmp(this->type, "thermistor"))
    return "C";
  else if (!strcmp(this->type, "4-20ma"))
    return "mA";
  else if (!strcmp(this->type, "tank_sensor"))
    return "%";
  else if (!strcmp(this->type, "high_volt_divider"))
    return "v";
  else if (!strcmp(this->type, "low_volt_divider"))
    return "v";
  else if (!strcmp(this->type, "ten_k_pullup"))
    return "Ω";
  else
    return "";
}

#endif