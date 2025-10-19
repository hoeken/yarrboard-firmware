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

// dummy functions - should be implemented by the real clases.
void ADCHelper::requestADCReading(uint8_t channel) {}
bool ADCHelper::isADCReady() { return false; }
unsigned int ADCHelper::loadReadingFromADC(uint8_t channel) { return 0; }

unsigned int ADCHelper::getReadingCount(uint8_t channel)
{
  return _averages[channel].count();
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

void ADCHelper::loadReading(uint8_t channel)
{
  unsigned int reading = loadReadingFromADC(channel);

  if (channel < _totalChannels)
    _averages[channel].add(reading);

  _isReady = false;
  _isBusy = false;
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
    : ADCHelper::ADCHelper(8, vref, 15, samples, window_ms), _adc(adc)
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

// esp32Helper::esp32Helper(float vref, uint8_t channel, uint16_t samples)
//     : ADCHelper::ADCHelper(vref, 12, samples)
// {
//   this->channel = channel;
// }

// unsigned int esp32Helper::getReading()
// {
//   unsigned int reading = analogReadMilliVolts(this->channel);
//   this->addReading(reading);

//   return reading;
// }

// float esp32Helper::toVoltage(unsigned int reading)
// {
//   return (float)reading / 1000.0;
// }

// MCP3564Helper::MCP3564Helper(float vref, uint8_t channel, MCP3564* adc, uint16_t samples)
//     : ADCHelper::ADCHelper(vref, 24, samples), channel(channel), adc(adc)
// {
// }

// unsigned int MCP3564Helper::getReading()
// {
//   unsigned int reading;
//   reading = this->adc->analogRead(_channelAddresses[this->channel]);
//   this->addReading(reading);

//   return reading;
// }

// float MCP3564Helper::toVoltage(unsigned int reading)
// {
//   return reading * this->adc->getReference() / (this->adc->getMaxValue() / 2);
// }

// MCP3425Helper::MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc, uint16_t samples)
//     : ADCHelper::ADCHelper(vref, 15, samples), adc(adc), config(config)
// {
// }

// void MCP3425Helper::setup()
// {
//   Wire.begin();
//   this->start_conversion = true;
// }

// unsigned int MCP3425Helper::getReading()
// {
//   long value = 0;
//   MCP342x::Config status;
//   uint8_t err;

//   // do we need to trigger a conversion?
//   if (this->start_conversion) {

//     err = this->adc->convert(this->config);
//     if (err) {
//       Serial.printf("MCP3425 convert error: %d\n", err);
//     }
//     this->start_conversion = false;
//     return 0;
//   }

//   // okay, is it ready?
//   err = this->adc->read(value, status);
//   if (!err && status.isReady()) {
//     this->addReading(value);
//     this->start_conversion = true;
//   } else {
//     if (err != 4)
//       Serial.printf("MCP3425 read error: %d\n", err);

//     value = 0;
//   }

//   return value;
// }
