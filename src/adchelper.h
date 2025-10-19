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

#include "config.h"

#ifndef RA_DEFAULT_SIZE
  #define RA_DEFAULT_SIZE 16
#endif

class ADCHelper
{
  public:
    ADCHelper(float vref, uint8_t resolution, uint16_t samples = RA_DEFAULT_SIZE);

    bool requestReading(uint8_t channel);
    bool isReady();
    unsigned int getReading();
    float getAverageReading();
    void addReading(unsigned int reading);
    float getVoltage();
    float getAverageVoltage();
    void resetAverage();
    float toVoltage(unsigned int reading);

  private:
    float vref = 0.0;
    uint8_t resolution;
    RollingAverage runningAverage;
};

class esp32Helper : public ADCHelper
{
  public:
    esp32Helper(float vref, uint8_t channel, uint16_t samples = RA_DEFAULT_SIZE);
    unsigned int getReading();
    float toVoltage(unsigned int reading);

  private:
    uint8_t channel;
};

class MCP3425Helper : public ADCHelper
{
  public:
    MCP3425Helper(MCP342x::Config& config, float vref, MCP342x* adc, uint16_t samples = RA_DEFAULT_SIZE);
    unsigned int getReading();
    void setup();

  private:
    MCP342x* adc;
    bool start_conversion = false;
    MCP342x::Config config;
};

class MCP3564Helper : public ADCHelper
{
  public:
    MCP3564Helper(float vref, uint8_t channel, MCP3564* adc, uint16_t samples = RA_DEFAULT_SIZE);
    unsigned int getReading();
    float toVoltage(unsigned int reading);

  private:
    unsigned int _channelAddresses[8] = {MCP_CH0, MCP_CH1, MCP_CH2, MCP_CH3, MCP_CH4, MCP_CH5, MCP_CH6, MCP_CH7};
    MCP3564* adc;
    uint8_t channel;
};

class ADS1115Helper : public ADCHelper
{
  public:
    ADS1115Helper(float vref, uint8_t channel, ADS1115* adc, uint16_t samples = RA_DEFAULT_SIZE);
    bool requestReading(uint8_t channel);
    bool isReady();
    unsigned int getReading();
    float toVoltage(unsigned int reading);
    ADS1115* adc;

  private:
    uint8_t channel;
};

#endif /* !YARR_ADC_H */