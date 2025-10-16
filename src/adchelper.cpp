/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"

ADCHelper::ADCHelper() : vref(3.3), resolution(12), runningAverage(10)
{
}

ADCHelper::ADCHelper(float vref, uint8_t resolution) : vref(vref), resolution(resolution), runningAverage(10)
{
}

bool ADCHelper::requestReading(uint8_t channel) { return false; }

bool ADCHelper::isReady() { return false; }

unsigned int ADCHelper::getReading() { return 0; }

void ADCHelper::addReading(unsigned int reading)
{
  this->runningAverage.add(reading);
}

float ADCHelper::getVoltage() { return this->toVoltage(this->getReading()); }

float ADCHelper::getAverageReading()
{
  // Serial.printf("Reading Count: %d\n", this->readingCount);
  // Serial.printf("Cumulative Readings: %d\n", this->cumulativeReadings);

  return this->runningAverage.getAverage();
}

float ADCHelper::getAverageVoltage()
{
  return this->toVoltage(this->getAverageReading());
}

void ADCHelper::resetAverage()
{
  this->runningAverage.clear();
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

MCP3425Helper::MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc)
    : ADCHelper::ADCHelper(vref, 15), adc(adc), config(config)
{
}

void MCP3425Helper::setup()
{
  Wire.begin();
  this->start_conversion = true;
}

unsigned int MCP3425Helper::getReading()
{
  long value = 0;
  MCP342x::Config status;
  uint8_t err;

  // do we need to trigger a conversion?
  if (this->start_conversion) {

    err = this->adc->convert(this->config);
    if (err) {
      Serial.printf("MCP3425 convert error: %d\n", err);
    }
    this->start_conversion = false;
    return 0;
  }

  // okay, is it ready?
  err = this->adc->read(value, status);
  if (!err && status.isReady()) {
    this->addReading(value);
    this->start_conversion = true;
  } else {
    if (err != 4)
      Serial.printf("MCP3425 read error: %d\n", err);

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