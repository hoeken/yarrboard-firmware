/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_IS_BRINEOMATIC

  #include "BrineomaticController.h"
  #include "UnitConversion.h"
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>

// Define the static member variable
BrineomaticController* BrineomaticController::_instance = nullptr;

BrineomaticController::BrineomaticController(YarrboardApp& app, RelayController& relays, ServoController& servos, StepperController& steppers) : BaseController(app, "brineomatic"),
                                                                                                                                                 wm(app, relays, servos, steppers)
{
}

bool BrineomaticController::setup()
{
  _instance = this; // Capture the instance for callbacks

  _app.protocol.registerCommand(GUEST, "start_watermaker", this, &BrineomaticController::handleStartWatermaker);
  _app.protocol.registerCommand(GUEST, "flush_watermaker", this, &BrineomaticController::handleFlushWatermaker);
  _app.protocol.registerCommand(GUEST, "pickle_watermaker", this, &BrineomaticController::handlePickleWatermaker);
  _app.protocol.registerCommand(GUEST, "depickle_watermaker", this, &BrineomaticController::handleDepickleWatermaker);
  _app.protocol.registerCommand(GUEST, "stop_watermaker", this, &BrineomaticController::handleStopWatermaker);
  _app.protocol.registerCommand(GUEST, "idle_watermaker", this, &BrineomaticController::handleIdleWatermaker);
  _app.protocol.registerCommand(GUEST, "manual_watermaker", this, &BrineomaticController::handleManualWatermaker);
  _app.protocol.registerCommand(GUEST, "set_watermaker", this, &BrineomaticController::handleSetWatermaker);
  _app.protocol.registerCommand(GUEST, "brineomatic_save_general_config", this, &BrineomaticController::handleSaveGeneralConfig);
  _app.protocol.registerCommand(GUEST, "brineomatic_save_hardware_config", this, &BrineomaticController::handleSaveHardwareConfig);
  _app.protocol.registerCommand(GUEST, "brineomatic_save_safeguards_config", this, &BrineomaticController::handleSaveSafeguardsConfig);

  wm.init();

  // Create a FreeRTOS task for the state machine
  xTaskCreatePinnedToCore(
    BrineomaticController::stateMachineTask, // Task function
    "brineomatic_sm",                        // Name of the task
    4096,                                    // Stack size
    this,                                    // Task input parameters
    2,                                       // Priority of the task
    NULL,                                    // Task handle
    1                                        // Core where the task should run
  );

  return true;
}

void BrineomaticController::loop()
{
  wm.loop();
}

void BrineomaticController::stateMachineTask(void* pvParameters)
{
  // Cast the void pointer back to our class type
  BrineomaticController* instance = static_cast<BrineomaticController*>(pvParameters);

  // Call the actual member function
  instance->stateMachine();
}

