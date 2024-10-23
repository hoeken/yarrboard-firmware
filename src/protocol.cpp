/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "protocol.h"

char board_name[YB_BOARD_NAME_LENGTH] = "Yarrboard";
char admin_user[YB_USERNAME_LENGTH] = "admin";
char admin_pass[YB_PASSWORD_LENGTH] = "admin";
char guest_user[YB_USERNAME_LENGTH] = "guest";
char guest_pass[YB_PASSWORD_LENGTH] = "guest";
unsigned int app_update_interval = 500;
bool app_enable_mfd = true;
bool app_enable_api = true;
bool app_enable_serial = false;
bool app_enable_ssl = false;
bool is_serial_authenticated = false;
UserRole app_default_role = NOBODY;
UserRole serial_role = NOBODY;
UserRole api_role = NOBODY;
String app_theme = "light";
float globalBrightness = 1.0;

// for tracking our message loop
unsigned long previousMessageMillis = 0;

unsigned int receivedMessages = 0;
unsigned int receivedMessagesPerSecond = 0;
unsigned long totalReceivedMessages = 0;
unsigned int sentMessages = 0;
unsigned int sentMessagesPerSecond = 0;
unsigned long totalSentMessages = 0;

unsigned int websocketClientCount = 0;
unsigned int httpClientCount = 0;

String arduino_version = String(ESP_ARDUINO_VERSION_MAJOR) + "." +
                         String(ESP_ARDUINO_VERSION_MINOR) + "." +
                         String(ESP_ARDUINO_VERSION_PATCH);

void protocol_setup() {
  // look up our board name
  if (preferences.isKey("boardName"))
    strlcpy(board_name, preferences.getString("boardName").c_str(),
            sizeof(board_name));

  // look up our username/password
  if (preferences.isKey("admin_user"))
    strlcpy(admin_user, preferences.getString("admin_user").c_str(),
            sizeof(admin_user));
  if (preferences.isKey("admin_pass"))
    strlcpy(admin_pass, preferences.getString("admin_pass").c_str(),
            sizeof(admin_pass));
  if (preferences.isKey("guest_user"))
    strlcpy(guest_user, preferences.getString("guest_user").c_str(),
            sizeof(guest_user));
  if (preferences.isKey("guest_pass"))
    strlcpy(guest_pass, preferences.getString("guest_pass").c_str(),
            sizeof(guest_pass));
  if (preferences.isKey("appUpdateInter"))
    app_update_interval = preferences.getUInt("appUpdateInter");
  if (preferences.isKey("appDefaultRole")) {
    String role = preferences.getString("appDefaultRole");
    if (role == "admin")
      app_default_role = ADMIN;
    else if (role == "guest")
      app_default_role = GUEST;
    else
      app_default_role = NOBODY;

    serial_role = app_default_role;
    api_role = app_default_role;
  }
  if (preferences.isKey("appEnableMFD"))
    app_enable_mfd = preferences.getBool("appEnableMFD");
  if (preferences.isKey("appEnableApi"))
    app_enable_api = preferences.getBool("appEnableApi");
  if (preferences.isKey("appEnableSerial"))
    app_enable_serial = preferences.getBool("appEnableSerial");

  // if (preferences.isKey("brightness"))
  //   globalBrightness = preferences.getFloat("brightness");

  // send serial a config off the bat
  if (app_enable_serial) {
    JsonDocument output;

    generateConfigJSON(output);
    serializeJson(output, Serial);
  }
}

void protocol_loop() {
  // lookup our info periodically
  unsigned int messageDelta = millis() - previousMessageMillis;
  if (messageDelta >= 1000) {
// TODO: move this to pwm loop
#ifdef YB_HAS_PWM_CHANNELS
    // update our averages, etc.
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++)
      pwm_channels[i].calculateAverages(messageDelta);
#endif

// TODO: move this to bus voltage loop
#ifdef YB_HAS_BUS_VOLTAGE
    busVoltage = getBusVoltage();
#endif

    // for keeping track.
    receivedMessagesPerSecond = receivedMessages;
    receivedMessages = 0;
    sentMessagesPerSecond = sentMessages;
    sentMessages = 0;

    previousMessageMillis = millis();
  }

  // any serial port customers?
  if (app_enable_serial) {
    if (Serial.available() > 0)
      handleSerialJson();
  }
}

void handleSerialJson() {
  JsonDocument input;
  DeserializationError err = deserializeJson(input, Serial);

  // StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  JsonDocument output;

  // ignore newlines with serial.
  if (err) {
    if (strcmp(err.c_str(), "EmptyInput")) {
      char error[64];
      sprintf(error, "deserializeJson() failed with code %s", err.c_str());
      generateErrorJSON(output, error);
      serializeJson(output, Serial);
    }
  } else {
    handleReceivedJSON(input, output, YBP_MODE_SERIAL);

    // we can have empty responses
    if (output.size()) {
      serializeJson(output, Serial);

      sentMessages++;
      totalSentMessages++;
    }
  }
}

