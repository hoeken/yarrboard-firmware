/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PWM_CHANNEL_H
#define YARR_PWM_CHANNEL_H

#include "adchelper.h"
#include "base_channel.h"
#include "bus_voltage.h"
#include "config.h"
#include "driver/ledc.h"
#include "mqtt.h"
#include "network.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>
#include <SPI.h>

class PWMChannel : public BaseChannel
{
  private:
    char ha_uuid[64];
    char ha_topic_cmd_state[128];
    char ha_topic_cmd_brightness[128];
    char ha_topic_state_state[128];
    char ha_topic_state_brightness[128];
    char ha_topic_avail[128];
    char ha_topic_voltage[128];
    char ha_topic_current[128];

  protected:
    byte _pins[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_PINS;

  public:
    /**
     * @brief Potential status that channel might be in.
     */
    enum class Status {
      ON,
      OFF,
      TRIPPED,
      BLOWN,
      BYPASSED
    };

    Status status = Status::OFF;
    bool outputState = false;
    char type[30] = "other";
    char defaultState[10] = "OFF";
    bool sendFastUpdate = false;
    char source[YB_HOSTNAME_LENGTH];

    unsigned int stateChangeCount = 0;
    unsigned int softFuseTripCount = 0;

    float dutyCycle = 0.0;
    float lastDutyCycle = 0.0;
    unsigned long lastDutyCycleUpdate = 0;
    unsigned long dutyCycleIsThrottled = 0;

    bool fadeRequested = false;
    unsigned long fadeStartTime = 0;
    unsigned long fadeDuration = 0;
    float fadeDutyCycleStart = 0;
    float fadeDutyCycleEnd = 0;

#ifdef YB_PWM_CHANNEL_CURRENT_ADC_DRIVER_MCP3564
    MCP3564Helper* amperageHelper;
#endif

#ifdef YB_HAS_CHANNEL_VOLTAGE
  #ifdef YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_ADS1115
    ADS1115Helper* voltageHelper;
  #elif defined(YB_PWM_CHANNEL_VOLTAGE_ADC_DRIVER_MCP3564)
    MCP3564Helper* voltageHelper;
  #endif
#endif

    float voltageOffset = 0;
    float amperageOffset = 0;

    float voltage = 0.0;
    float amperage = 0.0;
    float softFuseAmperage = YB_PWM_CHANNEL_MAX_AMPS;
    float ampHours = 0.0;
    float wattHours = 0.0;

    bool isDimmable = false;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;

    void setup();
    void setupLedc();
    void setupInterrupt();
    void setupOffset();
    void setupDefaultState();
    void saveThrottledDutyCycle();

    void updateOutput(bool check_status = false);
    void checkStatus();

    float getVoltage();
    float toVoltage(float adcVoltage);
    void checkVoltage();
    void checkFuseBlown();
    void checkFuseBypassed();

    float getAmperage();
    float toAmperage(float voltage);
    void checkAmperage();
    void checkSoftFuse();

    void checkIfFadeOver();
    void setState(const char* state);
    void setState(bool newState);

    void setFade(float duty, int max_fade_time_ms);
    void setDuty(float duty);
    void calculateAverages(unsigned int delta);
    const char* getStatus();

    void updateOutputLED();

    void haGenerateDiscovery(JsonVariant doc);
    void haGenerateLightDiscovery(JsonVariant doc);
    void haGenerateVoltageDiscovery(JsonVariant doc);
    void haGenerateAmperageDiscovery(JsonVariant doc);
    void haPublishAvailable();
    void haPublishState();
    void haHandleCommand(const char* topic, const char* payload);
};

extern etl::array<PWMChannel, YB_PWM_CHANNEL_COUNT> pwm_channels;

void pwm_channels_setup();
void pwm_channels_loop();

void pwm_handle_ha_command(const char* topic, const char* payload, int retain, int qos, bool dup);

#endif /* !YARR_PWM_CHANNEL_H */