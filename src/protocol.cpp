/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "protocol.h"

char board_name[YB_BOARD_NAME_LENGTH] = "Yarrboard";
char app_user[YB_USERNAME_LENGTH] = "admin";
char app_pass[YB_PASSWORD_LENGTH] = "admin";
bool require_login = true;
bool app_enable_api = true;
bool app_enable_serial = false;
bool app_enable_ssl = false;
bool is_serial_authenticated = false;

//for tracking our message loop
unsigned long previousMessageMillis = 0;
unsigned int lastHandledMessages = 0;
unsigned int handledMessages = 0;
unsigned long totalHandledMessages = 0;

void protocol_setup()
{
  //look up our board name
  if (preferences.isKey("boardName"))
    strlcpy(board_name, preferences.getString("boardName").c_str(), sizeof(board_name));

  //look up our username/password
  if (preferences.isKey("app_user"))
    strlcpy(app_user, preferences.getString("app_user").c_str(), sizeof(app_user));
  if (preferences.isKey("app_pass"))
    strlcpy(app_pass, preferences.getString("app_pass").c_str(), sizeof(app_pass));
  if (preferences.isKey("require_login"))
    require_login = preferences.getBool("require_login");
  if (preferences.isKey("appEnableApi"))
    app_enable_api = preferences.getBool("appEnableApi");
  if (preferences.isKey("appEnableSerial"))
    app_enable_serial = preferences.getBool("appEnableSerial");

  //send serial a config off the bat    
  if (app_enable_serial)
  {
    DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

    generateConfigJSON(output);
    serializeJson(output, Serial);
  }
}

void protocol_loop()
{
  //lookup our info periodically
  unsigned int messageDelta = millis() - previousMessageMillis;
  if (messageDelta >= YB_UPDATE_FREQUENCY)
  {
    #ifdef YB_HAS_PWM_CHANNELS
      //update our averages, etc.
      for (byte i=0; i<YB_PWM_CHANNEL_COUNT; i++)
        pwm_channels[i].calculateAverages(messageDelta);
    #endif
    
    #ifdef YB_HAS_BUS_VOLTAGE
      busVoltage = getBusVoltage();
    #endif

    //read and send out our json update
    sendUpdate();
  
    //how fast are we?
    //Serial.printf("Framerate: %f\n", framerate);
    //Serial.print(messageDelta);
    //Serial.print("ms | msg/s: ");
    //Serial.print(handledMessages - lastHandledMessages);
    //Serial.println();

    //for keeping track.
    lastHandledMessages = handledMessages;
    previousMessageMillis = millis();
  }

  //any serial port customers?
  if (app_enable_serial)
  {
    if (Serial.available() > 0)
      handleSerialJson();
  }
}

void handleSerialJson()
{
  StaticJsonDocument<1024> input;
  DeserializationError err = deserializeJson(input, Serial);

  //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  //ignore newlines with serial.
  if (err)
  {
    if (strcmp(err.c_str(), "EmptyInput"))
    {
      char error[64];
      sprintf(error, "deserializeJson() failed with code %s", err.c_str());
      generateErrorJSON(output, error);
      serializeJson(output, Serial);
    }
  }
  else
  {
    handleReceivedJSON(input, output, YBP_MODE_SERIAL);

    //we can have empty responses
    if (output.size())
      serializeJson(output, Serial);    
  }
}

