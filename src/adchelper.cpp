/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"

ADCHelper::ADCHelper(uint8_t channels, float vref, uint8_t resolution, uint16_t samples, uint32_t window_ms) : _totalChannels(channels),
                                                                                                               _vref(vref),
                                                                                                               _resolution(resolution),
                                                                                                               _pin(255)
{
  // build our array of channels
  _averages.reserve(_totalChannels);
  for (uint8_t i = 0; i < _totalChannels; i++) {
    _averages.emplace_back(samples, window_ms); // RollingAverage(capacity, window_ms)
  }
}

unsigned int ADCHelper::getNewReading(uint8_t channel)
{
  // save the old one.
  uint8_t oldChannel = _currentChannel;

  // if we're currently working on a reading, then ignore it.
  while (isBusy()) {
    while (!isReady())
      vTaskDelay(1);
    loadReading(channel);
  }

  // lookup our channel please
  requestReading(channel);

  // wait for our reading
  while (!isReady())
    vTaskDelay(1);

  // okay load it in.
  unsigned int reading = loadReading(channel);

  // resume our regular scrolling
  requestReading(oldChannel);

  return reading;
}
float ADCHelper::getNewVoltage(uint8_t channel)
{
  return toVoltage(getNewReading(channel));
}

// dummy functions - should be implemented by the real clases.
void ADCHelper::requestADCReading(uint8_t channel) {}
bool ADCHelper::isADCReady() { return false; }
unsigned int ADCHelper::loadReadingFromADC(uint8_t channel) { return 0; }

unsigned int ADCHelper::getReadingCount(uint8_t channel)
{
  return _averages[channel].count();
}

void ADCHelper::clearReadings(uint8_t channel)
{
  _averages[channel].clear();
}

unsigned int ADCHelper::getLatestReading(uint8_t channel)
{
  if (channel >= _totalChannels)
    return 0;
  return _averages[channel].latest();
}

unsigned int ADCHelper::getAverageReading(uint8_t channel)
{
  if (channel >= _totalChannels)
    return 0;
  return _averages[channel].average();
}

float ADCHelper::toVoltage(unsigned int reading)
{
  // DUMP(reading);
  // DUMP(this->vref);
  // DUMP(this->resolution);

  // avoid pow(); use integer shift
  const uint32_t fullScale = (1UL << _resolution) - 1UL;
  return (float)reading * _vref / (float)fullScale;
}

float ADCHelper::getLatestVoltage(uint8_t channel) { return this->toVoltage(this->getLatestReading(channel)); }
float ADCHelper::getAverageVoltage(uint8_t channel) { return this->toVoltage(this->getAverageReading(channel)); }

void ADCHelper::requestReading(uint8_t channel)
{
  _currentChannel = channel;
  _isReady = false;
  _isBusy = true;
  requestADCReading(_currentChannel);
}

bool ADCHelper::isReady()
{
  if (_pin != 255)
    return _isReady;
  else
    return isADCReady();
}
bool ADCHelper::isBusy() { return _isBusy; }

unsigned int ADCHelper::loadReading(uint8_t channel)
{
  unsigned int reading = loadReadingFromADC(channel);

  if (channel < _totalChannels)
    _averages[channel].add(reading);

  _isReady = false;
  _isBusy = false;

  return reading;
}

void ADCHelper::attachReadyPinInterrupt(uint8_t pin, int mode)
{
  _pin = pin;
  attachInterruptArg(_pin, &ADCHelper::onReadyISR, this, mode);
}

void ADCHelper::onLoop()
{
  // smoke em if you got em
  if (isReady())
    loadReading(_currentChannel);

  // should we tee up the next one?
  if (!isBusy()) {

    // increment our channel
    _currentChannel++;
    if (_currentChannel >= _totalChannels)
      _currentChannel = 0;

    requestReading(_currentChannel);
  }
}

//
// ADS1115 Helper Class
//
ADS1115Helper::ADS1115Helper(float vref, ADS1115* adc, uint16_t samples, uint32_t window_ms)
    : ADCHelper(4, vref, 15, samples, window_ms), _adc(adc)
{
}

void ADS1115Helper::requestADCReading(uint8_t channel) { _adc->requestADC(channel); }
bool ADS1115Helper::isADCReady() { return _adc->isReady(); }

unsigned int ADS1115Helper::loadReadingFromADC(uint8_t channel)
{
  int16_t reading;
  reading = _adc->getValue();

  // no negatives please.
  if (reading < 0)
    reading = 0;

  return reading;
}

MCP3425Helper::MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc, uint16_t samples, uint32_t window_ms)
    : ADCHelper(1, vref, 15, samples, window_ms), _adc(adc), _config(config)
{
}

void MCP3425Helper::requestADCReading(uint8_t channel)
{
  uint8_t err;

  err = this->_adc->convert(this->_config);
  if (err) {
    Serial.printf("MCP3425 convert error: %d\n", err);
  }
}

bool MCP3425Helper::isADCReady()
{
  long value = 0;
  MCP342x::Config status;

  this->_adc->read(value, status);
  return status.isReady();
}

unsigned int MCP3425Helper::loadReadingFromADC(uint8_t channel)
{
  long value = 0;
  MCP342x::Config status;
  uint8_t err;

  // okay, is it ready?
  err = this->_adc->read(value, status);
  if (!err && status.isReady()) {
    return value;
  }
  return 0;
}

//
// MCP3564 Helper Class
//

MCP3564Helper::MCP3564Helper(float vref, MCP3564* adc, uint16_t samples, uint32_t window_ms)
    : ADCHelper(8, vref, 23, samples, window_ms), _adc(adc)
{
}

void MCP3564Helper::requestADCReading(uint8_t channel)
{
  // this->_adc->requestConversion(_channelAddresses[channel]);
}

bool MCP3564Helper::isADCReady()
{
  return true;
  // return this->_adc->isComplete();
}

unsigned int MCP3564Helper::loadReadingFromADC(uint8_t channel)
{
  return this->_adc->analogRead(_channelAddresses[channel]);
}

// float MCP3564Helper::toVoltage(unsigned int reading)
// {
//   return reading * this->adc->getReference() / (this->adc->getMaxValue() / 2);
// }