void handleReceivedJSON(JsonVariantConst input, JsonVariant output, YBMode mode,
                        PsychicWebSocketClient *connection) {
  // make sure its correct
  if (!input["cmd"].is<String>())
    return generateErrorJSON(output, "'cmd' is a required parameter.");

  // what is your command?
  const char *cmd = input["cmd"];

  // let the client keep track of messages
  if (input["msgid"].is<unsigned int>()) {
    unsigned int msgid = input["msgid"];
    output["status"] = "ok";
    output["msgid"] = msgid;
  }

  // keep track!
  receivedMessages++;
  totalReceivedMessages++;

  // what would you say you do around here?
  UserRole role = getUserRole(input, mode, connection);

  // only pages with no login requirements
  if (!strcmp(cmd, "login"))
    return handleLogin(input, output, mode, connection);
  else if (!strcmp(cmd, "ping"))
    return generatePongJSON(output);
  else if (!strcmp(cmd, "hello"))
    return generateHelloJSON(output, role);

  // TODO: we don't actually need to require login
  // need to be logged in from here on out.
  //  if (!isLoggedIn(input, mode, connection))
  //      return generateLoginRequiredJSON(output);

  // commands for using the boards
  if (role == GUEST || role == ADMIN) {
    if (!strcmp(cmd, "get_config"))
      return generateConfigJSON(output);
    else if (!strcmp(cmd, "get_stats"))
      return generateStatsJSON(output);
    else if (!strcmp(cmd, "get_update"))
      return generateUpdateJSON(output);
    else if (!strcmp(cmd, "set_pwm_channel"))
      return handleSetPWMChannel(input, output);
    else if (!strcmp(cmd, "toggle_pwm_channel"))
      return handleTogglePWMChannel(input, output);
    else if (!strcmp(cmd, "fade_pwm_channel"))
      return handleFadePWMChannel(input, output);
    else if (!strcmp(cmd, "set_switch"))
      return handleSetSwitch(input, output);
    else if (!strcmp(cmd, "set_rgb"))
      return handleSetRGB(input, output);
    else if (!strcmp(cmd, "set_theme"))
      return handleSetTheme(input, output);
    else if (!strcmp(cmd, "set_brightness"))
      return handleSetBrightness(input, output);
    else if (!strcmp(cmd, "logout"))
      return handleLogout(input, output, mode, connection);
  }

  // commands for changing settings
  if (role == ADMIN) {
    if (!strcmp(cmd, "set_boardname"))
      return handleSetBoardName(input, output);
    else if (!strcmp(cmd, "get_network_config"))
      return generateNetworkConfigJSON(output);
    else if (!strcmp(cmd, "set_network_config"))
      return handleSetNetworkConfig(input, output);
    else if (!strcmp(cmd, "get_app_config"))
      return generateAppConfigJSON(output);
    else if (!strcmp(cmd, "set_app_config"))
      return handleSetAppConfig(input, output);
    else if (!strcmp(cmd, "restart"))
      return handleRestart(input, output);
    else if (!strcmp(cmd, "factory_reset"))
      return handleFactoryReset(input, output);
    else if (!strcmp(cmd, "ota_start"))
      return handleOTAStart(input, output);
    else if (!strcmp(cmd, "config_pwm_channel"))
      return handleConfigPWMChannel(input, output);
    else if (!strcmp(cmd, "config_switch"))
      return handleConfigSwitch(input, output);
    else if (!strcmp(cmd, "config_adc"))
      return handleConfigADC(input, output);
    else if (!strcmp(cmd, "config_rgb"))
      return handleConfigRGB(input, output);
  }

  // if we got here, no bueno.
  String error = "Invalid command: " + String(cmd);
  return generateErrorJSON(output, error.c_str());
}

const char *getRoleText(UserRole role) {
  if (role == ADMIN)
    return "admin";
  else if (role == GUEST)
    return "guest";
  else
    return "nobody";
}

void generateHelloJSON(JsonVariant output, UserRole role) {
  output["msg"] = "hello";
  output["role"] = getRoleText(role);
  output["default_role"] = getRoleText(app_default_role);
  output["theme"] = app_theme;
  output["brightness"] = globalBrightness;
  output["firmware_version"] = YB_FIRMWARE_VERSION;
}

