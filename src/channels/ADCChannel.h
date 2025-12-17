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
#include <Arduino.h>
#include <LittleFS.h>
#include <channels/BaseChannel.h>

struct CalibrationPoint {
    float voltage;
    float calibrated;
};

class ADCChannel : public BaseChannel
{
  public:
    int8_t displayDecimals = 2;
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
    char type[33] = "raw";

    ADS1115Helper* adcHelper;
    uint8_t _adcChannel = 0;

    unsigned int getReading();
    float getVoltage();

    float getTypeValue();
    const char* getTypeUnits();

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    bool parseCalibrationTableJson(JsonVariantConst root);
    float interpolateValue(float xv);
    bool addCalibrationValue(CalibrationPoint cp);

    void haGenerateDiscovery(JsonVariant doc);
    void haGenerateSensorDiscovery(JsonVariant doc);

  private:
    char ha_topic_value[128];

    void _sortAndDedupeCalibrationTable();
};

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_ADC_CHANNEL_H */