/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#ifdef YB_IS_BRINEOMATIC

  #include "BrineomaticController.h"
  #include <YarrboardApp.h>
  #include <YarrboardDebug.h>

BrineomaticController::BrineomaticController(YarrboardApp& app, RelayController& relays, ServoController& servos, StepperController& steppers) : BaseController(app, "brineomatic"),
                                                                                                                                                 wm(app, relays, servos, steppers)
{
}

bool BrineomaticController::setup()
{
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

void BrineomaticController::generateUpdateHook(JsonVariant output)
{
  wm.generateUpdateJSON(output);
};

void BrineomaticController::mqttUpdateHook(MQTTController* mqtt)
{
  JsonDocument output;
  wm.generateUpdateJSON(output);

  // char topic[128];
  // snprintf(topic, sizeof(topic), "%s/%s", this->channel_type, this->key);
  mqtt->traverseJSON(output, "");
}

void BrineomaticController::haUpdateHook(MQTTController* mqtt)
{
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
  if (input["water_temperature"]) {
    float temp = input["water_temperature"];
    wm.setWaterTemperature(temp);
    return;
  }

  if (input["tank_level"]) {
    float level = input["tank_level"];
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

  if (strcmp(wm.getStatus(), "IDLE"))
    return _app.protocol.generateErrorJSON(output, "Must be in IDLE mode to update hardware config.");

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