void handleSetBoardName(JsonVariantConst input, JsonVariant output) {
  if (!input["value"].is<String>())
    return generateErrorJSON(output, "'value' is a required parameter");

  // is it too long?
  if (strlen(input["value"]) > YB_BOARD_NAME_LENGTH - 1) {
    char error[50];
    sprintf(error, "Maximum board name length is %s characters.",
            YB_BOARD_NAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // update variable
  strlcpy(board_name, input["value"] | "Yarrboard", sizeof(board_name));

  // save to our storage
  preferences.putString("boardName", board_name);

  // give them the updated config
  return generateConfigJSON(output);
}

void handleSetNetworkConfig(JsonVariantConst input, JsonVariant output) {
  // clear our first boot flag since they submitted the network page.
  is_first_boot = false;

  char error[50];

  // error checking
  if (!input["wifi_mode"].is<String>())
    return generateErrorJSON(output, "'wifi_mode' is a required parameter");
  if (!input["wifi_ssid"].is<String>())
    return generateErrorJSON(output, "'wifi_ssid' is a required parameter");
  if (!input["wifi_pass"].is<String>())
    return generateErrorJSON(output, "'wifi_pass' is a required parameter");
  if (!input["local_hostname"].is<String>())
    return generateErrorJSON(output,
                             "'local_hostname' is a required parameter");

  // is it too long?
  if (strlen(input["wifi_ssid"]) > YB_WIFI_SSID_LENGTH - 1) {
    sprintf(error, "Maximum wifi ssid length is %s characters.",
            YB_WIFI_SSID_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  if (strlen(input["wifi_pass"]) > YB_WIFI_PASSWORD_LENGTH - 1) {
    sprintf(error, "Maximum wifi password length is %s characters.",
            YB_WIFI_PASSWORD_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  if (strlen(input["local_hostname"]) > YB_HOSTNAME_LENGTH - 1) {
    sprintf(error, "Maximum hostname length is %s characters.",
            YB_HOSTNAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // get our data
  char new_wifi_mode[16];
  char new_wifi_ssid[YB_WIFI_SSID_LENGTH];
  char new_wifi_pass[YB_WIFI_PASSWORD_LENGTH];

  strlcpy(new_wifi_mode, input["wifi_mode"] | "ap", sizeof(new_wifi_mode));
  strlcpy(new_wifi_ssid, input["wifi_ssid"] | "SSID", sizeof(new_wifi_ssid));
  strlcpy(new_wifi_pass, input["wifi_pass"] | "PASS", sizeof(new_wifi_pass));
  strlcpy(local_hostname, input["local_hostname"] | "yarrboard",
          sizeof(local_hostname));

  // no special cases here.
  preferences.putString("local_hostname", local_hostname);

  // make sure we can connect before we save
  if (!strcmp(new_wifi_mode, "client")) {
    // did we change username/password?
    if (strcmp(new_wifi_ssid, wifi_ssid) || strcmp(new_wifi_pass, wifi_pass)) {
      // try connecting.
      if (connectToWifi(new_wifi_ssid, new_wifi_pass)) {
        // changing modes?
        if (!strcmp(wifi_mode, "ap"))
          WiFi.softAPdisconnect();

        // save to flash
        preferences.putString("wifi_mode", new_wifi_mode);
        preferences.putString("wifi_ssid", new_wifi_ssid);
        preferences.putString("wifi_pass", new_wifi_pass);

        // save for local use
        strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
        strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
        strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));

        // let the client know.
        return generateSuccessJSON(output, "Connected to new WiFi.");
      }
      // nope, setup our wifi back to default.
      else
        return generateErrorJSON(output, "Can't connect to new WiFi.");
    } else
      return generateSuccessJSON(output, "Network settings updated.");
  }
  // okay, AP mode is easier
  else {
    // changing modes?
    // if (wifi_mode.equals("client"))
    //   WiFi.disconnect();

    // save to flash
    preferences.putString("wifi_mode", new_wifi_mode);
    preferences.putString("wifi_ssid", new_wifi_ssid);
    preferences.putString("wifi_pass", new_wifi_pass);

    // save for local use.
    strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
    strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
    strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));

    // switch us into AP mode
    setupWifi();

    return generateSuccessJSON(
        output, "AP mode successful, please connect to new network.");
  }
}

void handleSetAppConfig(JsonVariantConst input, JsonVariant output) {
  bool old_app_enable_ssl = app_enable_ssl;

  if (!input["admin_user"].is<String>())
    return generateErrorJSON(output, "'admin_user' is a required parameter");
  if (!input["admin_pass"].is<String>())
    return generateErrorJSON(output, "'admin_pass' is a required parameter");
  if (!input["guest_user"].is<String>())
    return generateErrorJSON(output, "'guest_user' is a required parameter");
  if (!input["guest_pass"].is<String>())
    return generateErrorJSON(output, "'guest_pass' is a required parameter");
  if (!input["default_role"].is<String>())
    return generateErrorJSON(output, "'default_role' is a required parameter");

  // username length checker
  if (strlen(input["admin_user"]) > YB_USERNAME_LENGTH - 1) {
    char error[60];
    sprintf(error, "Maximum admin username length is %s characters.",
            YB_USERNAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // password length checker
  if (strlen(input["admin_pass"]) > YB_PASSWORD_LENGTH - 1) {
    char error[60];
    sprintf(error, "Maximum admin password length is %s characters.",
            YB_PASSWORD_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // username length checker
  if (strlen(input["guest_user"]) > YB_USERNAME_LENGTH - 1) {
    char error[60];
    sprintf(error, "Maximum guest username length is %s characters.",
            YB_USERNAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // password length checker
  if (strlen(input["guest_pass"]) > YB_PASSWORD_LENGTH - 1) {
    char error[60];
    sprintf(error, "Maximum guest password length is %s characters.",
            YB_PASSWORD_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // get our data
  strlcpy(admin_user, input["admin_user"] | "admin", sizeof(admin_user));
  strlcpy(admin_pass, input["admin_pass"] | "admin", sizeof(admin_pass));
  strlcpy(guest_user, input["guest_user"] | "guest", sizeof(guest_user));
  strlcpy(guest_pass, input["guest_pass"] | "guest", sizeof(guest_pass));

  if (!strcmp(input["default_role"], "admin"))
    app_default_role = ADMIN;
  else if (!strcmp(input["default_role"], "guest"))
    app_default_role = GUEST;
  else
    app_default_role = NOBODY;

  app_enable_mfd = input["app_enable_mfd"];
  app_enable_api = input["app_enable_api"];
  app_enable_serial = input["app_enable_serial"];
  app_enable_ssl = input["app_enable_ssl"];

  if (input["app_update_interval"].is<int>()) {
    app_update_interval = input["app_update_interval"] | 500;
    app_update_interval = max(100, (int)app_update_interval);
    app_update_interval = min(5000, (int)app_update_interval);
  }

  // no special cases here.
  preferences.putString("admin_user", admin_user);
  preferences.putString("admin_pass", admin_pass);
  preferences.putString("guest_user", guest_user);
  preferences.putString("guest_pass", guest_pass);
  preferences.putUInt("appUpdateInter", app_update_interval);
  preferences.putString("appDefaultRole", getRoleText(app_default_role));
  preferences.putBool("appEnableMFD", app_enable_mfd);
  preferences.putBool("appEnableApi", app_enable_api);
  preferences.putBool("appEnableSerial", app_enable_serial);
  preferences.putBool("appEnableSSL", app_enable_ssl);

  // write our pem to local storage
  File fp = LittleFS.open("/server.crt", "w");
  fp.print(input["server_cert"] | "");
  fp.close();

  // write our key to local storage
  File fp2 = LittleFS.open("/server.key", "w");
  fp2.print(input["server_key"] | "");
  fp2.close();

  // restart the board.
  if (old_app_enable_ssl != app_enable_ssl)
    ESP.restart();
}

void handleLogin(JsonVariantConst input, JsonVariant output, YBMode mode,
                 PsychicWebSocketClient *connection) {
  if (!input["user"].is<String>())
    return generateErrorJSON(output, "'user' is a required parameter");

  if (!input["pass"].is<String>())
    return generateErrorJSON(output, "'pass' is a required parameter");

  // init
  char myuser[YB_USERNAME_LENGTH];
  char mypass[YB_PASSWORD_LENGTH];
  strlcpy(myuser, input["user"] | "", sizeof(myuser));
  strlcpy(mypass, input["pass"] | "", sizeof(mypass));

  // check their credentials
  bool is_authenticated = false;
  UserRole role = app_default_role;

  if (!strcmp(admin_user, myuser) && !strcmp(admin_pass, mypass)) {
    is_authenticated = true;
    role = ADMIN;
    output["role"] = "admin";
  }

  if (!strcmp(guest_user, myuser) && !strcmp(guest_pass, mypass)) {
    is_authenticated = true;
    role = GUEST;
    output["role"] = "guest";
  }

  // okay, are we in?
  if (is_authenticated) {
    // check to see if there's room for us.
    if (mode == YBP_MODE_WEBSOCKET) {
      if (!logClientIn(connection, role))
        return generateErrorJSON(output, "Too many connections.");
    } else if (mode == YBP_MODE_SERIAL) {
      is_serial_authenticated = true;
      serial_role = role;
    }

    output["msg"] = "login";
    output["role"] = getRoleText(role);
    output["message"] = "Login successful.";

    return;
  }

  // gtfo.
  return generateErrorJSON(output, "Wrong username/password.");
}

void handleLogout(JsonVariantConst input, JsonVariant output, YBMode mode,
                  PsychicWebSocketClient *connection) {
  if (!isLoggedIn(input, mode, connection))
    return generateErrorJSON(output, "You are not logged in.");

  // what type of client are you?
  if (mode == YBP_MODE_WEBSOCKET) {
    removeClientFromAuthList(connection);

    // we don't actually want to close the connection, bad UI
    // connection->close();
  } else if (mode == YBP_MODE_SERIAL) {
    is_serial_authenticated = false;
    serial_role = app_default_role;
  }
}

void handleRestart(JsonVariantConst input, JsonVariant output) {
  Serial.println("Restarting board.");

  ESP.restart();
}

void handleFactoryReset(JsonVariantConst input, JsonVariant output) {
  // delete all our prefs
  preferences.clear();
  preferences.end();

  // restart the board.
  ESP.restart();
}

void handleOTAStart(JsonVariantConst input, JsonVariant output) {
  // look for new firmware
  bool updatedNeeded = FOTA.execHTTPcheck();
  if (updatedNeeded)
    doOTAUpdate = true;
  else
    return generateErrorJSON(output, "Firmware already up to date.");
}

void handleSetPWMChannel(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_PWM_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<JsonVariantConst>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidPWMChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // is it enabled?
  if (!pwm_channels[cid].isEnabled)
    return generateErrorJSON(output, "Channel is not enabled.");

  // our duty cycle
  if (input["duty"].is<float>()) {
    // is it enabled?
    if (!pwm_channels[cid].isEnabled)
      return generateErrorJSON(output, "Channel is not enabled.");

    float duty = input["duty"];

    // what do we hate?  va-li-date!
    if (duty < 0)
      return generateErrorJSON(output, "Duty cycle must be >= 0");
    else if (duty > 1)
      return generateErrorJSON(output, "Duty cycle must be <= 1");

    // okay, we're good.
    pwm_channels[cid].setDuty(duty);

    // change our output pin to reflect
    pwm_channels[cid].updateOutput();
  }

  // change state
  if (input["state"].is<String>()) {
    // source is required
    if (!input["source"].is<String>())
      return generateErrorJSON(output, "'source' is a required parameter");

    // check the length
    char error[50];
    if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
      sprintf(error, "Maximum source length is %s characters.",
              YB_HOSTNAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // get our data
    strlcpy(pwm_channels[cid].source, input["source"] | local_hostname,
            sizeof(pwm_channels[cid].source));

    // okay, set our state
    char state[10];
    strlcpy(state, input["state"] | "OFF", sizeof(state));

    // update our pwm channel
    pwm_channels[cid].setState(state);

    // get that update out ASAP... if its our own update
    if (!strcmp(pwm_channels[cid].source, local_hostname))
      sendFastUpdate();
  }
#else
  return generateErrorJSON(output, "Board does not have output channels.");
#endif
}

void handleConfigPWMChannel(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_PWM_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidPWMChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // channel name
  if (input["name"].is<String>()) {
    // is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH - 1) {
      char error[50];
      sprintf(error, "Maximum channel name length is %s characters.",
              YB_CHANNEL_NAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(pwm_channels[cid].name, input["name"] | "Channel ?",
            sizeof(pwm_channels[cid].name));
    sprintf(prefIndex, "pwmName%d", cid);
    preferences.putString(prefIndex, pwm_channels[cid].name);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // channel type
  if (input["type"].is<String>()) {
    // is it too long?
    if (strlen(input["type"]) > YB_TYPE_LENGTH - 1) {
      char error[50];
      sprintf(error, "Maximum channel type length is %s characters.",
              YB_CHANNEL_NAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(pwm_channels[cid].type, input["type"] | "other",
            sizeof(pwm_channels[cid].type));
    sprintf(prefIndex, "pwmType%d", cid);
    preferences.putString(prefIndex, pwm_channels[cid].type);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // default state
  if (input["defaultState"].is<String>()) {
    // is it too long?
    if (strlen(input["defaultState"]) >
        sizeof(pwm_channels[cid].defaultState) - 1) {
      char error[50];
      sprintf(error, "Maximum default state length is %s characters.",
              sizeof(pwm_channels[cid].defaultState) - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(pwm_channels[cid].defaultState, input["defaultState"] | "OFF",
            sizeof(pwm_channels[cid].defaultState));
    sprintf(prefIndex, "pwmDefault%d", cid);
    preferences.putString(prefIndex, pwm_channels[cid].defaultState);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // dimmability
  if (input["isDimmable"].is<bool>()) {
    bool isDimmable = input["isDimmable"];
    pwm_channels[cid].isDimmable = isDimmable;

    // save to our storage
    sprintf(prefIndex, "pwmDimmable%d", cid);
    preferences.putBool(prefIndex, isDimmable);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // enabled
  if (input["enabled"].is<bool>()) {
    // save right nwo.
    bool enabled = input["enabled"];
    pwm_channels[cid].isEnabled = enabled;

    // save to our storage
    sprintf(prefIndex, "pwmEnabled%d", cid);
    preferences.putBool(prefIndex, enabled);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // soft fuse
  if (input["softFuse"].is<float>()) {
    // i crave validation!
    float softFuse = input["softFuse"];
    softFuse = constrain(softFuse, 0.01, 20.0);

    // save right nwo.
    pwm_channels[cid].softFuseAmperage = softFuse;

    // save to our storage
    sprintf(prefIndex, "pwmSoftFuse%d", cid);
    preferences.putFloat(prefIndex, softFuse);

    // give them the updated config
    return generateConfigJSON(output);
  }
#else
  return generateErrorJSON(output, "Board does not have output channels.");
#endif
}

void handleTogglePWMChannel(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_PWM_CHANNELS
  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidPWMChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // source is required
  if (!input["source"].is<String>())
    return generateErrorJSON(output, "'source' is a required parameter");

  // check the length
  char error[50];
  if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
    sprintf(error, "Maximum source length is %s characters.",
            YB_HOSTNAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // save our source
  strlcpy(pwm_channels[cid].source, input["source"] | local_hostname,
          sizeof(pwm_channels[cid].source));

  // update our state
  if (pwm_channels[cid].tripped)
    pwm_channels[cid].setState("OFF");
  else if (!pwm_channels[cid].state)
    pwm_channels[cid].setState("ON");
  else
    pwm_channels[cid].setState("OFF");
#else
  return generateErrorJSON(output, "Board does not have output channels.");
#endif
}

void handleFadePWMChannel(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_PWM_CHANNELS
  unsigned long start = micros();
  unsigned long t1, t2, t3, t4 = 0;

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");
  if (!input["duty"].is<float>())
    return generateErrorJSON(output, "'duty' is a required parameter");
  if (!input["millis"].is<int>())
    return generateErrorJSON(output, "'millis' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidPWMChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  float duty = input["duty"];

  // what do we hate?  va-li-date!
  if (duty < 0)
    return generateErrorJSON(output, "Duty cycle must be >= 0");
  else if (duty > 1)
    return generateErrorJSON(output, "Duty cycle must be <= 1");

  t1 = micros();
  t2 = micros();

  int fadeDelay = input["millis"] | 0;

  pwm_channels[cid].setFade(duty, fadeDelay);

  t3 = micros();

  unsigned long finish = micros();

  if (finish - start > 10000) {
    Serial.println("led fade");
    Serial.printf("params: %dus\n", t1 - start);
    Serial.printf("channelSetDuty: %dus\n", t2 - t1);
    Serial.printf("channelFade: %dus\n", t3 - t2);
    Serial.printf("total: %dus\n", finish - start);
    Serial.println();
  }
#else
  return generateErrorJSON(output, "Board does not have output channels.");
#endif
}

void handleSetSwitch(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_INPUT_CHANNELS
  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidInputChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // state is required
  if (!input["state"].is<String>())
    return generateErrorJSON(output, "'state' is a required parameter");

  // source is required
  if (!input["source"].is<String>())
    return generateErrorJSON(output, "'source' is a required parameter");

  // check the length
  char error[50];
  if (strlen(input["source"]) > YB_HOSTNAME_LENGTH - 1) {
    sprintf(error, "Maximum source length is %s characters.",
            YB_HOSTNAME_LENGTH - 1);
    return generateErrorJSON(output, error);
  }

  // get our data
  strlcpy(input_channels[cid].source, input["source"] | local_hostname,
          sizeof(local_hostname));

  // okay, set our state
  char state[10];
  strlcpy(state, input["state"] | "OFF", sizeof(state));

  // update our pwm channel
  input_channels[cid].setState(state);
#else
  return generateErrorJSON(output, "Board does not have RGB channels.");
#endif
}

void handleConfigSwitch(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_INPUT_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidInputChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // channel name
  if (input["name"].is<String>()) {
    // is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH - 1) {
      char error[50];
      sprintf(error, "Maximum channel name length is %s characters.",
              YB_CHANNEL_NAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // save the name
    strlcpy(input_channels[cid].name, input["name"] | "Input ?",
            sizeof(input_channels[cid].name));
    sprintf(prefIndex, "iptName%d", cid);
    preferences.putString(prefIndex, input_channels[cid].name);
  }

  // switch mode
  if (input["mode"].is<String>()) {
    String tempMode = input["mode"] | "DIRECT";
    input_channels[cid].mode = InputChannel::getMode(tempMode);
    sprintf(prefIndex, "iptMode%d", cid);
    preferences.putUChar(prefIndex, input_channels[cid].mode);
  }

  // enabled
  if (input["enabled"].is<bool>()) {
    // save right nwo.
    bool enabled = input["enabled"];
    input_channels[cid].isEnabled = enabled;

    // save to our storage
    sprintf(prefIndex, "iptEnabled%d", cid);
    preferences.putBool(prefIndex, enabled);
  }

  // default state
  if (input["defaultState"].is<String>()) {
    // is it too long?
    if (strlen(input["defaultState"]) >
        sizeof(input_channels[cid].defaultState) - 1) {
      char error[50];
      sprintf(error, "Maximum default state length is %s characters.",
              sizeof(input_channels[cid].defaultState) - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(input_channels[cid].defaultState, input["defaultState"] | "OFF",
            sizeof(input_channels[cid].defaultState));
    sprintf(prefIndex, "iptDefault%d", cid);
    preferences.putString(prefIndex, input_channels[cid].defaultState);
  }

  // give them the updated config
  return generateConfigJSON(output);
#else
  return generateErrorJSON(output, "Board does not have input channels.");
#endif
}

void handleConfigRGB(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_RGB_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidRGBChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // channel name
  if (input["name"].is<String>()) {
    // is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH - 1) {
      char error[50];
      sprintf(error, "Maximum channel name length is %s characters.",
              YB_CHANNEL_NAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(rgb_channels[cid].name, input["name"] | "RGB ?",
            sizeof(rgb_channels[cid].name));
    sprintf(prefIndex, "rgbName%d", cid);
    preferences.putString(prefIndex, rgb_channels[cid].name);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // enabled
  if (input["enabled"].is<bool>()) {
    // save right nwo.
    bool enabled = input["enabled"];
    rgb_channels[cid].isEnabled = enabled;

    // save to our storage
    sprintf(prefIndex, "rgbEnabled%d", cid);
    preferences.putBool(prefIndex, enabled);

    // give them the updated config
    return generateConfigJSON(output);
  }
#else
  return generateErrorJSON(output, "Board does not have RGB channels.");
#endif
}

void handleSetRGB(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_RGB_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidRGBChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // new color?
  if (input["red"].is<float>() || input["green"].is<float>() ||
      input["blue"].is<float>()) {
    float red = rgb_channels[cid].red;
    float green = rgb_channels[cid].green;
    float blue = rgb_channels[cid].blue;

    // what do we hate?  va-li-date!
    if (input["red"].is<float>()) {
      red = input["red"];
      if (red < 0)
        return generateErrorJSON(output, "Red must be >= 0");
      else if (red > 1)
        return generateErrorJSON(output, "Red must be <= 1");
    }

    // what do we hate?  va-li-date!
    if (input["green"].is<float>()) {
      green = input["green"];
      if (green < 0)
        return generateErrorJSON(output, "Green must be >= 0");
      else if (green > 1)
        return generateErrorJSON(output, "Green must be <= 1");
    }

    // what do we hate?  va-li-date!
    if (input["blue"].is<float>()) {
      blue = input["blue"];
      if (blue < 0)
        return generateErrorJSON(output, "Blue must be >= 0");
      else if (blue > 1)
        return generateErrorJSON(output, "Blue must be <= 1");
    }

    rgb_channels[cid].setRGB(red, green, blue);
  }
#else
  return generateErrorJSON(output, "Board does not have RGB channels.");
#endif
}

void handleConfigADC(JsonVariantConst input, JsonVariant output) {
#ifdef YB_HAS_ADC_CHANNELS
  char prefIndex[YB_PREF_KEY_LENGTH];

  // id is required
  if (!input["id"].is<int>())
    return generateErrorJSON(output, "'id' is a required parameter");

  // is it a valid channel?
  byte cid = input["id"];
  if (!isValidADCChannel(cid))
    return generateErrorJSON(output, "Invalid channel id");

  // channel name
  if (input["name"].is<String>()) {
    // is it too long?
    if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH - 1) {
      char error[50];
      sprintf(error, "Maximum channel name length is %s characters.",
              YB_CHANNEL_NAME_LENGTH - 1);
      return generateErrorJSON(output, error);
    }

    // save to our storage
    strlcpy(adc_channels[cid].name, input["name"] | "ADC ?",
            sizeof(adc_channels[cid].name));
    sprintf(prefIndex, "adcName%d", cid);
    preferences.putString(prefIndex, adc_channels[cid].name);

    // give them the updated config
    return generateConfigJSON(output);
  }

  // enabled
  if (input["enabled"].is<bool>()) {
    // save right nwo.
    bool enabled = input["enabled"];
    adc_channels[cid].isEnabled = enabled;

    // save to our storage
    sprintf(prefIndex, "adcEnabled%d", cid);
    preferences.putBool(prefIndex, enabled);

    // give them the updated config
    return generateConfigJSON(output);
  }
#else
  return generateErrorJSON(output, "Board does not have ADC channels.");
#endif
}

void handleSetTheme(JsonVariantConst input, JsonVariant output) {
  if (!input["theme"].is<String>())
    return generateErrorJSON(output, "'theme' is a required parameter");

  String temp = input["theme"];

  if (temp != "light" && temp != "dark")
    return generateErrorJSON(output,
                             "'theme' must either be 'light' or 'dark'");

  app_theme = temp;

  sendThemeUpdate();
}

void handleSetBrightness(JsonVariantConst input, JsonVariant output) {
  if (input["brightness"].is<float>()) {
    float brightness = input["brightness"];

    // what do we hate?  va-li-date!
    if (brightness < 0)
      return generateErrorJSON(output, "Brightness must be >= 0");
    else if (brightness > 1)
      return generateErrorJSON(output, "Brightness must be <= 1");

    globalBrightness = brightness;

// TODO: need to put this on a time delay
// preferences.putFloat("brightness", globalBrightness);

// loop through all our light stuff and update outputs
#ifdef YB_HAS_PWM_CHANNELS
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++)
      pwm_channels[i].updateOutput();
#endif

#ifdef YB_HAS_RGB_CHANNELS
    for (byte i = 0; i < YB_RGB_CHANNEL_COUNT; i++)
      rgb_channels[i].updateOutput();
#endif

    sendBrightnessUpdate();
  } else
    return generateErrorJSON(output, "'brightness' is a required parameter.");
}

void generateConfigJSON(JsonVariant output) {
  // our identifying info
  output["msg"] = "config";
  output["firmware_version"] = YB_FIRMWARE_VERSION;
  output["hardware_version"] = YB_HARDWARE_VERSION;
  output["esp_idf_version"] = esp_get_idf_version();
  output["arduino_version"] = arduino_version;
  output["name"] = board_name;
  output["hostname"] = local_hostname;
  output["use_ssl"] = app_enable_ssl;
  output["uuid"] = uuid;
  output["default_role"] = getRoleText(app_default_role);
  output["brightness"] = globalBrightness;

  // some debug info
  output["last_restart_reason"] = getResetReason();
  if (has_coredump)
    output["has_coredump"] = has_coredump;

#ifdef YB_HAS_BUS_VOLTAGE
  output["bus_voltage"] = true;
#endif

  // do we want to flag it for config?
  if (is_first_boot)
    output["first_boot"] = true;

// output / pwm channels
#ifdef YB_HAS_PWM_CHANNELS
  for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    output["pwm"][i]["id"] = i;
    output["pwm"][i]["name"] = pwm_channels[i].name;
    output["pwm"][i]["type"] = pwm_channels[i].type;
    output["pwm"][i]["enabled"] = pwm_channels[i].isEnabled;
    output["pwm"][i]["hasCurrent"] = true;
    output["pwm"][i]["softFuse"] = round2(pwm_channels[i].softFuseAmperage);
    output["pwm"][i]["isDimmable"] = pwm_channels[i].isDimmable;
    output["pwm"][i]["defaultState"] = pwm_channels[i].defaultState;
  }
#endif

// input / digital IO channels
#ifdef YB_HAS_INPUT_CHANNELS
  for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
    output["switches"][i]["id"] = i;
    output["switches"][i]["name"] = input_channels[i].name;
    output["switches"][i]["enabled"] = input_channels[i].isEnabled;
    output["switches"][i]["mode"] =
        InputChannel::getModeName(input_channels[i].mode);
    output["switches"][i]["defaultState"] = input_channels[i].defaultState;
  }
#endif

// input / analog ADC channesl
#ifdef YB_HAS_ADC_CHANNELS
  for (byte i = 0; i < YB_ADC_CHANNEL_COUNT; i++) {
    output["adc"][i]["id"] = i;
    output["adc"][i]["name"] = adc_channels[i].name;
    output["adc"][i]["enabled"] = adc_channels[i].isEnabled;
  }
#endif

// input / analog ADC channesl
#ifdef YB_HAS_RGB_CHANNELS
  for (byte i = 0; i < YB_RGB_CHANNEL_COUNT; i++) {
    output["rgb"][i]["id"] = i;
    output["rgb"][i]["name"] = rgb_channels[i].name;
    output["rgb"][i]["enabled"] = rgb_channels[i].isEnabled;
  }
#endif
}

void generateUpdateJSON(JsonVariant output) {
  output["msg"] = "update";
  output["uptime"] = esp_timer_get_time();

#ifdef YB_HAS_BUS_VOLTAGE
  output["bus_voltage"] = busVoltage;
#endif

#ifdef YB_HAS_PWM_CHANNELS
  for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    output["pwm"][i]["id"] = i;
    output["pwm"][i]["state"] = pwm_channels[i].getState();
    output["pwm"][i]["source"] = pwm_channels[i].source;
    // output["pwm"][i]["tripped"] = pwm_channels[i].tripped;
    if (pwm_channels[i].isDimmable)
      output["pwm"][i]["duty"] = round2(pwm_channels[i].dutyCycle);
    output["pwm"][i]["current"] = round2(pwm_channels[i].amperage);
    output["pwm"][i]["aH"] = round3(pwm_channels[i].ampHours);
    output["pwm"][i]["wH"] = round3(pwm_channels[i].wattHours);
  }
#endif

#ifdef YB_HAS_INPUT_CHANNELS
  for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
    output["switches"][i]["id"] = i;
    output["switches"][i]["raw"] = input_channels[i].raw;
    output["switches"][i]["state"] = input_channels[i].getState();
    output["switches"][i]["source"] = input_channels[i].source;
  }
#endif

// input / analog ADC channesl
#ifdef YB_HAS_ADC_CHANNELS
  for (byte i = 0; i < YB_ADC_CHANNEL_COUNT; i++) {
    output["adc"][i]["id"] = i;
    output["adc"][i]["voltage"] = adc_channels[i].getVoltage();
    output["adc"][i]["reading"] = adc_channels[i].getReading();
    output["adc"][i]["percentage"] = 100.0 *
                                     (float)adc_channels[i].getReading() /
                                     (pow(2, YB_ADC_RESOLUTION) - 1);
    adc_channels[i].resetAverage();
  }
#endif

// input / analog ADC channesl
#ifdef YB_HAS_RGB_CHANNELS
  for (byte i = 0; i < YB_RGB_CHANNEL_COUNT; i++) {
    output["rgb"][i]["id"] = i;
    output["rgb"][i]["red"] = rgb_channels[i].red;
    output["rgb"][i]["green"] = rgb_channels[i].green;
    output["rgb"][i]["blue"] = rgb_channels[i].blue;
  }
#endif
}

void generateFastUpdateJSON(JsonVariant output) {
  output["msg"] = "update";
  output["fast"] = 1;
  output["uptime"] = esp_timer_get_time();

#ifdef YB_HAS_BUS_VOLTAGE
  output["bus_voltage"] = busVoltage;
#endif

  byte j;

#ifdef YB_HAS_PWM_CHANNELS
  j = 0;
  for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    if (pwm_channels[i].sendFastUpdate) {
      output["pwm"][j]["id"] = i;
      output["pwm"][j]["state"] = pwm_channels[i].getState();
      output["pwm"][j]["source"] = pwm_channels[i].source;
      if (pwm_channels[i].isDimmable)
        output["pwm"][j]["duty"] = round2(pwm_channels[i].dutyCycle);
      output["pwm"][j]["current"] = round2(pwm_channels[i].amperage);
      output["pwm"][j]["aH"] = round3(pwm_channels[i].ampHours);
      output["pwm"][j]["wH"] = round3(pwm_channels[i].wattHours);
      // output["pwm"][j]["tripped"] = pwm_channels[i].tripped;
      j++;

      // dont need it anymore
      pwm_channels[i].sendFastUpdate = false;
    }
  }
#endif

#ifdef YB_HAS_INPUT_CHANNELS
  j = 0;
  for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
    if (input_channels[i].sendFastUpdate) {
      output["switches"][j]["id"] = i;
      output["switches"][j]["state"] = input_channels[i].getState();
      output["switches"][j]["source"] = input_channels[i].source;
      input_channels[i].sendFastUpdate = false;
      j++;
    }
  }
#endif
}

void generateStatsJSON(JsonVariant output) {
  // some basic statistics and info
  output["msg"] = "stats";
  output["uuid"] = uuid;
  output["received_message_total"] = totalReceivedMessages;
  output["received_message_mps"] = receivedMessagesPerSecond;
  output["sent_message_total"] = totalSentMessages;
  output["sent_message_mps"] = sentMessagesPerSecond;
  output["websocket_client_count"] = websocketClientCount;
  output["http_client_count"] = httpClientCount - websocketClientCount;
  output["fps"] = (int)framerate;
  output["uptime"] = esp_timer_get_time();
  output["heap_size"] = ESP.getHeapSize();
  output["free_heap"] = ESP.getFreeHeap();
  output["min_free_heap"] = ESP.getMinFreeHeap();
  output["max_alloc_heap"] = ESP.getMaxAllocHeap();
  output["rssi"] = WiFi.RSSI();

  // what is our IP address?
  if (!strcmp(wifi_mode, "ap"))
    output["ip_address"] = apIP;
  else
    output["ip_address"] = WiFi.localIP();

#ifdef YB_HAS_BUS_VOLTAGE
  output["bus_voltage"] = busVoltage;
#endif

#ifdef YB_HAS_INPUT_CHANNELS
  for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
    output["switches"][i]["id"] = i;
    output["switches"][i]["state_change_count"] =
        input_channels[i].stateChangeCount;
  }
#endif

#ifdef YB_HAS_PWM_CHANNELS
  // info about each of our channels
  for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
    output["pwm"][i]["id"] = i;
    output["pwm"][i]["name"] = pwm_channels[i].name;
    output["pwm"][i]["aH"] = pwm_channels[i].ampHours;
    output["pwm"][i]["wH"] = pwm_channels[i].wattHours;
    output["pwm"][i]["state_change_count"] = pwm_channels[i].stateChangeCount;
    output["pwm"][i]["soft_fuse_trip_count"] =
        pwm_channels[i].softFuseTripCount;
  }
#endif

#ifdef YB_HAS_FANS
  // info about each of our fans
  for (byte i = 0; i < YB_FAN_COUNT; i++)
    output["fans"][i]["rpm"] = fans_last_rpm[i];
#endif
}

void generateNetworkConfigJSON(JsonVariant output) {
  // our identifying info
  output["msg"] = "network_config";
  output["wifi_mode"] = wifi_mode;
  output["wifi_ssid"] = wifi_ssid;
  output["wifi_pass"] = wifi_pass;
  output["local_hostname"] = local_hostname;
}

void generateAppConfigJSON(JsonVariant output) {
  // our identifying info
  output["msg"] = "app_config";
  output["default_role"] = getRoleText(app_default_role);
  output["admin_user"] = admin_user;
  output["admin_pass"] = admin_pass;
  output["guest_user"] = guest_user;
  output["guest_pass"] = guest_pass;
  output["app_update_interval"] = app_update_interval;
  output["app_enable_mfd"] = app_enable_mfd;
  output["app_enable_api"] = app_enable_api;
  output["app_enable_serial"] = app_enable_serial;
  output["app_enable_ssl"] = app_enable_ssl;
  output["server_cert"] = server_cert;
  output["server_key"] = server_key;
}

void generateOTAProgressUpdateJSON(JsonVariant output, float progress) {
  output["msg"] = "ota_progress";
  output["progress"] = round2(progress);
}

void generateOTAProgressFinishedJSON(JsonVariant output) {
  output["msg"] = "ota_finished";
}

void generateErrorJSON(JsonVariant output, const char *error) {
  output["msg"] = "status";
  output["status"] = "error";
  output["message"] = error;
}

void generateSuccessJSON(JsonVariant output, const char *success) {
  output["msg"] = "status";
  output["status"] = "success";
  output["message"] = success;
}

void generateLoginRequiredJSON(JsonVariant output) {
  generateErrorJSON(output, "You must be logged in.");
}

void generatePongJSON(JsonVariant output) { output["pong"] = millis(); }

void sendThemeUpdate() {
  JsonDocument output;
  output["msg"] = "set_theme";
  output["theme"] = app_theme;

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(output);
  char *jsonBuffer = (char *)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // did we get anything?
  if (jsonBuffer != NULL) {
    serializeJson(output, jsonBuffer, jsonSize + 1);
    sendToAll(jsonBuffer, NOBODY);
  }
  free(jsonBuffer);
}

void sendBrightnessUpdate() {
  JsonDocument output;
  output["msg"] = "set_brightness";
  output["brightness"] = globalBrightness;

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(output);
  char *jsonBuffer = (char *)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // did we get anything?
  if (jsonBuffer != NULL) {
    serializeJson(output, jsonBuffer, jsonSize + 1);
    sendToAll(jsonBuffer, NOBODY);
  }
  free(jsonBuffer);
}

void sendFastUpdate() {
  JsonDocument output;
  generateFastUpdateJSON(output);

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(output);
  char *jsonBuffer = (char *)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // did we get anything?
  if (jsonBuffer != NULL) {
    serializeJson(output, jsonBuffer, jsonSize + 1);
    sendToAll(jsonBuffer, GUEST);
  }
  free(jsonBuffer);
}

void sendOTAProgressUpdate(float progress) {
  JsonDocument output;
  generateOTAProgressUpdateJSON(output, progress);

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(output);
  char *jsonBuffer = (char *)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // did we get anything?
  if (jsonBuffer != NULL) {
    serializeJson(output, jsonBuffer, jsonSize + 1);
    sendToAll(jsonBuffer, GUEST);
  }
  free(jsonBuffer);
}

void sendOTAProgressFinished() {
  JsonDocument output;
  generateOTAProgressFinishedJSON(output);

  // dynamically allocate our buffer
  size_t jsonSize = measureJson(output);
  char *jsonBuffer = (char *)malloc(jsonSize + 1);
  jsonBuffer[jsonSize] = '\0'; // null terminate

  // did we get anything?
  if (jsonBuffer != NULL) {
    serializeJson(output, jsonBuffer, jsonSize + 1);
    sendToAll(jsonBuffer, GUEST);
  }
  free(jsonBuffer);
}

void sendToAll(const char *jsonString, UserRole auth_level) {
  sendToAllWebsockets(jsonString, auth_level);

  if (app_enable_serial && serial_role >= auth_level)
    Serial.println(jsonString);
}
