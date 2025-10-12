/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_ADC_CHANNEL_H
#define YARR_ADC_CHANNEL_H

#include "ArduinoJson.h"
#include "adchelper.h"
#include "config.h"
#include "etl/vector.h"
#include "prefs.h"
#include <Arduino.h>
#include <LittleFS.h>

#ifdef YB_HAS_ADC_CHANNELS

struct CalibrationPoint {
    float voltage;
    float calibrated;
};

class ADCChannel
{
  public:
    byte id = 0;
    bool isEnabled = true;
    char name[YB_CHANNEL_NAME_LENGTH];
    int8_t displayDecimals = -1;
    float lastValue = 0.0;

    bool useCalibrationTable = false;
    char calibratedUnits[YB_ADC_UNIT_LENGTH];
    etl::vector<CalibrationPoint, YB_ADC_CALIBRATION_TABLE_MAX> calibrationTable;

    /*
    raw - Raw Output
    digital_switch - Digital Switch
    thermistor - Thermistor
    4-20ma - 4-20mA Sensor
    high_volt_divider - 0-32v Input
    low_volt_divider - 0-5v Input
    ten_k_pullup - 10k Pullup
    */
    char type[30];

  #ifdef YB_ADC_DRIVER_ADS1115
    ADS1115Helper* adcHelper;
  #elif YB_ADC_DRIVER_MCP3564
    MCP3564Helper* adcHelper;
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

    bool loadCalibrationTable();
    bool parseCalibrationTableJson(JsonVariantConst root);
    float interpolateValue(float xv);
    bool saveCalibrationTable();
    bool addCalibrationValue(CalibrationPoint cp);

  private:
    void _sortAndDedupeCalibrationTable();
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

#endif /* !YARR_ADC_CHANNEL_H */