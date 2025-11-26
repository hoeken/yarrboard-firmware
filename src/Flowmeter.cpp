#include "Flowmeter.h"

Flowmeter::Flowmeter()
    : _pin(0),
      _pulsesPerLiter(0),
      _pulseCounter(0),
      _lastCheckMicros(0)
{
}

void Flowmeter::begin(uint8_t pin, float pulsesPerLiter)
{
  _pin = pin;
  _pulsesPerLiter = pulsesPerLiter;

  _pulseCounter = 0;
  _lastCheckMicros = 0;

  pinMode(_pin, INPUT);

  // ESP32 supports ISR with argument
  attachInterruptArg(
    digitalPinToInterrupt(_pin),
    Flowmeter::isrHandler,
    this,
    FALLING);
}

void IRAM_ATTR Flowmeter::isrHandler(void* arg)
{
  Flowmeter* fm = static_cast<Flowmeter*>(arg);
  fm->_pulseCounter = fm->_pulseCounter + 1;
}

bool Flowmeter::measure()
{
  uint32_t now = micros();
  uint32_t elapsed = now - _lastCheckMicros;

  if (elapsed >= 1000000) { // 1 second interval
    float elapsedSeconds = elapsed / 1000000.0f;

    float litersPerSecond = (static_cast<float>(_pulseCounter) / _pulsesPerLiter) / elapsedSeconds;

    _flowrate = litersPerSecond * 3600.0f; // convert to L/h
    _volume = _pulseCounter / _pulsesPerLiter;

    _pulseCounter = 0;
    _lastCheckMicros = now;

    return true;
  }

  return false;
}

float Flowmeter::pulsesPerLiter()
{
  return _pulsesPerLiter;
}

float Flowmeter::getFlowrate()
{
  return _flowrate;
}

float Flowmeter::getVolume()
{
  return _volume;
}