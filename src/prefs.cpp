/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "prefs.h"

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

  return true;
}

bool loadAppConfigFromJSON(JsonVariantConst config, char* error)
{
  TRACE();
  return true;
}

bool loadBoardConfigFromJSON(JsonVariantConst config, char* error)
{
  TRACE();
  return true;
}