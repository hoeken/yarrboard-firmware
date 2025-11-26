#ifndef YB_FLOWMETER_H
#define YB_FLOWMETER_H

#include <Arduino.h>

class Flowmeter
{
  public:
    Flowmeter();

    void begin(uint8_t pin, float pulsesPerLiter);
    bool measure();
    float pulsesPerLiter();

    float getFlowrate();
    float getVolume();

  private:
    uint8_t _pin;
    float _pulsesPerLiter;

    volatile uint16_t _pulseCounter;
    uint32_t _lastCheckMicros;

    float _flowrate;
    float _volume;

    static void IRAM_ATTR isrHandler(void* arg);
};

#endif