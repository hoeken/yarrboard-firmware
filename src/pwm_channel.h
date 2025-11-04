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
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "mqtt.h"
#include "networking.h"
#include "prefs.h"
#include "protocol.h"
#include <Arduino.h>
#include <SPI.h>

class PWMChannel : public BaseChannel
{
  private:
    char ha_topic_cmd_state[128];
    char ha_topic_cmd_brightness[128];
    char ha_topic_state_state[128];
    char ha_topic_state_brightness[128];
    char ha_topic_voltage[128];
    char ha_topic_current[128];

    byte pin = 0;
    unsigned long lastStateChange = 0;

    struct GammaState {
        uint8_t pin = 255;
        uint8_t idx = 0;
        int step_ms = 0;
        int last_ms = 0;
        etl::array<uint32_t, 16> targets;
        void (*user_cb)(void*) = nullptr;
        void* user_arg = nullptr;
        bool active = false;
        PWMChannel* owner;
    } gamma;

    // Gamma aware led fading: same signature as ledcFadeWithInterruptArg
    bool gammaFadeWithInterrupt(uint8_t pin_,
      uint32_t start_duty,
      uint32_t end_duty,
      int total_ms,
      void (*final_cb)(void*),
      void* final_arg);

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

    // polled from other code; updated in ISR
    volatile bool isFading = false; // used to check if we're actively fading
    volatile bool fadeOver = false; // used for running code after a fade

    // bool isInverted = false; // determines whether our output pin is inverted or not

    float dutyCycle = 0.0;
    float lastDutyCycle = 0.0;
    unsigned long lastDutyCycleUpdate = 0;
    unsigned long dutyCycleIsThrottled = 0;

    // fading speeds / duration
    unsigned int rampOnMillis = 1000;
    unsigned int rampOffMillis = 1000;

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
    uint8_t _adcVoltageChannel = 0;
    uint8_t _adcAmperageChannel = 0;

    float voltageOffset = 0.0;
    float amperageOffset = 0.0;

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
    void setupOffset();
    void setupDefaultState();
    void saveThrottledDutyCycle();
    float getCurrentDutyCycle();

    void updateOutput(bool check_status = false);
    void checkStatus();

    float getVoltage();
    float toVoltage(float adcVoltage);
    void checkFuseBlown();
    void checkFuseBypassed();

    float getAmperage();
    float toAmperage(float voltage);
    void checkSoftFuse();

    float getWattage();

    void checkIfFadeOver();
    void setState(const char* state);
    void setState(bool newState);
    void writePWM(uint16_t pwm);

    void startFade(float duty, int fade_time);
    void setDuty(float duty);
    void calculateAverages(unsigned int delta);
    const char* getStatus();

    void updateOutputLED();

    void haGenerateDiscovery(JsonVariant doc);
    void haGenerateLightDiscovery(JsonVariant doc);
    void haGenerateVoltageDiscovery(JsonVariant doc);
    void haGenerateAmperageDiscovery(JsonVariant doc);
    void haPublishState();
    void haHandleCommand(const char* topic, const char* payload);

    static void ARDUINO_ISR_ATTR onFadeISR(void* arg)
    {
      // ISR context — keep it tiny
      auto* self = static_cast<PWMChannel*>(arg);
      self->isFading = false;
      self->fadeOver = true;
    }

    // ISR (Arduino style)
    static void ARDUINO_ISR_ATTR gammaISR(void* arg);
    static void continueGammaThunk(void* arg, uint32_t);
};

extern etl::array<PWMChannel, YB_PWM_CHANNEL_COUNT> pwm_channels;

void pwm_channels_setup();
void pwm_channels_loop();

void pwm_handle_ha_command(const char* topic, const char* payload, int retain, int qos, bool dup);

#endif /* !YARR_PWM_CHANNEL_H */