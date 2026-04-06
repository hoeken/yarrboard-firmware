/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_CONTROLLER_H
#define YARR_BRINEOMATIC_CONTROLLER_H

#include "config.h"

#ifdef YB_IS_BRINEOMATIC
  #include "adchelper.h"

  #include <Arduino.h>

  #include "brineomatic.h"
  #include <ArduinoJson.h>
  #include <PsychicMqttClient.h>
  #include <controllers/BaseController.h>
  #include <controllers/ProtocolController.h>

class YarrboardApp;
class ConfigManager;
class RelayController;
class ServoController;
class StepperController;

class BrineomaticController : public BaseController
{
  public:
    BrineomaticController(YarrboardApp& app, RelayController& relays, ServoController& servos, StepperController& steppers);

    bool setup() override;
    void loop() override;

    bool loadConfigHook(JsonVariant config, char* error, size_t len) override;
    void generateConfigHook(JsonVariant config) override;
    void generateCapabilitiesHook(JsonVariant config) override;
    void generateUpdateHook(JsonVariant output) override;
    void generateStatsHook(JsonVariant output) override;
    void mqttUpdateHook(MQTTController* mqtt) override;

    void haUpdateHook(MQTTController* mqtt) override;
    void haGenerateDiscoveryHook(JsonVariant components, const char* uuid, MQTTController* mqtt) override;

    void haGenerateMotorTemperatureDiscovery(JsonVariant doc);
    void haGenerateWaterTemperatureDiscovery(JsonVariant doc);
    void haGenerateStatusDiscovery(JsonVariant doc);
    void haGenerateFilterPressureDiscovery(JsonVariant doc);
    void haGenerateMembranePressureDiscovery(JsonVariant doc);
    void haGenerateProductSalinityDiscovery(JsonVariant doc);
    void haGenerateBrineSalinityDiscovery(JsonVariant doc);
    void haGenerateProductFlowrateDiscovery(JsonVariant doc);
    void haGenerateBrineFlowrateDiscovery(JsonVariant doc);
    void haGenerateTotalFlowrateDiscovery(JsonVariant doc);
    void haGenerateTankLevelDiscovery(JsonVariant doc);
    void haGenerateBatteryLevelDiscovery(JsonVariant doc);
    void haGenerateVolumeDiscovery(JsonVariant doc);
    void haGenerateFlushVolumeDiscovery(JsonVariant doc);
    void haGenerateHighPressurePumpDiscovery(JsonVariant doc);
    void haGenerateDiverterValveDiscovery(JsonVariant doc);
    void haGenerateFlushValveDiscovery(JsonVariant doc);
    void haGenerateCoolingFanDiscovery(JsonVariant doc);
    void haGenerateBoostPumpDiscovery(JsonVariant doc);
    void haGenerateRunResultDiscovery(JsonVariant doc);
    void haGenerateFlushResultDiscovery(JsonVariant doc);
    void haGeneratePickleResultDiscovery(JsonVariant doc);
    void haGenerateDepickleResultDiscovery(JsonVariant doc);
    void haGenerateNextFlushCountdownDiscovery(JsonVariant doc);
    void haGenerateRuntimeElapsedDiscovery(JsonVariant doc);
    void haGenerateFinishCountdownDiscovery(JsonVariant doc);

    static void handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup);

    static void handleMotorTemperatureCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleMotorTemperatureCallback(const char* topic, const char* payload, int retain, int qos, bool dup);

    static void handleWaterTemperatureCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleWaterTemperatureCallback(const char* topic, const char* payload, int retain, int qos, bool dup);

    static void handleTankLevelCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleTankLevelCallback(const char* topic, const char* payload, int retain, int qos, bool dup);

    static void handleBatteryLevelCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup);
    void handleBatteryLevelCallback(const char* topic, const char* payload, int retain, int qos, bool dup);

    void handleStartWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleFlushWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handlePickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleDepickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleStopWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleIdleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleManualWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSetWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveUIConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveGeneralConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveHardwareConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);
    void handleSaveSafeguardsConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context);

  private:
    Brineomatic wm;

    uint32_t lastOutput;

    char ha_key[YB_HOSTNAME_LENGTH];
    char ha_uuid[64];
    char ha_topic_avail[128];
    char ha_topic_cmd_state[128];
    char ha_topic_state_state[128];

    char ha_topic_motor_temperature[128];
    char ha_topic_water_temperature[128];
    char ha_topic_status[128];
    char ha_topic_filter_pressure[128];
    char ha_topic_membrane_pressure[128];
    char ha_topic_product_salinity[128];
    char ha_topic_brine_salinity[128];
    char ha_topic_product_flowrate[128];
    char ha_topic_brine_flowrate[128];
    char ha_topic_total_flowrate[128];
    char ha_topic_tank_level[128];
    char ha_topic_battery_level[128];
    char ha_topic_volume[128];
    char ha_topic_flush_volume[128];
    char ha_topic_high_pressure_pump_on[128];
    char ha_topic_diverter_valve_open[128];
    char ha_topic_flush_valve_open[128];
    char ha_topic_cooling_fan_on[128];
    char ha_topic_boost_pump_on[128];
    char ha_topic_run_result[128];
    char ha_topic_flush_result[128];
    char ha_topic_pickle_result[128];
    char ha_topic_depickle_result[128];
    char ha_topic_next_flush_countdown[128];
    char ha_topic_runtime_elapsed[128];
    char ha_topic_finish_countdown[128];

    bool _haCallbacksRegistered = false;
    static BrineomaticController* _instance;

    // The actual state machine logic
    void stateMachine();

    // The static wrapper that FreeRTOS can call
    static void stateMachineTask(void* pvParameters);
};

#endif
#endif /* !YARR_BRINEOMATIC_CONTROLLER_H */