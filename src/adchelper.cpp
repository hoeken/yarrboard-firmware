/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "adchelper.h"
#include "debug.h"

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

uint32_t ADCHelper::getNewReading(uint8_t channel)
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
  uint32_t reading = loadReading(channel);

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
uint32_t ADCHelper::loadReadingFromADC(uint8_t channel) { return 0; }

uint16_t ADCHelper::getReadingCount(uint8_t channel)
{
  return _averages[channel].count();
}

void ADCHelper::clearReadings(uint8_t channel)
{
  _averages[channel].clear();
}

void ADCHelper::printDebug()
{
  // only print every 10s
  if (millis() - lastDebugTime > 5000) {
    YBP.printf("%d Channels | Vref: %.3f | Resolution: %d\n", _totalChannels, _vref, _resolution);

    for (byte i = 0; i < _totalChannels; i++) {
      uint16_t cnt = getReadingCount(i);
      size_t cap = _averages[i].cap();
      uint32_t window = _averages[i].window();
      uint32_t avgr = getAverageReading(i);
      float avgv = getAverageVoltage(i);
      YBP.printf("CH%d: Window: %dms | Readings: %d/%d | Average: %d | Voltage: %.3f\n", i, window, cnt, cap, avgr, avgv);
    }

    lastDebugTime = millis();
  }
}

uint32_t ADCHelper::getLatestReading(uint8_t channel)
{
  if (channel >= _totalChannels)
    return 0;
  return _averages[channel].latest();
}

uint32_t ADCHelper::getAverageReading(uint8_t channel)
{
  if (channel >= _totalChannels)
    return 0;
  return _averages[channel].average();
}

float ADCHelper::toVoltage(uint32_t reading)
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

uint32_t ADCHelper::loadReading(uint8_t channel)
{
  uint32_t reading = loadReadingFromADC(channel);

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
  if (isReady()) {
    delay(1);
    loadReading(_currentChannel);
  }

  // should we tee up the next one?
  if (!isBusy()) {
    // increment our channel
    _currentChannel++;
    if (_currentChannel >= _totalChannels)
      _currentChannel = 0;

    requestReading(_currentChannel);
  }

  // printDebug();
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

void ADS1115Helper::attachReadyPinInterrupt(uint8_t pin, int mode)
{
  _adc->setComparatorThresholdLow(0x0000);
  _adc->setComparatorThresholdHigh(0x8000);
  _adc->setComparatorPolarity(0);
  _adc->setComparatorLatch(0);
  _adc->setComparatorQueConvert(0);

  ADCHelper::attachReadyPinInterrupt(pin, mode);
}

uint32_t ADS1115Helper::loadReadingFromADC(uint8_t channel)
{
  // YBP.printf("CH%d READ\n", channel);
  // bool busy = _adc->isBusy();
  // while (busy) {
  //   YBP.printf("CH%d BUSY: %d\n", channel, busy);
  //   busy = _adc->isBusy();
  //   delay(10);
  // }

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
    YBP.printf("MCP3425 convert error: %d\n", err);
  }
}

bool MCP3425Helper::isADCReady()
{
  long value = 0;
  MCP342x::Config status;

  this->_adc->read(value, status);
  return status.isReady();
}

uint32_t MCP3425Helper::loadReadingFromADC(uint8_t channel)
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
  this->_adc->startMux(_channelAddresses[channel]);
}

bool MCP3564Helper::isADCReady()
{
  return this->_adc->isComplete();
}

uint32_t MCP3564Helper::loadReadingFromADC(uint8_t channel)
{
  int32_t result = this->_adc->readResult();
  if (result < 0)
    result = 0;

  return result;
}