void handleReceivedJSON(JsonVariantConst input, JsonVariant output, byte mode, MongooseHttpWebSocketConnection *connection)
{
  //make sure its correct
  if (!input.containsKey("cmd"))
    return generateErrorJSON(output, "'cmd' is a required parameter.");

  //what is your command?
  const char* cmd = input["cmd"];

  //let the client keep track of messages
  if (input.containsKey("msgid"))
  {
    unsigned int msgid = input["msgid"];
    output["status"] = "ok";
    output["msgid"] = msgid;
  }

  //keep track!
  handledMessages++;
  totalHandledMessages++;

  //only pages with no login requirements
  if (!strcmp(cmd, "login") || !strcmp(cmd, "ping"))
  {
    if (!strcmp(cmd, "login"))
      return handleLogin(input, output, mode, connection);
    else if (!strcmp(cmd, "ping"))
      return generatePongJSON(output);
  }
  else
  {
    //need to be logged in here.
    if (!isLoggedIn(input, mode, connection))
        return generateLoginRequiredJSON(output);

    //what is your command?
    if (!strcmp(cmd, "set_boardname"))
      return handleSetBoardName(input, output);   
    else if (!strcmp(cmd, "get_config"))
      return generateConfigJSON(output);
    else if (!strcmp(cmd, "get_network_config"))
      return generateNetworkConfigJSON(output);
    else if (!strcmp(cmd, "set_network_config"))
      return handleSetNetworkConfig(input, output);
    else if (!strcmp(cmd, "get_app_config"))
      return generateAppConfigJSON(output);
    else if (!strcmp(cmd, "set_app_config"))
      return handleSetAppConfig(input, output);
    else if (!strcmp(cmd, "get_stats"))
      return generateStatsJSON(output);
    else if (!strcmp(cmd, "get_update"))
      return generateUpdateJSON(output);
    else if (!strcmp(cmd, "restart"))
      return handleRestart(input, output);
    else if (!strcmp(cmd, "factory_reset"))
      return handleFactoryReset(input, output);
    else if (!strcmp(cmd, "ota_start"))
      return handleOTAStart(input, output);
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
    else if (!strcmp(cmd, "set_adc"))
      return handleSetADC(input, output);
    else
      return generateErrorJSON(output, "Invalid command.");
  }

  //unknown command.
  return generateErrorJSON(output, "Malformed json.");
}

