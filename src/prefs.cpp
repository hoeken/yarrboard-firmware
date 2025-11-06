/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "prefs.h"
#include "debug.h"

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS
  #include "digital_input_channel.h"
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  #include "relay_channel.h"
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  #include "servo_channel.h"
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  #include "stepper_channel.h"
#endif

String arduino_version = String(ESP_ARDUINO_VERSION_MAJOR) + "." +
                         String(ESP_ARDUINO_VERSION_MINOR) + "." +
                         String(ESP_ARDUINO_VERSION_PATCH);

Preferences preferences;

bool is_first_boot = true;

bool prefs_setup()
{
  if (preferences.begin("yarrboard", false)) {
    YBP.println("Prefs OK");
    YBP.printf("There are: %u entries available in the 'yarrboard' prefs table.\n", preferences.freeEntries());

  } else {
    YBP.println("Opening Preferences failed.");
    return false;
  }

  // default to first time, prove it later
  is_first_boot = true;

  // initialize error string
  char error[YB_ERROR_LENGTH] = "";

  // start with defaults
  initializeChannels();

  // load our config from the json file.
  if (loadConfigFromFile(YB_BOARD_CONFIG_PATH, error, sizeof(error))) {
    YBP.println("Configuration OK");
    return true;
  } else {
    YBP.printf("CONFIG ERROR: %s\n", error);
    return false;
  }

  return false;
}

void initializeChannels()
{
#ifdef YB_HAS_ADC_CHANNELS
  initChannels(adc_channels);
#endif

#ifdef YB_HAS_PWM_CHANNELS
  initChannels(pwm_channels);
#endif

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS
  initChannels(digital_input_channels);
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  initChannels(relay_channels);
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  initChannels(servo_channels);
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  initChannels(stepper_channels);
#endif
}

bool saveConfig(char* error, size_t len)
{
  // our doc to store.
  JsonDocument config;

  // generate a full new document each time
  generateFullConfigJSON(config);

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(config);
  char* jsonBuffer = (char*)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // now serialize it to the buffer
  if (jsonBuffer != NULL) {
    serializeJson(config, jsonBuffer, jsonSize + 1);
  } else {
    free(jsonBuffer);
    snprintf(error, len, "serializeJson() failed to create buffer of size %d", jsonSize);
    return false;
  }

  // write our config to local storage
  File fp = LittleFS.open(YB_BOARD_CONFIG_PATH, "w");
  if (!fp) {
    snprintf(error, len, "Failed to open %s for writing", YB_BOARD_CONFIG_PATH);
    return false;
  }

  // check write result
  size_t bytesWritten = fp.print((char*)jsonBuffer);
  if (bytesWritten == 0) {
    fp.close();
    strncpy(error, "Failed to write JSON data to file", len);
    return false;
  }

  // flush data (no return value, but still good to call)
  fp.flush();
  fp.close();

  // confirm file exists and has non-zero length
  if (!LittleFS.exists(YB_BOARD_CONFIG_PATH)) {
    strncpy(error, "File not found after write", len);
    return false;
  }

  // check for size and opening
  File verify = LittleFS.open(YB_BOARD_CONFIG_PATH, "r");
  if (!verify || verify.size() == 0) {
    verify.close();
    strncpy(error, "Wrote file but it appears empty or unreadable", len);
    return false;
  }
  verify.close();

  // free up our memory
  free(jsonBuffer);

  return true;
}

void generateFullConfigJSON(JsonVariant output)
{
  // our board specific configuration
  JsonObject board = output["board"].to<JsonObject>();
  generateBoardConfigJSON(board);

  // yarrboard application specific configuration
  JsonObject app = output["app"].to<JsonObject>();
  generateAppConfigJSON(app);
  app.remove("msg");

  // network connection specific configuration
  JsonObject network = output["network"].to<JsonObject>();
  generateNetworkConfigJSON(network);
  network.remove("msg");
}

