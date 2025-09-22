/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_CHANNEL_H
#define YARR_ADC_CHANNEL_H

#include "adchelper.h"
#include "config.h"
#include "prefs.h"
#include <Arduino.h>

#ifdef YB_HAS_ADC_CHANNELS

class ADCChannel
{
  public:
    /**
     * @brief What type of sensor is it?
     */
    enum class Type {
      RAW,
      POSITIVE_SWITCHING,
      NEGATIVE_SWITCHING,
      TANK_SENDER,
      THERMISTOR_1K,
      THERMISTOR_10K,
      FOUR_TWENTY_MA,
      HIGH_VOLT_DIVIDER,
      ONE_K_PULLUP,
      TEN_K_PULLUP,
      LOW_VOLT_DIVIDER
    };

    byte id = 0;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];
    Type type = Type::RAW;

  #ifdef YB_ADC_DRIVER_ADS1115
    ADS1115Helper* adcHelper;
  #elif YB_ADC_DRIVER_MCP3208
    MCP3208Helper* adcHelper;
  #endif

    void setup();
    void update();
    unsigned int getReading();
    float getVoltage();
    void resetAverage();

    float getTypeValue();
    const char* getTypeUnits();
};

extern ADCChannel adc_channels[YB_ADC_CHANNEL_COUNT];

void adc_channels_setup();
void adc_channels_loop();
bool isValidADCChannel(byte cid);

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif

#endif /* !YARR_INPUT_CHANNEL_H */