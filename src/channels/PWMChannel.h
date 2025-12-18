/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PWM_CHANNEL_H
#define YARR_PWM_CHANNEL_H

#include "config.h"

#ifdef YB_HAS_PWM_CHANNELS

  #include "adchelper.h"
  #include "controllers/BusVoltageController.h"
  #include "controllers/RGBController.h"
  #include "driver/ledc.h"
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
  #include <Arduino.h>
  #include <SPI.h>
  #include <channels/BaseChannel.h>
  #include <controllers/MQTTController.h>

  #ifdef YB_PWM_CHANNEL_HAS_INA226
    #include "INA226.h"
  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
    #include <Temperature_LM75_Derived.h>
  #endif

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

  #ifdef YB_PWM_CHANNEL_HAS_INA226
    static void IRAM_ATTR ina226AlertHandler(void* arg);
  #endif

  public:
    /**
     * @brief Potential status that channel might be in.
     */
    enum class Status {
      ON,
      OFF,
      TRIPPED,
      BLOWN,
      BYPASSED,
      OVERHEAT
    };

    volatile Status status = Status::OFF;
    volatile bool outputState = false;
    char type[30] = "other";
    char defaultState[10] = "OFF";
    char source[YB_HOSTNAME_LENGTH];

    unsigned int stateChangeCount = 0;
    volatile unsigned int softFuseTripCount = 0;
    unsigned int overheatCount = 0;

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

    ConfigManager* _cfg = nullptr;
    BusVoltageController* busVoltage = nullptr;
    RGBControllerInterface* rgb = nullptr;

    void init(uint8_t id) override;
    bool loadConfig(JsonVariantConst config, char* error, size_t len) override;
    void generateConfig(JsonVariant config) override;
    void generateUpdate(JsonVariant config) override;
    void generateStats(JsonVariant output) override;

    void setup();
    void setupLedc();
    void setupOffset();
    void setupDefaultState();

  #ifdef YB_PWM_CHANNEL_HAS_INA226
    INA226* ina226;
    void setupINA226();
    void readINA226();
    float lastVoltage;
    uint32_t lastVoltageUpdate = 0;
    float lastAmperage;
    uint32_t lastAmperageUpdate = 0;
  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
    TI_LM75B* lm75;
    float lastTemperature;
    uint32_t lastTemperatureUpdate = 0;
    void readLM75();
  #endif

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

    float getTemperature();
    void checkOverheat();

    void checkIfFadeOver();
    void setState(const char* state);
    void setState(bool newState);
    void writePWM(uint16_t pwm);

    void startFade(float duty, int fade_time);
    void setDuty(float duty);
    void calculateAverages(unsigned int delta);
    const char* getStatus();

    void updateOutputLED();

    void haGenerateDiscovery(JsonVariant doc, const char* uuid, MQTTController* mqtt);
    void haGenerateLightDiscovery(JsonVariant doc);
    void haGenerateVoltageDiscovery(JsonVariant doc);
    void haGenerateAmperageDiscovery(JsonVariant doc);
    void haPublishState(MQTTController* mqtt);
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

  private:
    /* Setting PWM Properties */
    const unsigned int MAX_DUTY_CYCLE = (int)(pow(2, YB_PWM_CHANNEL_RESOLUTION)) - 1;

    // our channel pins
    byte _pwm_pins[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_PINS;

  #ifdef YB_PWM_CHANNEL_INA226_ADDRESS
    byte _ina226_addresses[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_INA226_ADDRESS;
  #endif

  #ifdef YB_PWM_CHANNEL_INA226_ALERT
    byte _ina226_alert_pins[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_INA226_ALERT;
  #endif

  #ifdef YB_PWM_CHANNEL_HAS_LM75
    byte _lm75_addresses[YB_PWM_CHANNEL_COUNT] = YB_PWM_CHANNEL_LM75_ADDRESS;
  #endif
};
#endif
#endif /* !YARR_PWM_CHANNEL_H */