void handleSetBoardName(JsonVariantConst input, JsonVariant output)
{
    if (!input.containsKey("value"))
      return generateErrorJSON(output, "'value' is a required parameter");

    //is it too long?
    if (strlen(input["value"]) > YB_BOARD_NAME_LENGTH-1)
    {
      char error[50];
      sprintf(error, "Maximum board name length is %s characters.", YB_BOARD_NAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //update variable
    strlcpy(board_name, input["value"] | "Yarrboard", sizeof(board_name));

    //save to our storage
    preferences.putString("boardName", board_name);

    //give them the updated config
    return generateConfigJSON(output);
}

void handleSetNetworkConfig(JsonVariantConst input, JsonVariant output)
{
    //clear our first boot flag since they submitted the network page.
    is_first_boot = false;

    char error[50];

    //error checking
    if (!input.containsKey("wifi_mode"))
      return generateErrorJSON(output, "'wifi_mode' is a required parameter");
    if (!input.containsKey("wifi_ssid"))
      return generateErrorJSON(output, "'wifi_ssid' is a required parameter");
    if (!input.containsKey("wifi_pass"))
      return generateErrorJSON(output, "'wifi_pass' is a required parameter");
    if (!input.containsKey("local_hostname"))
      return generateErrorJSON(output, "'local_hostname' is a required parameter");

    //is it too long?
    if (strlen(input["wifi_ssid"]) > YB_WIFI_SSID_LENGTH-1)
    {
      sprintf(error, "Maximum wifi ssid length is %s characters.", YB_WIFI_SSID_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    if (strlen(input["wifi_pass"]) > YB_WIFI_PASSWORD_LENGTH-1)
    {
      sprintf(error, "Maximum wifi password length is %s characters.", YB_WIFI_PASSWORD_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    if (strlen(input["local_hostname"]) > YB_HOSTNAME_LENGTH-1)
    {
      sprintf(error, "Maximum hostname length is %s characters.", YB_HOSTNAME_LENGTH-1);
      return generateErrorJSON(output, error);
    }

    //get our data
    char new_wifi_mode[16];
    char new_wifi_ssid[YB_WIFI_SSID_LENGTH];
    char new_wifi_pass[YB_WIFI_PASSWORD_LENGTH];
    
    strlcpy(new_wifi_mode, input["wifi_mode"] | "ap", sizeof(new_wifi_mode));
    strlcpy(new_wifi_ssid, input["wifi_ssid"] | "SSID", sizeof(new_wifi_ssid));
    strlcpy(new_wifi_pass, input["wifi_pass"] | "PASS", sizeof(new_wifi_pass));
    strlcpy(local_hostname, input["local_hostname"] | "yarrboard", sizeof(local_hostname));

    //no special cases here.
    preferences.putString("local_hostname", local_hostname);

    //make sure we can connect before we save
    if (!strcmp(new_wifi_mode, "client"))
    {
      //did we change username/password?
      if (strcmp(new_wifi_ssid, wifi_ssid) || strcmp(new_wifi_pass, wifi_pass))
      {
        //try connecting.
        if (connectToWifi(new_wifi_ssid, new_wifi_pass))
        {
          //changing modes?
          if (!strcmp(wifi_mode, "ap"))
            WiFi.softAPdisconnect();

          //save to flash
          preferences.putString("wifi_mode", new_wifi_mode);
          preferences.putString("wifi_ssid", new_wifi_ssid);
          preferences.putString("wifi_pass", new_wifi_pass);

          //save for local use
          strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
          strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
          strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));

          //let the client know.
          return generateSuccessJSON(output, "Connected to new WiFi.");
        }
        //nope, setup our wifi back to default.
        else
          return generateErrorJSON(output, "Can't connect to new WiFi.");
      }
      else
        return generateSuccessJSON(output, "Network settings updated.");
    }
    //okay, AP mode is easier
    else
    {
      //changing modes?
      //if (wifi_mode.equals("client"))
      //  WiFi.disconnect();

      //save to flash
      preferences.putString("wifi_mode", new_wifi_mode);
      preferences.putString("wifi_ssid", new_wifi_ssid);
      preferences.putString("wifi_pass", new_wifi_pass);

      //save for local use.
      strlcpy(wifi_mode, new_wifi_mode, sizeof(wifi_mode));
      strlcpy(wifi_ssid, new_wifi_ssid, sizeof(wifi_ssid));
      strlcpy(wifi_pass, new_wifi_pass, sizeof(wifi_pass));

      //switch us into AP mode
      setupWifi();

      return generateSuccessJSON(output, "AP mode successful, please connect to new network.");
    }
}

void handleSetAppConfig(JsonVariantConst input, JsonVariant output)
{
  if (!input.containsKey("app_user"))
    return generateErrorJSON(output, "'app_user' is a required parameter");
  if (!input.containsKey("app_pass"))
    return generateErrorJSON(output, "'app_pass' is a required parameter");

  //username length checker
  if (strlen(input["app_user"]) > YB_USERNAME_LENGTH-1)
  {
    char error[50];
    sprintf(error, "Maximum username length is %s characters.", YB_USERNAME_LENGTH-1);
    return generateErrorJSON(output, error);
  }

  //password length checker
  if (strlen(input["app_pass"]) > YB_PASSWORD_LENGTH-1)
  {
    char error[50];
    sprintf(error, "Maximum password length is %s characters.", YB_PASSWORD_LENGTH-1);
    return generateErrorJSON(output, error);
  }

  //get our data
  strlcpy(app_user, input["app_user"] | "admin", sizeof(app_user));
  strlcpy(app_pass, input["app_pass"] | "admin", sizeof(app_pass));
  require_login = input["require_login"];
  app_enable_api = input["app_enable_api"];
  app_enable_serial = input["app_enable_serial"];
  app_enable_ssl = input["app_enable_ssl"];

  //no special cases here.
  preferences.putString("app_user", app_user);
  preferences.putString("app_pass", app_pass);
  preferences.putBool("require_login", require_login);  
  preferences.putBool("appEnableApi", app_enable_api);
  preferences.putBool("appEnableSerial", app_enable_serial);
  preferences.putBool("appEnableSSL", app_enable_ssl);

  //write our pem to local storage
  File fp = LittleFS.open("/server.crt", "w");
  fp.print(input["server_cert"] | "");
  fp.close();

  // Serial.println("ssl cert:");
  // Serial.println(input["server_cert"] | "");

  //write our key to local storage
  File fp2 = LittleFS.open("/server.key", "w");
  fp2.print(input["server_key"] | "");
  fp2.close();

  // Serial.println("ssl key:");
  // Serial.println(input["server_key"] | "");

  //restart the board.
  ESP.restart();
}

void handleLogin(JsonVariantConst input, JsonVariant output, byte mode, MongooseHttpWebSocketConnection *connection)
{
    if (!require_login)
      return generateErrorJSON(output, "Login not required.");

  if (!input.containsKey("user"))
    return generateErrorJSON(output, "'user' is a required parameter");

  if (!input.containsKey("pass"))
    return generateErrorJSON(output, "'pass' is a required parameter");

    //init
    char myuser[YB_USERNAME_LENGTH];
    char mypass[YB_PASSWORD_LENGTH];
    strlcpy(myuser, input["user"] | "", sizeof(myuser));
    strlcpy(mypass, input["pass"] | "", sizeof(mypass));

    //morpheus... i'm in.
    if (!strcmp(app_user, myuser) && !strcmp(app_pass, mypass))
    {
        //check to see if there's room for us.
        if (mode == YBP_MODE_WEBSOCKET)
        {
          if (!logClientIn(connection))
            return generateErrorJSON(output, "Too many connections.");
        }
        else if (mode == YBP_MODE_SERIAL)
          is_serial_authenticated = true;

        return generateSuccessJSON(output, "Login successful.");
    }

    //gtfo.
    return generateErrorJSON(output, "Wrong username/password.");
}

void handleRestart(JsonVariantConst input, JsonVariant output)
{
  Serial.println("Restarting board.");

  ESP.restart();
}

void handleFactoryReset(JsonVariantConst input, JsonVariant output)
{
  //delete all our prefs
  preferences.clear();
  preferences.end();

  //restart the board.
  ESP.restart();
}

void handleOTAStart(JsonVariantConst input, JsonVariant output)
{
  //look for new firmware
  bool updatedNeeded = FOTA.execHTTPcheck();
  if (updatedNeeded)
    doOTAUpdate = true;
  else
    return generateErrorJSON(output, "Firmware already up to date.");
}

void handleSetPWMChannel(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_PWM_CHANNELS
    char prefIndex[YB_PREF_KEY_LENGTH];

    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidPWMChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    //change state
    if (input.containsKey("state"))
    {
      //is it enabled?
      if (!pwm_channels[cid].isEnabled)
        return generateErrorJSON(output, "Channel is not enabled.");

      //what is our new state?
      bool state = input["state"];

      //this can crash after long fading sessions, reset it with a manual toggle
      //isChannelFading[this->id] = false;

      //keep track of how many toggles
      if (state && pwm_channels[cid].state != state)
        pwm_channels[cid].stateChangeCount++;

      //record our new state
      pwm_channels[cid].state = state;

      //reset soft fuse when we turn on
      if (state)
        pwm_channels[cid].tripped = false;

      //change our output pin to reflect
      pwm_channels[cid].updateOutput();
    }

    //our duty cycle
    if (input.containsKey("duty"))
    {
      //is it enabled?
      if (!pwm_channels[cid].isEnabled)
        return generateErrorJSON(output, "Channel is not enabled.");

      float duty = input["duty"];

      //what do we hate?  va-li-date!
      if (duty < 0)
        return generateErrorJSON(output, "Duty cycle must be >= 0");
      else if (duty > 1)
        return generateErrorJSON(output, "Duty cycle must be <= 1");

      //okay, we're good.
      pwm_channels[cid].setDuty(duty);

      //change our output pin to reflect
      pwm_channels[cid].updateOutput();
    }

    //channel name
    if (input.containsKey("name"))
    {
      //is it too long?
      if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH-1)
      {
        char error[50];
        sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH-1);
        return generateErrorJSON(output, error);
      }

      //save to our storage
      strlcpy(pwm_channels[cid].name, input["name"] | "Channel ?", sizeof(pwm_channels[cid].name));
      sprintf(prefIndex, "pwmName%d", cid);
      preferences.putString(prefIndex, pwm_channels[cid].name);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //dimmability
    if (input.containsKey("isDimmable"))
    {
      bool isDimmable = input["isDimmable"];
      pwm_channels[cid].isDimmable = isDimmable;

      //save to our storage
      sprintf(prefIndex, "pwmDimmable%d", cid);
      preferences.putBool(prefIndex, isDimmable);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //enabled
    if (input.containsKey("enabled"))
    {
      //save right nwo.
      bool enabled = input["enabled"];
      pwm_channels[cid].isEnabled = enabled;

      //save to our storage
      sprintf(prefIndex, "pwmEnabled%d", cid);
      preferences.putBool(prefIndex, enabled);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //soft fuse
    if (input.containsKey("softFuse"))
    {
      //i crave validation!
      float softFuse = input["softFuse"];
      softFuse = constrain(softFuse, 0.01, 20.0);

      //save right nwo.
      pwm_channels[cid].softFuseAmperage = softFuse;

      //save to our storage
      sprintf(prefIndex, "pwmSoftFuse%d", cid);
      preferences.putFloat(prefIndex, softFuse);

      //give them the updated config
      return generateConfigJSON(output);
    }
  #else
    return generateErrorJSON(output, "Board does not have output channels.");
  #endif
}

void handleTogglePWMChannel(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_PWM_CHANNELS
    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidPWMChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    //keep track of how many toggles
    pwm_channels[cid].stateChangeCount++;

    //record our new state
    pwm_channels[cid].state = !pwm_channels[cid].state;

    //reset soft fuse when we turn on
    if (pwm_channels[cid].state)
      pwm_channels[cid].tripped = false;

    //change our output pin to reflect
    pwm_channels[cid].updateOutput();
  #else
    return generateErrorJSON(output, "Board does not have output channels.");
  #endif
}

void handleFadePWMChannel(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_PWM_CHANNELS
    unsigned long start = micros();
    unsigned long t1, t2, t3, t4 = 0;

    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");
    if (!input.containsKey("duty"))
      return generateErrorJSON(output, "'duty' is a required parameter");
    if (!input.containsKey("millis"))
      return generateErrorJSON(output, "'millis' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidPWMChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    float duty = input["duty"];

    //what do we hate?  va-li-date!
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

    if (finish-start > 10000)
    {
      Serial.println("led fade");
      Serial.printf("params: %dus\n", t1-start); 
      Serial.printf("channelSetDuty: %dus\n", t2-t1); 
      Serial.printf("channelFade: %dus\n", t3-t2); 
      Serial.printf("total: %dus\n", finish-start);
      Serial.println();
    }
  #else
    return generateErrorJSON(output, "Board does not have output channels.");
  #endif
}

void handleSetSwitch(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_INPUT_CHANNELS
    char prefIndex[YB_PREF_KEY_LENGTH];

    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidInputChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    //channel name
    if (input.containsKey("name"))
    {
      //is it too long?
      if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH-1)
      {
        char error[50];
        sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH-1);
        return generateErrorJSON(output, error);
      }

      //save to our storage
      strlcpy(input_channels[cid].name, input["name"] | "Input ?", sizeof(input_channels[cid].name));
      sprintf(prefIndex, "iptName%d", cid);
      preferences.putString(prefIndex, input_channels[cid].name);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //enabled
    if (input.containsKey("enabled"))
    {
      //save right nwo.
      bool enabled = input["enabled"];
      input_channels[cid].isEnabled = enabled;

      //save to our storage
      sprintf(prefIndex, "iptEnabled%d", cid);
      preferences.putBool(prefIndex, enabled);

      //give them the updated config
      return generateConfigJSON(output);
    }
  #else
    return generateErrorJSON(output, "Board does not have input channels.");
  #endif
}

void handleSetRGB(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_RGB_CHANNELS
    char prefIndex[YB_PREF_KEY_LENGTH];

    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidRGBChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    //channel name
    if (input.containsKey("name"))
    {
      //is it too long?
      if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH-1)
      {
        char error[50];
        sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH-1);
        return generateErrorJSON(output, error);
      }

      //save to our storage
      strlcpy(rgb_channels[cid].name, input["name"] | "RGB ?", sizeof(rgb_channels[cid].name));
      sprintf(prefIndex, "rgbName%d", cid);
      preferences.putString(prefIndex, rgb_channels[cid].name);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //enabled
    if (input.containsKey("enabled"))
    {
      //save right nwo.
      bool enabled = input["enabled"];
      rgb_channels[cid].isEnabled = enabled;

      //save to our storage
      sprintf(prefIndex, "rgbEnabled%d", cid);
      preferences.putBool(prefIndex, enabled);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //new color?
    if (input.containsKey("red") || input.containsKey("green") || input.containsKey("blue"))
    {
      float red = rgb_channels[cid].red;
      float green = rgb_channels[cid].green;
      float blue = rgb_channels[cid].blue;

      //what do we hate?  va-li-date!
      if (input.containsKey("red"))
      {
        red = input["red"];
        if (red < 0)
          return generateErrorJSON(output, "Red must be >= 0");
        else if (red > 1)
          return generateErrorJSON(output, "Red must be <= 1");
      }

      //what do we hate?  va-li-date!
      if (input.containsKey("green"))
      {
        green = input["green"];
        if (green < 0)
          return generateErrorJSON(output, "Green must be >= 0");
        else if (green > 1)
          return generateErrorJSON(output, "Green must be <= 1");
      }

      //what do we hate?  va-li-date!
      if (input.containsKey("blue"))
      {
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

void handleSetADC(JsonVariantConst input, JsonVariant output)
{
  #ifdef YB_HAS_ADC_CHANNELS
    char prefIndex[YB_PREF_KEY_LENGTH];

    //id is required
    if (!input.containsKey("id"))
      return generateErrorJSON(output, "'id' is a required parameter");

    //is it a valid channel?
    byte cid = input["id"];
    if (!isValidADCChannel(cid))
      return generateErrorJSON(output, "Invalid channel id");

    //channel name
    if (input.containsKey("name"))
    {
      //is it too long?
      if (strlen(input["name"]) > YB_CHANNEL_NAME_LENGTH-1)
      {
        char error[50];
        sprintf(error, "Maximum channel name length is %s characters.", YB_CHANNEL_NAME_LENGTH-1);
        return generateErrorJSON(output, error);
      }

      //save to our storage
      strlcpy(adc_channels[cid].name, input["name"] | "ADC ?", sizeof(adc_channels[cid].name));
      sprintf(prefIndex, "adcName%d", cid);
      preferences.putString(prefIndex, adc_channels[cid].name);

      //give them the updated config
      return generateConfigJSON(output);
    }

    //enabled
    if (input.containsKey("enabled"))
    {
      //save right nwo.
      bool enabled = input["enabled"];
      adc_channels[cid].isEnabled = enabled;

      //save to our storage
      sprintf(prefIndex, "adcEnabled%d", cid);
      preferences.putBool(prefIndex, enabled);

      //give them the updated config
      return generateConfigJSON(output);
    }
  #else
    return generateErrorJSON(output, "Board does not have ADC channels.");
  #endif
}

void generateConfigJSON(JsonVariant output)
{
  //our identifying info
  output["msg"] = "config";
  output["firmware_version"] = YB_FIRMWARE_VERSION;
  output["hardware_version"] = YB_HARDWARE_VERSION;
  output["name"] = board_name;
  output["hostname"] = local_hostname;
  output["use_ssl"] = app_enable_ssl;
  output["uuid"] = uuid;

  //some debug info
  output["last_restart_reason"] = getResetReason();
  if (has_coredump)
    output["has_coredump"] = has_coredump;

  #ifdef YB_HAS_BUS_VOLTAGE
    output["bus_voltage"] = true;
  #endif

  //do we want to flag it for config?
  if (is_first_boot)
    output["first_boot"] = true;

  //output / pwm channels
  #ifdef YB_HAS_PWM_CHANNELS
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
      output["pwm"][i]["id"] = i;
      output["pwm"][i]["name"] = pwm_channels[i].name;
      output["pwm"][i]["enabled"] = pwm_channels[i].isEnabled;
      output["pwm"][i]["hasCurrent"] = true;
      output["pwm"][i]["softFuse"] = round2(pwm_channels[i].softFuseAmperage);
      output["pwm"][i]["isDimmable"] = pwm_channels[i].isDimmable;
    }
  #endif

  //input / digital IO channels
  #ifdef YB_HAS_INPUT_CHANNELS
    for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
      output["switches"][i]["id"] = i;
      output["switches"][i]["name"] = input_channels[i].name;
      output["switches"][i]["enabled"] = input_channels[i].isEnabled;
    }
  #endif

  //input / analog ADC channesl
  #ifdef YB_HAS_ADC_CHANNELS
    for (byte i = 0; i < YB_ADC_CHANNEL_COUNT; i++) {
      output["adc"][i]["id"] = i;
      output["adc"][i]["name"] = adc_channels[i].name;
      output["adc"][i]["enabled"] = adc_channels[i].isEnabled;
    }
  #endif

  //input / analog ADC channesl
  #ifdef YB_HAS_RGB_CHANNELS
    for (byte i = 0; i < YB_RGB_CHANNEL_COUNT; i++) {
      output["rgb"][i]["id"] = i;
      output["rgb"][i]["name"] = rgb_channels[i].name;
      output["rgb"][i]["enabled"] = rgb_channels[i].isEnabled;
    }
  #endif
}

void generateUpdateJSON(JsonVariant output)
{
  output["msg"] = "update";
  output["uptime"] = millis();

  #ifdef YB_HAS_BUS_VOLTAGE
    output["bus_voltage"] = busVoltage;
  #endif

  #ifdef YB_HAS_PWM_CHANNELS
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
      output["pwm"][i]["id"] = i;
      output["pwm"][i]["state"] = pwm_channels[i].state;
      if (pwm_channels[i].isDimmable)
        output["pwm"][i]["duty"] = round2(pwm_channels[i].dutyCycle);

      output["pwm"][i]["current"] = round2(pwm_channels[i].amperage);
      output["pwm"][i]["aH"] = round3(pwm_channels[i].ampHours);
      output["pwm"][i]["wH"] = round3(pwm_channels[i].wattHours);

      if (pwm_channels[i].tripped)
        output["pwm"][i]["soft_fuse_tripped"] = true;
    }
  #endif

  #ifdef YB_HAS_INPUT_CHANNELS
    for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
      output["switches"][i]["id"] = i;
      output["switches"][i]["isOpen"] = input_channels[i].state;
    }
  #endif

  //input / analog ADC channesl
  #ifdef YB_HAS_ADC_CHANNELS
    for (byte i = 0; i < YB_ADC_CHANNEL_COUNT; i++) {
      output["adc"][i]["id"] = i;
      output["adc"][i]["voltage"] = adc_channels[i].getVoltage();
      output["adc"][i]["reading"] = adc_channels[i].getReading();
      output["adc"][i]["percentage"] = 100.0 * (float)adc_channels[i].getReading() / (pow(2, YB_ADC_RESOLUTION)-1);
      adc_channels[i].resetAverage();
    }
  #endif

  //input / analog ADC channesl
  #ifdef YB_HAS_RGB_CHANNELS
    for (byte i = 0; i < YB_RGB_CHANNEL_COUNT; i++) {
      output["rgb"][i]["id"] = i;
      output["rgb"][i]["red"] = rgb_channels[i].red;
      output["rgb"][i]["green"] = rgb_channels[i].green;
      output["rgb"][i]["blue"] = rgb_channels[i].blue;
    }
  #endif
}

void generateFastUpdateJSON(JsonVariant output)
{
  output["msg"] = "update";
  output["uptime"] = millis();

  #ifdef YB_HAS_BUS_VOLTAGE
    output["bus_voltage"] = busVoltage;
  #endif

  #ifdef YB_HAS_PWM_CHANNELS
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
      output["pwm"][i]["id"] = i;
      output["pwm"][i]["state"] = pwm_channels[i].state;
      if (pwm_channels[i].isDimmable)
        output["pwm"][i]["duty"] = round2(pwm_channels[i].dutyCycle);

      output["pwm"][i]["current"] = round2(pwm_channels[i].amperage);
      output["pwm"][i]["aH"] = round3(pwm_channels[i].ampHours);
      output["pwm"][i]["wH"] = round3(pwm_channels[i].wattHours);

      if (pwm_channels[i].tripped)
        output["pwm"][i]["soft_fuse_tripped"] = true;
    }
  #endif

  #ifdef YB_HAS_INPUT_CHANNELS
    byte j = 0;
    for (byte i = 0; i < YB_INPUT_CHANNEL_COUNT; i++) {
      if (input_channels[i].sendFastUpdate)
      {
        output["switches"][j]["id"] = i;
        output["switches"][j]["isOpen"] = input_channels[i].state;
        input_channels[i].sendFastUpdate = false;
        j++;
      }
    }
  #endif
}

void generateStatsJSON(JsonVariant output)
{
  //some basic statistics and info
  output["msg"] = "stats";
  output["uuid"] = uuid;
  output["messages"] = totalHandledMessages;
  output["fps"] = (int)framerate;
  output["uptime"] = millis();
  output["heap_size"] = ESP.getHeapSize();
  output["free_heap"] = ESP.getFreeHeap();
  output["min_free_heap"] = ESP.getMinFreeHeap();
  output["max_alloc_heap"] = ESP.getMaxAllocHeap();
  output["rssi"] = WiFi.RSSI();

  //what is our IP address?
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
      output["switches"][i]["state_change_count"] = input_channels[i].stateChangeCount;
    }
  #endif

  #ifdef YB_HAS_PWM_CHANNELS
    //info about each of our channels
    for (byte i = 0; i < YB_PWM_CHANNEL_COUNT; i++) {
      output["pwm"][i]["id"] = i;
      output["pwm"][i]["name"] = pwm_channels[i].name;
      output["pwm"][i]["aH"] = pwm_channels[i].ampHours;
      output["pwm"][i]["wH"] = pwm_channels[i].wattHours;
      output["pwm"][i]["state_change_count"] = pwm_channels[i].stateChangeCount;
      output["pwm"][i]["soft_fuse_trip_count"] = pwm_channels[i].softFuseTripCount;
    }
  #endif

  #ifdef YB_HAS_FANS
    //info about each of our fans
    for (byte i = 0; i < YB_FAN_COUNT; i++)
      output["fans"][i]["rpm"] = fans_last_rpm[i];
  #endif
}

void generateNetworkConfigJSON(JsonVariant output)
{
  //our identifying info
  output["msg"] = "network_config";
  output["wifi_mode"] = wifi_mode;
  output["wifi_ssid"] = wifi_ssid;
  output["wifi_pass"] = wifi_pass;
  output["local_hostname"] = local_hostname;
}

void generateAppConfigJSON(JsonVariant output)
{
  //our identifying info
  output["msg"] = "app_config";
  output["require_login"] = require_login;
  output["app_user"] = app_user;
  output["app_pass"] = app_pass;
  output["app_enable_api"] = app_enable_api;
  output["app_enable_serial"] = app_enable_serial;
  output["app_enable_ssl"] = app_enable_ssl;
  output["server_cert"] = server_cert;
  output["server_key"] = server_key;
}

void generateOTAProgressUpdateJSON(JsonVariant output, float progress)
{
  output["msg"] = "ota_progress";
  output["progress"] = round2(progress);
}

void generateOTAProgressFinishedJSON(JsonVariant output)
{
  output["msg"] = "ota_finished";
}

void generateErrorJSON(JsonVariant output, const char* error)
{
  output["msg"] = "status";
  output["status"] = "error";
  output["message"] = error;
}

void generateSuccessJSON(JsonVariant output, const char* success)
{
  output["msg"] = "status";
  output["status"] = "success";
  output["message"] = success;
}

void generateLoginRequiredJSON(JsonVariant output)
{
  generateErrorJSON(output, "You must be logged in.");
}

void generatePongJSON(JsonVariant output)
{
  output["pong"] = millis();
}

void sendFastUpdate()
{
  //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  char jsonBuffer[YB_MAX_JSON_LENGTH];

  generateFastUpdateJSON(output);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);

  Serial.println("Fast Update: ");
  Serial.println(jsonBuffer);
}


void sendUpdate()
{
  //StaticJsonDocument<YB_LARGE_JSON_SIZE> output;
  DynamicJsonDocument output(YB_LARGE_JSON_SIZE);

  char jsonBuffer[YB_MAX_JSON_LENGTH];

  generateUpdateJSON(output);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendOTAProgressUpdate(float progress)
{
  StaticJsonDocument<256> output;
  char jsonBuffer[256];

  generateOTAProgressUpdateJSON(output, progress);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendOTAProgressFinished()
{
  StaticJsonDocument<256> output;
  char jsonBuffer[256];

  generateOTAProgressFinishedJSON(output);

  serializeJson(output, jsonBuffer);
  sendToAll(jsonBuffer);
}

void sendToAll(const char * jsonString)
{
  sendToAllWebsockets(jsonString);

  if (app_enable_serial)
    Serial.println(jsonString);
}