void generateBoardConfigJSON(JsonVariant output)
{
  // our identifying info
  output["firmware_version"] = YB_FIRMWARE_VERSION;
  output["hardware_version"] = YB_HARDWARE_VERSION;
  output["esp_idf_version"] = esp_get_idf_version();
  output["arduino_version"] = arduino_version;
  output["psychic_http_version"] = PSYCHIC_VERSION_STR;
  output["name"] = board_name;
  output["uuid"] = uuid;

// output / pwm channels
#ifdef YB_HAS_PWM_CHANNELS
  JsonArray channels = output["pwm"].to<JsonArray>();
  for (auto& ch : pwm_channels) {
    JsonObject jo = channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS
  JsonArray channels = output["dio"].to<JsonArray>();
  for (auto& ch : digital_input_channels) {
    JsonObject jo = channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  JsonArray r_channels = output["relay"].to<JsonArray>();
  for (auto& ch : relay_channels) {
    JsonObject jo = r_channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  JsonArray s_channels = output["servo"].to<JsonArray>();
  for (auto& ch : servo_channels) {
    JsonObject jo = s_channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  JsonArray st_channels = output["stepper"].to<JsonArray>();
  for (auto& ch : stepper_channels) {
    JsonObject jo = st_channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

// input / analog ADC channels
#ifdef YB_HAS_ADC_CHANNELS
  output["adc_resolution"] = YB_ADC_RESOLUTION;
  JsonArray channels = output["adc"].to<JsonArray>();
  for (auto& ch : adc_channels) {
    JsonObject jo = channels.add<JsonObject>();
    ch.generateConfig(jo);
  }
#endif

#ifdef YB_IS_BRINEOMATIC
  JsonObject bom = output["brineomatic"].to<JsonObject>();
  bom["has_boost_pump"] = wm.hasBoostPump();
  bom["has_high_pressure_pump"] = wm.hasHighPressurePump();
  bom["has_diverter_valve"] = wm.hasDiverterValve();
  bom["has_flush_valve"] = wm.hasFlushValve();
  bom["has_cooling_fan"] = wm.hasCoolingFan();
  bom["tank_capacity"] = wm.getTankCapacity();
#endif
}

void generateAppConfigJSON(JsonVariant output)
{
  // our identifying info
  output["default_role"] = getRoleText(app_default_role);
  output["admin_user"] = admin_user;
  output["admin_pass"] = admin_pass;
  output["guest_user"] = guest_user;
  output["guest_pass"] = guest_pass;
  output["app_update_interval"] = app_update_interval;
  output["app_enable_mfd"] = app_enable_mfd;
  output["app_enable_api"] = app_enable_api;
  output["app_enable_serial"] = app_enable_serial;
  output["app_enable_ota"] = app_enable_ota;
  output["app_enable_ssl"] = app_enable_ssl;
  output["app_enable_mqtt"] = app_enable_mqtt;
  output["app_enable_ha_integration"] = app_enable_ha_integration;
  output["app_use_hostname_as_mqtt_uuid"] = app_use_hostname_as_mqtt_uuid;
  output["mqtt_server"] = mqtt_server;
  output["mqtt_user"] = mqtt_user;
  output["mqtt_pass"] = mqtt_pass;
  output["mqtt_cert"] = mqtt_cert;
  output["server_cert"] = server_cert;
  output["server_key"] = server_key;
}

void generateNetworkConfigJSON(JsonVariant output)
{
  // our identifying info
  output["wifi_mode"] = wifi_mode;
  output["wifi_ssid"] = wifi_ssid;
  output["wifi_pass"] = wifi_pass;
  output["local_hostname"] = local_hostname;
}

bool loadConfigFromFile(const char* file, char* error, size_t len)
{
  // sanity check on LittleFS
  if (!LittleFS.begin()) {
    snprintf(error, len, "LittleFS mount failed");
    return false;
  }

  // open file
  File configFile = LittleFS.open(file, "r");
  if (!configFile || !configFile.available()) {
    snprintf(error, len, "Could not open file: %s", file);
    return false;
  }

  // get size and check reasonableness
  size_t size = configFile.size();
  if (size == 0) {
    snprintf(error, len, "File %s is empty", file);
    configFile.close();
    return false;
  }
  if (size > 10000) { // arbitrary limit to prevent large loads
    snprintf(error, len, "File %s too large (%u bytes)", file, (unsigned int)size);
    configFile.close();
    return false;
  }

  // read into buffer
  char* buf = (char*)malloc(size + 1);
  if (!buf) {
    snprintf(error, len, "Memory allocation failed for %u bytes", (unsigned int)size);
    configFile.close();
    return false;
  }

  size_t bytesRead = configFile.readBytes(buf, size);
  configFile.close();
  buf[bytesRead] = '\0';

  if (bytesRead != size) {
    snprintf(error, len, "Read size mismatch: expected %u, got %u", (unsigned int)size, (unsigned int)bytesRead);
    return false;
  }

  // parse JSON
  JsonDocument doc; // adjust to match your configuration complexity
  DeserializationError err = deserializeJson(doc, buf);

  if (err) {
    snprintf(error, len, "JSON parse error: %s", err.c_str());
    return false;
  }

  // sanity check: ensure root object
  if (!doc.is<JsonObject>()) {
    snprintf(error, len, "Root element is not a JSON object");
    return false;
  }

  // if we have an existing config file, no need for first round stuff.
  is_first_boot = false;

  return loadConfigFromJSON(doc, error, len);
}

bool loadConfigFromJSON(JsonVariantConst config, char* error, size_t len)
{
  if (!config["network"]) {
    snprintf(error, len, "Missing 'network' config");
    return false;
  }
  if (!loadNetworkConfigFromJSON(config["network"], error, len))
    return false;

  if (!config["app"]) {
    snprintf(error, len, "Missing 'app' config");
    return false;
  }
  if (!loadAppConfigFromJSON(config["app"], error, len))
    return false;

  if (!config["board"]) {
    snprintf(error, len, "Missing 'board' config");
    return false;
  }
  if (!loadBoardConfigFromJSON(config["board"], error, len))
    return false;

  return true;
}

bool loadNetworkConfigFromJSON(JsonVariantConst config, char* error, size_t len)
{
  const char* value;

  value = config["local_hostname"].as<const char*>();
  snprintf(local_hostname, sizeof(local_hostname), "%s", (value && *value) ? value : YB_DEFAULT_HOSTNAME);

  value = config["wifi_ssid"].as<const char*>();
  snprintf(wifi_ssid, sizeof(wifi_ssid), "%s", (value && *value) ? value : YB_DEFAULT_AP_SSID);

  value = config["wifi_pass"].as<const char*>();
  snprintf(wifi_pass, sizeof(wifi_pass), "%s", (value && *value) ? value : YB_DEFAULT_AP_PASS);

  value = config["wifi_mode"].as<const char*>();
  snprintf(wifi_mode, sizeof(wifi_mode), "%s", (value && *value) ? value : "ap");

  return true;
}

bool loadAppConfigFromJSON(JsonVariantConst config, char* error, size_t len)
{
  const char* value;

  value = config["admin_user"].as<const char*>();
  snprintf(admin_user, sizeof(admin_user), "%s", (value && *value) ? value : "admin");

  value = config["admin_pass"].as<const char*>();
  snprintf(admin_pass, sizeof(admin_pass), "%s", (value && *value) ? value : "admin");

  value = config["guest_user"].as<const char*>();
  snprintf(guest_user, sizeof(guest_user), "%s", (value && *value) ? value : "guest");

  value = config["guest_pass"].as<const char*>();
  snprintf(guest_pass, sizeof(guest_pass), "%s", (value && *value) ? value : "guest");

  value = config["mqtt_server"].as<const char*>();
  snprintf(mqtt_server, sizeof(mqtt_server), "%s", (value && *value) ? value : "");

  value = config["mqtt_user"].as<const char*>();
  snprintf(mqtt_user, sizeof(mqtt_user), "%s", (value && *value) ? value : "");

  value = config["mqtt_pass"].as<const char*>();
  snprintf(mqtt_pass, sizeof(mqtt_user), "%s", (value && *value) ? value : "");

  mqtt_cert = config["mqtt_cert"] | "";

  if (config["app_update_interval"].is<unsigned int>()) {
    app_update_interval = config["app_update_interval"] | 1000;
    app_update_interval = max(100u, app_update_interval);
    app_update_interval = min(10000u, app_update_interval);
  } else {
    app_update_interval = 1000;
  }

  // what is our default role?
  app_default_role = NOBODY;
  value = config["default_role"].as<const char*>();
  if (value && *value) {
    if (!strcmp(value, "admin"))
      app_default_role = ADMIN;
    else if (!strcmp(value, "guest"))
      app_default_role = GUEST;
  }
  serial_role = app_default_role;
  api_role = app_default_role;

  app_enable_mfd = config["app_enable_mfd"];
  app_enable_api = config["app_enable_api"];
  app_enable_serial = config["app_enable_serial"];
  app_enable_ota = config["app_enable_ota"];
  app_enable_ssl = config["app_enable_ssl"];
  app_enable_mqtt = config["app_enable_mqtt"];
  app_enable_ha_integration = config["app_enable_ha_integration"];
  app_use_hostname_as_mqtt_uuid = config["app_use_hostname_as_mqtt_uuid"];

  server_cert = config["server_cert"].as<String>();
  server_key = config["server_key"].as<String>();

  return true;
}

bool loadBoardConfigFromJSON(JsonVariantConst config, char* error, size_t len)
{
  const char* value;

  value = config["name"].as<const char*>();
  snprintf(board_name, sizeof(board_name), "%s", (value && *value) ? value : YB_BOARD_NAME);

#ifdef YB_HAS_ADC_CHANNELS
  if (!loadChannelsConfigFromJSON("adc", adc_channels, config, error, len))
    return false;
#endif

#ifdef YB_HAS_PWM_CHANNELS
  if (!loadChannelsConfigFromJSON("pwm", pwm_channels, config, error, len))
    return false;
#endif

#ifdef YB_HAS_DIGITAL_INPUT_CHANNELS
  if (!loadChannelsConfigFromJSON("dio", digital_input_channels, config, error, len))
    return false;
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  if (!loadChannelsConfigFromJSON("relay", relay_channels, config, error, len))
    return false;
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  if (!loadChannelsConfigFromJSON("servo", servo_channels, config, error, len))
    return false;
#endif

#ifdef YB_HAS_STEPPER_CHANNELS
  if (!loadChannelsConfigFromJSON("stepper", stepper_channels, config, error, len))
    return false;
#endif

  return true;
}