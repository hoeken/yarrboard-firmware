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

    uint32_t getNewReading(uint8_t channel);
    float getNewVoltage(uint8_t channel);

    uint32_t getLatestReading(uint8_t channel);
    uint32_t getAverageReading(uint8_t channel);
    float toVoltage(uint32_t reading);
    float getLatestVoltage(uint8_t channel);
    float getAverageVoltage(uint8_t channel);

    uint16_t getReadingCount(uint8_t channel);
    void clearReadings(uint8_t channel);
    void printDebug(int8_t channel = -1, bool rawData = false);

    virtual void attachReadyPinInterrupt(uint8_t pin, int mode);
    void requestReading(uint8_t channel);
    void onLoop();
    bool isReady();
    bool isBusy();

  protected:
    float _vref = 0.0;

  private:
    volatile bool _isReady = false;
    bool _isBusy = false;
    uint8_t _pin = 255;
    uint8_t _resolution;
    std::vector<RollingAverage> _averages;
    unsigned long lastDebugTime = 0;

    uint8_t _totalChannels = 0;
    uint8_t _currentChannel = 0;

    virtual void requestADCReading(uint8_t channel);
    virtual bool isADCReady();
    uint32_t loadReading(uint8_t channel);
    virtual uint32_t loadReadingFromADC(uint8_t channel);

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
    ADS1115Helper(const float vrefs[4], const uint8_t gains[4], ADS1115* adc, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);
    void attachReadyPinInterrupt(uint8_t pin, int mode) override;
    void requestADCReading(uint8_t channel) override;
    bool isADCReady() override;
    uint32_t loadReadingFromADC(uint8_t channel) override;
    float getLatestVoltage(uint8_t channel);
    float getAverageVoltage(uint8_t channel);

  private:
    ADS1115* _adc;
    bool _perChannelConfig = false;
    float _vrefs[4];
    uint8_t _gains[4];
};

class MCP3425Helper : public ADCHelper
{
  public:
    MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc, uint16_t samples = RA_DEFAULT_SIZE, uint32_t window_ms = RA_DEFAULT_WINDOW);
    void requestADCReading(uint8_t channel) override;
    bool isADCReady() override;
    uint32_t loadReadingFromADC(uint8_t channel) override;

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
    uint32_t loadReadingFromADC(uint8_t channel) override;

  private:
    uint32_t _channelAddresses[8] = {MCP_CH0, MCP_CH1, MCP_CH2, MCP_CH3, MCP_CH4, MCP_CH5, MCP_CH6, MCP_CH7};
    MCP3564* _adc;
};

#endif /* !YARR_ADC_H */