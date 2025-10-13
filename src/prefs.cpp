/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "prefs.h"
#include "protocol.h"

Preferences preferences;

bool prefs_setup()
{
  if (preferences.begin("yarrboard", false)) {
    Serial.println("Prefs OK");
    Serial.printf("There are: %u entries available in the 'yarrboard' prefs table.\n", preferences.freeEntries());

  } else {
    Serial.println("Opening Preferences failed.");
    return false;
  }

  // TODO: update this.
  // first time here?
  // is_first_boot = !preferences.isKey("is_first_boot");
  is_first_boot = true;

  // initialize error string
  char error[YB_ERROR_LENGTH] = "";

  // load our config from the json file.
  if (loadConfigFromFile(YB_BOARD_CONFIG_PATH, error)) {
    Serial.println("Configuration OK");
    return true;
  } else {
    Serial.println(error);
    return false;
  }

  return false;
}

bool loadConfigFromFile(const char* file, char* error)
{

  // sanity check on LittleFS
  if (!LittleFS.begin()) {
    snprintf(error, YB_ERROR_LENGTH, "LittleFS mount failed");
    return false;
  }

  // open file
  File configFile = LittleFS.open(file, "r");
  if (!configFile || !configFile.available()) {
    snprintf(error, YB_ERROR_LENGTH, "Could not open file: %s", file);
    return false;
  }

  // get size and check reasonableness
  size_t size = configFile.size();
  if (size == 0) {
    snprintf(error, YB_ERROR_LENGTH, "File %s is empty", file);
    configFile.close();
    return false;
  }
  if (size > 10000) { // arbitrary limit to prevent large loads
    snprintf(error, YB_ERROR_LENGTH, "File %s too large (%u bytes)", file, (unsigned int)size);
    configFile.close();
    return false;
  }

  // read into buffer
  std::unique_ptr<char[]> buf(new (std::nothrow) char[size + 1]);
  if (!buf) {
    snprintf(error, YB_ERROR_LENGTH, "Memory allocation failed for %u bytes", (unsigned int)size);
    configFile.close();
    return false;
  }

  size_t bytesRead = configFile.readBytes(buf.get(), size);
  configFile.close();
  buf[bytesRead] = '\0';

  if (bytesRead != size) {
    snprintf(error, YB_ERROR_LENGTH, "Read size mismatch: expected %u, got %u", (unsigned int)size, (unsigned int)bytesRead);
    return false;
  }

  // parse JSON
  JsonDocument doc; // adjust to match your configuration complexity
  DeserializationError err = deserializeJson(doc, buf.get());

  if (err) {
    snprintf(error, YB_ERROR_LENGTH, "JSON parse error: %s", err.c_str());
    return false;
  }

  // sanity check: ensure root object
  if (!doc.is<JsonObject>()) {
    snprintf(error, YB_ERROR_LENGTH, "Root element is not a JSON object");
    return false;
  }

  // if we have an existing config file, no need for first round stuff.
  is_first_boot = false;

  return loadConfigFromJSON(doc, error);
}

bool loadConfigFromJSON(JsonVariantConst config, char* error)
{
  if (!config["network"]) {
    snprintf(error, YB_ERROR_LENGTH, "Missing 'network' config");
    return false;
  }
  if (!loadNetworkConfigFromJSON(config["network"], error))
    return false;

  if (!config["app"]) {
    snprintf(error, YB_ERROR_LENGTH, "Missing 'app' config");
    return false;
  }
  if (!loadAppConfigFromJSON(config["app"], error))
    return false;

  if (!config["board"]) {
    snprintf(error, YB_ERROR_LENGTH, "Missing 'board' config");
    return false;
  }
  if (!loadBoardConfigFromJSON(config["board"], error))
    return false;

  return true;
}

bool loadNetworkConfigFromJSON(JsonVariantConst config, char* error)
{
  const char* value;

  value = config["local_hostname"].as<const char*>();
  snprintf(local_hostname, YB_HOSTNAME_LENGTH, "%s", (value && *value) ? value : YB_DEFAULT_HOSTNAME);

  value = config["wifi_ssid"].as<const char*>();
  snprintf(wifi_ssid, YB_WIFI_SSID_LENGTH, "%s", (value && *value) ? value : YB_DEFAULT_AP_SSID);

  value = config["wifi_pass"].as<const char*>();
  snprintf(wifi_pass, YB_WIFI_PASSWORD_LENGTH, "%s", (value && *value) ? value : YB_DEFAULT_AP_PASS);

  value = config["wifi_mode"].as<const char*>();
  snprintf(wifi_mode, YB_WIFI_MODE_LENGTH, "%s", (value && *value) ? value : "ap");

  value = config["uuid"].as<const char*>();
  snprintf(uuid, YB_UUID_LENGTH, "%s", (value && *value) ? value : "");

  return true;
}

bool loadAppConfigFromJSON(JsonVariantConst config, char* error)
{
  const char* value;

  value = config["admin_user"].as<const char*>();
  snprintf(admin_user, YB_USERNAME_LENGTH, "%s", (value && *value) ? value : "admin");

  value = config["admin_pass"].as<const char*>();
  snprintf(admin_pass, YB_PASSWORD_LENGTH, "%s", (value && *value) ? value : "admin");

  value = config["guest_user"].as<const char*>();
  snprintf(guest_user, YB_USERNAME_LENGTH, "%s", (value && *value) ? value : "admin");

  value = config["guest_pass"].as<const char*>();
  snprintf(guest_pass, YB_PASSWORD_LENGTH, "%s", (value && *value) ? value : "admin");

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
    else if (strcmp(value, "guest"))
      app_default_role = GUEST;
  }
  serial_role = app_default_role;
  api_role = app_default_role;

  app_enable_mfd = config["app_enable_mfd"];
  app_enable_api = config["app_enable_api"];
  app_enable_serial = config["app_enable_serial"];
  app_enable_ota = config["app_enable_ota"];
  app_enable_ssl = config["app_enable_ssl"];

  return true;
}

bool loadBoardConfigFromJSON(JsonVariantConst config, char* error)
{
  const char* value;

  value = config["name"].as<const char*>();
  snprintf(board_name, YB_BOARD_NAME_LENGTH, "%s", (value && *value) ? value : "Yarrboard");

#ifdef YB_HAS_ADC_CHANNELS
  if (!loadChannelsConfigFromJSON("adc", adc_channels, config, error, YB_ERROR_LENGTH))
    return false;
#endif

  return true;
}