/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"

ADCHelper::ADCHelper()
{
  this->vref = 3.3;
  this->resolution = 12;
}

ADCHelper::ADCHelper(float vref, uint8_t resolution)
{
  this->vref = vref;
  this->resolution = resolution;
}

bool ADCHelper::requestReading(uint8_t channel) { return false; }

bool ADCHelper::isReady() { return false; }

unsigned int ADCHelper::getReading() { return 0; }

void ADCHelper::addReading(unsigned int reading)
{
  this->cumulativeReadings += reading;
  this->readingCount++;
}

float ADCHelper::getVoltage() { return this->toVoltage(this->getReading()); }

unsigned int ADCHelper::getAverageReading()
{
  // Serial.printf("Reading Count: %d\n", this->readingCount);
  // Serial.printf("Cumulative Readings: %d\n", this->cumulativeReadings);

  if (this->readingCount > 0)
    return round((float)this->cumulativeReadings / (float)this->readingCount);
  else
    return 0;
}

float ADCHelper::getAverageVoltage()
{
  return this->toVoltage(this->getAverageReading());
}

void ADCHelper::resetAverage()
{
  this->readingCount = 0;
  this->cumulativeReadings = 0;
}

float ADCHelper::toVoltage(unsigned int reading)
{
  // DUMP(reading);
  // DUMP(this->vref);
  // DUMP(this->resolution);

  return (float)reading * this->vref / (float)(pow(2, this->resolution) - 1);
}

esp32Helper::esp32Helper() : ADCHelper::ADCHelper() {}
esp32Helper::esp32Helper(float vref, uint8_t channel)
    : ADCHelper::ADCHelper(vref, 12)
{
  this->channel = channel;
}

unsigned int esp32Helper::getReading()
{
  unsigned int reading = analogReadMilliVolts(this->channel);
  this->addReading(reading);

  return reading;
}

float esp32Helper::toVoltage(unsigned int reading)
{
  return (float)reading / 1000.0;
}

MCP3208Helper::MCP3208Helper() : ADCHelper::ADCHelper() {}
MCP3208Helper::MCP3208Helper(float vref, uint8_t channel, MCP3208* adc)
    : ADCHelper::ADCHelper(vref, 12)
{
  this->channel = channel;
  this->adc = adc;
}

unsigned int MCP3208Helper::getReading()
{
  unsigned int reading = this->adc->readADC(this->channel);
  this->addReading(reading);

  return reading;
}

MCP3564Helper::MCP3564Helper() : ADCHelper::ADCHelper() {}
MCP3564Helper::MCP3564Helper(float vref, uint8_t channel, MCP3564* adc)
    : ADCHelper::ADCHelper(vref, 24)
{
  this->channel = channel;
  this->adc = adc;
}

unsigned int MCP3564Helper::getReading()
{
  unsigned int reading;
  reading = this->adc->analogRead(_channelAddresses[this->channel]);
  this->addReading(reading);

  return reading;
}

float MCP3564Helper::toVoltage(unsigned int reading)
{
  return reading * this->adc->getReference() / (this->adc->getMaxValue() / 2);
}

MCP3425Helper::MCP3425Helper() : ADCHelper::ADCHelper() {}
MCP3425Helper::MCP3425Helper(float vref, MCP342x* adc)
    : ADCHelper::ADCHelper(vref, 15)
{
  this->adc = adc;
}

MCP342x::Config MCP3425_config(MCP342x::channel1, MCP342x::oneShot,
  MCP342x::resolution16, MCP342x::gain1);

void MCP3425Helper::setup()
{
  Wire.begin();
  MCP342x::generalCallReset();
  delay(1); // MC342x needs 300us to settle, wait 1ms

  this->adc->configure(MCP3425_config);

  this->start_conversion = true;
}

unsigned int MCP3425Helper::getReading()
{
  long value = 0;
  MCP342x::Config status;
  uint8_t err;

  // do we need to trigger a conversion?
  if (this->start_conversion) {
    err = this->adc->convert(MCP3425_config);
    if (err) {
      Serial.print("MCP3425 convert error: ");
      Serial.println(err);
    }
    this->start_conversion = false;
  }

  // okay, is it ready?
  err = this->adc->read(value, status);
  if (!err && status.isReady()) {
    // For debugging purposes print the return value.
    // Serial.print("Value: ");
    // Serial.println(value);

    this->addReading(value);

    this->start_conversion = true;
  } else {
    // Serial.print("ADC error: ");
    // Serial.println(err);

    value = 0;
  }

  return value;
}

ADS1115Helper::ADS1115Helper() : ADCHelper::ADCHelper() {}
ADS1115Helper::ADS1115Helper(float vref, uint8_t channel, ADS1115* adc)
    : ADCHelper::ADCHelper(vref, 15)
{
  this->channel = channel;
  this->adc = adc;
}

bool ADS1115Helper::requestReading(uint8_t channel)
{
  this->adc->requestADC(channel);
  return true;
}

bool ADS1115Helper::isReady() { return this->adc->isReady(); }

unsigned int ADS1115Helper::getReading()
{
  int16_t reading;
  reading = this->adc->getValue();

  // no negatives please.
  if (reading < 0)
    reading = 0;

  this->addReading(reading);

  return reading;
}

float ADS1115Helper::toVoltage(unsigned int reading)
{
  float factor = this->adc->toVoltage(1);
  float voltage = factor * reading;

  return voltage;
}