void BrineomaticController::stateMachine()
{
  while (true) {
    wm.runStateMachine();

    // Add a delay to prevent task starvation
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool BrineomaticController::loadConfigHook(JsonVariant config, char* error, size_t len)
{
  bool result = true;

  if (config["brineomatic"]) {
    JsonVariant bom = config["brineomatic"].as<JsonVariant>();

    // validate prunes invalid entries, so it's safe to load even on error.
    // we don't want a single bad config option to nuke the whole config loading.
    if (!wm.validateConfigJSON(bom, error, len)) {
      YBP.println(error);
      result = false;
    }

    wm.loadGeneralConfigJSON(bom);
    wm.loadHardwareConfigJSON(bom);
    wm.loadSafeguardsConfigJSON(bom);
  }

  return result;
}

void BrineomaticController::generateConfigHook(JsonVariant output)
{
  wm.generateConfigJSON(output);
};

void BrineomaticController::generateCapabilitiesHook(JsonVariant config)
{
  #ifdef YB_DS18B20_MOTOR_PIN
  config["brineomatic"]["motor_temperature"] = true;
  #endif

  #ifdef YB_DS18B20_WATER_PIN
  config["brineomatic"]["water_temperature"] = true;
  #endif

  #ifdef YB_PRODUCT_FLOWMETER_PIN
  config["brineomatic"]["product_flowmeter"] = true;
  #endif

  #ifdef YB_BRINE_FLOWMETER_PIN
  config["brineomatic"]["brine_flowmeter"] = true;
  #endif

  #ifdef YB_BRINE_TDS_CHANNEL
  config["brineomatic"]["brine_tds"] = true;
  #endif

  #ifdef YB_PRODUCT_TDS_CHANNEL
  config["brineomatic"]["product_tds"] = true;
  #endif

  #ifdef YB_LP_SENSOR_CHANNEL
  config["brineomatic"]["lp_sensor"] = true;
  #endif

  #ifdef YB_HP_SENSOR_CHANNEL
  config["brineomatic"]["hp_sensor"] = true;
  #endif

  #ifdef YB_HAS_MODBUS
  config["brineomatic"]["modbus"] = true;
  #endif
}

void BrineomaticController::generateUpdateHook(JsonVariant output)
{
  wm.generateUpdateJSON(output);
};

void BrineomaticController::mqttUpdateHook(MQTTController* mqtt)
{
  JsonDocument output;
  wm.generateUpdateJSON(output);

  // Convert temperature fields
  output["motor_temperature"] = convertTemperature(output["motor_temperature"], "celsius", wm.getTemperatureUnits());
  output["water_temperature"] = convertTemperature(output["water_temperature"], "celsius", wm.getTemperatureUnits());

  // Convert pressure fields
  output["filter_pressure"] = convertPressure(output["filter_pressure"], "bar", wm.getPressureUnits());
  output["membrane_pressure"] = convertPressure(output["membrane_pressure"], "bar", wm.getPressureUnits());

  // Convert volume fields
  output["volume"] = convertVolume(output["volume"], "liters", wm.getVolumeUnits());
  output["flush_volume"] = convertVolume(output["flush_volume"], "liters", wm.getVolumeUnits());

  // Convert flowrate fields
  output["product_flowrate"] = convertFlowrate(output["product_flowrate"], "lph", wm.getFlowrateUnits());
  output["brine_flowrate"] = convertFlowrate(output["brine_flowrate"], "lph", wm.getFlowrateUnits());
  output["total_flowrate"] = convertFlowrate(output["total_flowrate"], "lph", wm.getFlowrateUnits());

  mqtt->traverseJSON(output, "");
}

void BrineomaticController::haUpdateHook(MQTTController* mqtt)
{
  mqtt->publish(ha_topic_avail, "online", false);

  if (strcmp(wm.getStatus(), "IDLE") == 0)
    mqtt->publish(ha_topic_state_state, "OFF", false);
  else
    mqtt->publish(ha_topic_state_state, "ON", false);
}

void BrineomaticController::haGenerateDiscoveryHook(JsonVariant components, const char* uuid, MQTTController* mqtt)
{
  sprintf(ha_uuid, "yarrboard/%s", _cfg.local_hostname);
  sprintf(ha_topic_avail, "%s/ha/availability", ha_uuid);
  sprintf(ha_topic_cmd_state, "%s/ha/set", ha_uuid);
  sprintf(ha_topic_state_state, "%s/ha/state", ha_uuid);

  if (!_haCallbacksRegistered) {
    mqtt->onTopic(ha_topic_cmd_state, 0, &BrineomaticController::handleHACommandCallbackStatic);
    _haCallbacksRegistered = true;
  }

  // configuration object for the individual channel
  JsonObject obj = components[ha_uuid].to<JsonObject>();
  obj["platform"] = "switch";
  obj["name"] = _app.board_name;
  obj["unique_id"] = ha_uuid;
  obj["state_topic"] = ha_topic_state_state;
  obj["command_topic"] = ha_topic_cmd_state;
  obj["payload_on"] = "ON";
  obj["payload_off"] = "OFF";
  obj["icon"] = "mdi:water-sync";

  // availability is an array of objects
  JsonArray availability = obj["availability"].to<JsonArray>();
  JsonObject avail = availability.add<JsonObject>();
  avail["topic"] = ha_topic_avail;

  if (wm.hasMotorTemperature())
    haGenerateMotorTemperatureDiscovery(components);
  if (wm.hasWaterTemperature())
    haGenerateWaterTemperatureDiscovery(components);

  haGenerateStatusDiscovery(components);
  haGenerateRunResultDiscovery(components);
  haGenerateFlushResultDiscovery(components);
  haGeneratePickleResultDiscovery(components);
  haGenerateDepickleResultDiscovery(components);
  haGenerateNextFlushCountdownDiscovery(components);
  haGenerateRuntimeElapsedDiscovery(components);
  haGenerateFinishCountdownDiscovery(components);

  if (wm.hasFilterPressure())
    haGenerateFilterPressureDiscovery(components);
  if (wm.hasMembranePressure())
    haGenerateMembranePressureDiscovery(components);
  if (wm.hasProductTDS())
    haGenerateProductSalinityDiscovery(components);
  if (wm.hasBrineTDS())
    haGenerateBrineSalinityDiscovery(components);
  if (wm.hasProductFlow())
    haGenerateProductFlowrateDiscovery(components);
  if (wm.hasBrineFlow())
    haGenerateBrineFlowrateDiscovery(components);
  if (wm.hasProductFlow() || wm.hasBrineFlow())
    haGenerateTotalFlowrateDiscovery(components);

  haGenerateTankLevelDiscovery(components);
  haGenerateVolumeDiscovery(components);
  haGenerateFlushVolumeDiscovery(components);

  if (wm.hasBoostPump())
    haGenerateBoostPumpDiscovery(components);
  if (wm.hasHighPressurePump())
    haGenerateHighPressurePumpDiscovery(components);
  if (wm.hasDiverterValve())
    haGenerateDiverterValveDiscovery(components);
  if (wm.hasFlushValve())
    haGenerateFlushValveDiscovery(components);
  if (wm.hasCoolingFan())
    haGenerateCoolingFanDiscovery(components);
}

void BrineomaticController::haGenerateMotorTemperatureDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_motor_temperature", ha_uuid);
  sprintf(ha_topic_motor_temperature, "%s/motor_temperature", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Motor Temperature";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_motor_temperature;
  obj["device_class"] = "temperature";
  if (!strcmp(wm.getTemperatureUnits(), "celsius"))
    obj["unit_of_measurement"] = "°C";
  else
    obj["unit_of_measurement"] = "°F";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateWaterTemperatureDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_water_temperature", ha_uuid);
  sprintf(ha_topic_water_temperature, "%s/water_temperature", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Water Temperature";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_water_temperature;
  obj["device_class"] = "temperature";
  if (!strcmp(wm.getTemperatureUnits(), "celsius"))
    obj["unit_of_measurement"] = "°C";
  else
    obj["unit_of_measurement"] = "°F";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateStatusDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_status", ha_uuid);
  sprintf(ha_topic_status, "%s/status", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Status";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_status;
  obj["icon"] = "mdi:water-sync";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateRunResultDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_run_result", ha_uuid);
  sprintf(ha_topic_run_result, "%s/run_result", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Run Result";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_run_result;
  obj["icon"] = "mdi:water-sync";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateFlushResultDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_flush_result", ha_uuid);
  sprintf(ha_topic_flush_result, "%s/flush_result", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Flush Result";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_flush_result;
  obj["icon"] = "mdi:water-sync";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGeneratePickleResultDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_pickle_result", ha_uuid);
  sprintf(ha_topic_pickle_result, "%s/pickle_result", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Pickle Result";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_pickle_result;
  obj["icon"] = "mdi:water-sync";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateDepickleResultDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_depickle_result", ha_uuid);
  sprintf(ha_topic_depickle_result, "%s/depickle_result", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Depickle Result";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_depickle_result;
  obj["icon"] = "mdi:water-sync";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateNextFlushCountdownDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_next_flush_countdown", ha_uuid);
  sprintf(ha_topic_next_flush_countdown, "%s/next_flush_countdown", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Next Flush Countdown";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_next_flush_countdown;
  obj["device_class"] = "duration";
  obj["unit_of_measurement"] = "h";
  obj["value_template"] = "{{ (value | int / 3600000) | round(2) }}";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateRuntimeElapsedDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_runtime_elapsed", ha_uuid);
  sprintf(ha_topic_runtime_elapsed, "%s/runtime_elapsed", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Runtime Elapsed";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_runtime_elapsed;
  obj["device_class"] = "duration";
  obj["unit_of_measurement"] = "min";
  obj["value_template"] = "{{ (value | int / 60000) | round(2) }}";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateFinishCountdownDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_finish_countdown", ha_uuid);
  sprintf(ha_topic_finish_countdown, "%s/finish_countdown", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Finish Countdown";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_finish_countdown;
  obj["device_class"] = "duration";
  obj["unit_of_measurement"] = "min";
  obj["value_template"] = "{{ (value | int / 60000) | round(2) }}";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateFilterPressureDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_filter_pressure", ha_uuid);
  sprintf(ha_topic_filter_pressure, "%s/filter_pressure", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Filter Pressure";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_filter_pressure;
  obj["device_class"] = "pressure";
  if (strcmp(wm.getPressureUnits(), "kilopascal") == 0)
    obj["unit_of_measurement"] = "kPa";
  else if (strcmp(wm.getPressureUnits(), "psi") == 0)
    obj["unit_of_measurement"] = "psi";
  else
    obj["unit_of_measurement"] = "bar";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateMembranePressureDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_membrane_pressure", ha_uuid);
  sprintf(ha_topic_membrane_pressure, "%s/membrane_pressure", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Membrane Pressure";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_membrane_pressure;
  obj["device_class"] = "pressure";
  if (strcmp(wm.getPressureUnits(), "kilopascal") == 0)
    obj["unit_of_measurement"] = "kPa";
  else if (strcmp(wm.getPressureUnits(), "psi") == 0)
    obj["unit_of_measurement"] = "psi";
  else
    obj["unit_of_measurement"] = "bar";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateProductSalinityDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_product_salinity", ha_uuid);
  sprintf(ha_topic_product_salinity, "%s/product_salinity", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Product Salinity";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_product_salinity;
  obj["unit_of_measurement"] = "ppm";
  obj["suggested_display_precision"] = 0;
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateBrineSalinityDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_brine_salinity", ha_uuid);
  sprintf(ha_topic_brine_salinity, "%s/brine_salinity", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Brine Salinity";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_brine_salinity;
  obj["unit_of_measurement"] = "ppm";
  obj["suggested_display_precision"] = 0;
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateProductFlowrateDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_product_flowrate", ha_uuid);
  sprintf(ha_topic_product_flowrate, "%s/product_flowrate", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Product Flowrate";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_product_flowrate;
  obj["device_class"] = "volume_flow_rate";
  if (strcmp(wm.getFlowrateUnits(), "lph") == 0)
    obj["unit_of_measurement"] = "L/h";
  else
    obj["unit_of_measurement"] = "gal/h";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateBrineFlowrateDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_brine_flowrate", ha_uuid);
  sprintf(ha_topic_brine_flowrate, "%s/brine_flowrate", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Brine Flowrate";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_brine_flowrate;
  obj["device_class"] = "volume_flow_rate";
  if (strcmp(wm.getFlowrateUnits(), "lph") == 0)
    obj["unit_of_measurement"] = "L/h";
  else
    obj["unit_of_measurement"] = "gal/h";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateTotalFlowrateDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_total_flowrate", ha_uuid);
  sprintf(ha_topic_total_flowrate, "%s/total_flowrate", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Total Flowrate";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_total_flowrate;
  obj["device_class"] = "volume_flow_rate";
  if (strcmp(wm.getFlowrateUnits(), "lph") == 0)
    obj["unit_of_measurement"] = "L/h";
  else
    obj["unit_of_measurement"] = "gal/h";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateTankLevelDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_tank_level", ha_uuid);
  sprintf(ha_topic_tank_level, "%s/tank_level", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Tank Level";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_tank_level;
  obj["unit_of_measurement"] = "%";
  obj["value_template"] = "{{ (value | float * 100) | round(1) }}";
  obj["state_class"] = "measurement";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateVolumeDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_volume", ha_uuid);
  sprintf(ha_topic_volume, "%s/volume", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Product Volume";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_volume;
  obj["device_class"] = "volume";
  if (strcmp(wm.getVolumeUnits(), "liters") == 0)
    obj["unit_of_measurement"] = "L";
  else
    obj["unit_of_measurement"] = "gal";
  obj["state_class"] = "total_increasing";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateFlushVolumeDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_flush_volume", ha_uuid);
  sprintf(ha_topic_flush_volume, "%s/flush_volume", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "sensor";
  obj["name"] = "Flush Volume";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_flush_volume;
  obj["device_class"] = "volume";
  if (strcmp(wm.getVolumeUnits(), "liters") == 0)
    obj["unit_of_measurement"] = "L";
  else
    obj["unit_of_measurement"] = "gal";
  obj["state_class"] = "total_increasing";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateBoostPumpDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_boost_pump_on", ha_uuid);
  sprintf(ha_topic_boost_pump_on, "%s/boost_pump_on", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "binary_sensor";
  obj["name"] = "Boost Pump";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_boost_pump_on;
  obj["payload_on"] = "true";
  obj["payload_off"] = "false";
  obj["icon"] = "mdi:water-pump";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateHighPressurePumpDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_high_pressure_pump_on", ha_uuid);
  sprintf(ha_topic_high_pressure_pump_on, "%s/high_pressure_pump_on", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "binary_sensor";
  obj["name"] = "High Pressure Pump";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_high_pressure_pump_on;
  obj["payload_on"] = "true";
  obj["payload_off"] = "false";
  obj["icon"] = "mdi:water-pump";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateDiverterValveDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_diverter_valve_open", ha_uuid);
  sprintf(ha_topic_diverter_valve_open, "%s/diverter_valve_open", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "binary_sensor";
  obj["name"] = "Diverter Valve";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_diverter_valve_open;
  obj["payload_on"] = "true";
  obj["payload_off"] = "false";
  obj["icon"] = "mdi:valve";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateFlushValveDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_flush_valve_open", ha_uuid);
  sprintf(ha_topic_flush_valve_open, "%s/flush_valve_open", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "binary_sensor";
  obj["name"] = "Flush Valve";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_flush_valve_open;
  obj["payload_on"] = "true";
  obj["payload_off"] = "false";
  obj["icon"] = "mdi:valve";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::haGenerateCoolingFanDiscovery(JsonVariant doc)
{
  char unique_id[128];
  sprintf(unique_id, "%s_cooling_fan_on", ha_uuid);
  sprintf(ha_topic_cooling_fan_on, "%s/cooling_fan_on", ha_uuid);

  JsonObject obj = doc[unique_id].to<JsonObject>();
  obj["platform"] = "binary_sensor";
  obj["name"] = "Cooling Fan";
  obj["unique_id"] = unique_id;
  obj["state_topic"] = ha_topic_cooling_fan_on;
  obj["payload_on"] = "true";
  obj["payload_off"] = "false";
  obj["icon"] = "mdi:fan";
  obj["availability_topic"] = ha_topic_avail;
}

void BrineomaticController::handleHACommandCallbackStatic(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  if (_instance) {
    _instance->handleHACommandCallback(topic, payload, retain, qos, dup);
  }
}

void BrineomaticController::handleHACommandCallback(const char* topic, const char* payload, int retain, int qos, bool dup)
{
  // start and stop internally handle if we're allowed to do it.
  if (!strcmp(payload, "ON"))
    wm.start();
  else
    wm.stop();
}

void BrineomaticController::generateStatsHook(JsonVariant output)
{
  output["brineomatic"] = true;
  output["total_cycles"] = wm.getTotalCycles();
  output["total_volume"] = wm.getTotalVolume();
  output["total_runtime"] = wm.getTotalRuntime();
};

void BrineomaticController::handleStartWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (strcmp(wm.getStatus(), "IDLE"))
    return _app.protocol.generateErrorJSON(output, "Watermaker is not in IDLE mode.");

  uint64_t duration = input["duration"];
  float volume = input["volume"];

  if (duration > 0)
    wm.startDuration(duration);
  else if (volume > 0)
    wm.startVolume(volume);
  else
    wm.start();
}

void BrineomaticController::handleFlushWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  uint64_t duration = input["duration"];
  float volume = input["volume"];

  if (!strcmp(wm.getStatus(), "IDLE") || !strcmp(wm.getStatus(), "PICKLED")) {
    if (duration > 0)
      wm.flushDuration(duration);
    else if (volume > 0)
      wm.flushVolume(volume);
    else
      wm.flush();
  } else
    return _app.protocol.generateErrorJSON(output, "Watermaker is not in IDLE or PICKLED modes.");
}

void BrineomaticController::handlePickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (!input["duration"].is<JsonVariantConst>())
    return _app.protocol.generateErrorJSON(output, "'duration' is a required parameter");

  uint64_t duration = input["duration"];

  if (!duration)
    return _app.protocol.generateErrorJSON(output, "'duration' must be non-zero");

  if (!strcmp(wm.getStatus(), "IDLE"))
    wm.pickle(duration);
  else
    return _app.protocol.generateErrorJSON(output, "Watermaker is not in IDLE mode.");
}

void BrineomaticController::handleDepickleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (!input["duration"].is<JsonVariantConst>())
    return _app.protocol.generateErrorJSON(output, "'duration' is a required parameter");

  uint64_t duration = input["duration"];

  if (!duration)
    return _app.protocol.generateErrorJSON(output, "'duration' must be non-zero");

  if (!strcmp(wm.getStatus(), "PICKLED"))
    wm.depickle(duration);
  else
    return _app.protocol.generateErrorJSON(output, "Watermaker is not in PICKLED mode.");
}

void BrineomaticController::handleStopWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (!strcmp(wm.getStatus(), "RUNNING") || !strcmp(wm.getStatus(), "FLUSHING") || !strcmp(wm.getStatus(), "PICKLING") || !strcmp(wm.getStatus(), "DEPICKLING"))
    wm.stop();
  else
    return _app.protocol.generateErrorJSON(output, "Watermaker must be in RUNNING, FLUSHING, or PICKLING mode to stop.");
}

void BrineomaticController::handleIdleWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (!strcmp(wm.getStatus(), "MANUAL"))
    wm.idle();
  else
    return _app.protocol.generateErrorJSON(output, "Watermaker must be in MANUAL mode to IDLE.");
}

void BrineomaticController::handleManualWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (!strcmp(wm.getStatus(), "IDLE"))
    wm.manual();
  else
    return _app.protocol.generateErrorJSON(output, "Watermaker must be in IDLE mode to switch to MANUAL.");
}

void BrineomaticController::handleSetWatermaker(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  if (input["motor_temperature"].is<float>()) {
    float temp = input["motor_temperature"];

    if (temp < -50 || temp > 200.0)
      return _app.protocol.generateErrorJSON(output, "Motor temperature must be between -50C and 200C");

    wm.setMotorTemperature(temp);
    return;
  }

  if (input["water_temperature"].is<float>()) {
    float temp = input["water_temperature"];

    if (temp < 0.0 || temp > 50.0)
      return _app.protocol.generateErrorJSON(output, "Water temperature must be between 0C and 50C");

    wm.setWaterTemperature(temp);
    return;
  }

  if (input["tank_level"].is<float>()) {
    float level = input["tank_level"];

    if (level < 0.0 || level > 1.0)
      return _app.protocol.generateErrorJSON(output, "Tank level must be between 0.0 and 1.0");

    wm.setTankLevel(level);
    return;
  }

  if (strcmp(wm.getStatus(), "MANUAL"))
    return _app.protocol.generateErrorJSON(output, "Watermaker must be in MANUAL mode.");

  String state;

  if (input["boost_pump"]) {
    if (wm.hasBoostPump()) {
      state = input["boost_pump"] | "OFF";

      if (state.equals("TOGGLE")) {
        if (!wm.isBoostPumpOn())
          state = "ON";
      }

      if (state.equals("ON"))
        wm.enableBoostPump();
      else
        wm.disableBoostPump();
    } else
      return _app.protocol.generateErrorJSON(output, "Watermaker does not have a boost pump.");
  }

  if (input["high_pressure_pump"]) {
    if (wm.hasHighPressurePump()) {
      state = input["high_pressure_pump"] | "OFF";

      if (state.equals("TOGGLE")) {
        if (!wm.isHighPressurePumpOn())
          state = "ON";
      }

      if (state.equals("ON"))
        wm.enableHighPressurePump();
      else
        wm.disableHighPressurePump();
    } else
      return _app.protocol.generateErrorJSON(output, "Watermaker does not have a high pressure pump.");
  }

  if (input["diverter_valve"]) {
    if (wm.hasDiverterValve()) {
      state = input["diverter_valve"] | "CLOSE";

      if (state.equals("TOGGLE")) {
        if (!wm.isDiverterValveOpen())
          state = "OPEN";
      }

      if (state.equals("OPEN"))
        wm.openDiverterValve();
      else
        wm.closeDiverterValve();
    } else
      return _app.protocol.generateErrorJSON(output, "Watermaker does not have a diverter valve.");
  }

  if (input["flush_valve"]) {
    if (wm.hasFlushValve()) {
      state = input["flush_valve"] | "CLOSE";

      if (state.equals("TOGGLE")) {
        if (!wm.isFlushValveOpen())
          state = "OPEN";
      }

      if (state.equals("OPEN"))
        wm.openFlushValve();
      else
        wm.closeFlushValve();
    } else
      return _app.protocol.generateErrorJSON(output, "Watermaker does not have a flush valve.");
  }

  if (input["cooling_fan"]) {
    if (wm.hasCoolingFan()) {
      state = input["cooling_fan"] | "ON";

      if (state.equals("TOGGLE")) {
        if (!wm.isCoolingFanOn())
          state = "ON";
      }

      if (state.equals("ON"))
        wm.enableCoolingFan();
      else
        wm.disableCoolingFan();
    } else
      return _app.protocol.generateErrorJSON(output, "Watermaker does not have a cooling fan.");
  }
}

void BrineomaticController::handleSaveGeneralConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  // we need a mutable format for the validation
  JsonDocument doc;
  doc.set(input);

  char error[128];
  if (!wm.validateGeneralConfigJSON(doc, error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);

  wm.loadGeneralConfigJSON(doc);

  if (!_cfg.saveConfig(error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);
}

void BrineomaticController::handleSaveHardwareConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  // we need a mutable format for the validation
  JsonDocument doc;
  doc.set(input);

  if (strcmp(wm.getStatus(), "IDLE") && strcmp(wm.getStatus(), "MANUAL"))
    return _app.protocol.generateErrorJSON(output, "Must be in IDLE or MANUAL mode to update hardware config.");

  char error[128];
  if (!wm.validateHardwareConfigJSON(doc, error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);

  wm.loadHardwareConfigJSON(doc);

  if (!_cfg.saveConfig(error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);

  // easiest to just restart - lots of init.
  ESP.restart();
}

void BrineomaticController::handleSaveSafeguardsConfig(JsonVariantConst input, JsonVariant output, ProtocolContext context)
{
  // we need a mutable format for the validation
  JsonDocument doc;
  doc.set(input);

  char error[128];
  if (!wm.validateSafeguardsConfigJSON(doc, error, sizeof(error))) {
    return _app.protocol.generateErrorJSON(output, error);
  }

  wm.loadSafeguardsConfigJSON(doc);

  if (!_cfg.saveConfig(error, sizeof(error)))
    return _app.protocol.generateErrorJSON(output, error);
}

#endif