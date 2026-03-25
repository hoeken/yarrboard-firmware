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

#ifdef YB_HAS_ADC_CHANNELS

struct CalibrationPoint {
    float voltage;
    float calibrated;
};

class MQTTController;

class ADCChannel : public BaseChannel
{
  public:
    int8_t displayDecimals = 2;
    uint32_t averageWindowMs = YB_ADC_RUNNING_AVERAGE_WINDOW_MS;
    float lastValue = 0.0;
    bool lastRawDigital = false;
    bool toggleState = false;

    bool useCalibrationTable = false;
    char calibratedUnits[YB_ADC_UNIT_LENGTH];
    etl::vector<CalibrationPoint, YB_ADC_CALIBRATION_TABLE_MAX> calibrationTable;

    char haDeviceClass[64] = "none";

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

    /*
      direct - Raw Output
      inverted - Output Inverted
      toggle_rising - Output Toggles on rising value
      toggle_falling - Output toggles on falling value
    */
    char digitalInputMode[20] = "direct";

    ADS1115Helper* adcHelper;
    uint8_t _adcChannel = 0;

    unsigned int getReading();
    float getVoltage();

    float getTypeValue();
    const char* getTypeUnits();

    void init(uint8_t id) override;
    void setup();
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    bool parseCalibrationTableJson(JsonVariantConst root, char* error, size_t len);
    float interpolateValue(float xv);
    bool addCalibrationValue(CalibrationPoint cp);

    void haGenerateDiscovery(JsonVariant doc, const char* uuid, MQTTController* mqtt);

  private:
    char ha_topic_value[128];

    void _sortAndDedupeCalibrationTable();
};

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif
#endif /* !YARR_ADC_CHANNEL_H */