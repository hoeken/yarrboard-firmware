/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_H
#define YARR_ADC_H

#include "RollingAverage.h"
#include <ADS1X15.h>
#include <Arduino.h>
#include <MCP342x.h>
#include <MCP3x6x.h>
#include <RunningAverage.h>
#include <Wire.h>
#include <vector>

#include "config.h"

class ADCHelper
{
  public:
    ADCHelper(uint8_t channels, float vref, uint8_t resolution, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);

    // non copyable.
    virtual ~ADCHelper() = default;
    ADCHelper(const ADCHelper&) = delete;
    ADCHelper& operator=(const ADCHelper&) = delete;

    unsigned int getNewReading(uint8_t channel);
    float getNewVoltage(uint8_t channel);

    unsigned int getLatestReading(uint8_t channel);
    unsigned int getAverageReading(uint8_t channel);
    float toVoltage(unsigned int reading);
    float getLatestVoltage(uint8_t channel);
    float getAverageVoltage(uint8_t channel);

    unsigned int getReadingCount(uint8_t channel);
    void clearReadings(uint8_t channel);
    void printDebug();

    void attachReadyPinInterrupt(uint8_t pin, int mode);
    void requestReading(uint8_t channel);
    void onLoop();
    bool isReady();
    bool isBusy();

  private:
    volatile bool _isReady = false;
    bool _isBusy = false;
    uint8_t _pin = 255;
    float _vref = 0.0;
    uint8_t _resolution;
    std::vector<RollingAverage> _averages;
    unsigned long lastDebugTime = 0 - 10000;

    uint8_t _totalChannels = 0;
    uint8_t _currentChannel = 0;

    virtual void requestADCReading(uint8_t channel);
    virtual bool isADCReady();
    unsigned int loadReading(uint8_t channel);
    virtual unsigned int loadReadingFromADC(uint8_t channel);

    static void ARDUINO_ISR_ATTR onReadyISR(void* arg)
    {
      // ISR context — keep it tiny
      auto* self = static_cast<ADCHelper*>(arg);
      self->_isReady = true;
    }
};

class ADS1115Helper : public ADCHelper
{
  public:
    ADS1115Helper(float vref, ADS1115* adc, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);
    void requestADCReading(uint8_t channel) override;
    bool isADCReady() override;
    unsigned int loadReadingFromADC(uint8_t channel) override;

  private:
    ADS1115* _adc;
};

class MCP3425Helper : public ADCHelper
{
  public:
    MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);
    void requestADCReading(uint8_t channel) override;
    bool isADCReady() override;
    unsigned int loadReadingFromADC(uint8_t channel) override;

  private:
    MCP342x* _adc;
    MCP342x::Config _config;
};

class MCP3564Helper : public ADCHelper
{
  public:
    MCP3564Helper(float vref, MCP3564* adc, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);
    void requestADCReading(uint8_t channel) override;
    bool isADCReady() override;
    unsigned int loadReadingFromADC(uint8_t channel) override;

  private:
    unsigned int _channelAddresses[8] = {MCP_CH0, MCP_CH1, MCP_CH2, MCP_CH3, MCP_CH4, MCP_CH5, MCP_CH6, MCP_CH7};
    MCP3564* _adc;
};

#endif /* !YARR_ADC_H */