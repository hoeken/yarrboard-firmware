/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PROTOCOL_H
#define YARR_PROTOCOL_H

#include "adchelper.h"
#include "debug.h"
#include "network.h"
#include "ota.h"
#include "prefs.h"
#include "server.h"
#include "utility.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PsychicHttp.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef YB_HAS_INPUT_CHANNELS
  #include "input_channel.h"
#endif

#ifdef YB_HAS_ADC_CHANNELS
  #include "adc_channel.h"
#endif

#ifdef YB_HAS_RGB_CHANNELS
  #include "rgb_channel.h"
#endif

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_RELAY_CHANNELS
  #include "relay_channel.h"
#endif

#ifdef YB_HAS_SERVO_CHANNELS
  #include "servo_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

#ifdef YB_HAS_BUS_VOLTAGE
  #include "bus_voltage.h"
#endif

#ifdef YB_IS_BRINEOMATIC
  #include "brineomatic.h"
#endif

// extern unsigned int handledMessages;
extern char board_name[YB_BOARD_NAME_LENGTH];
extern char admin_user[YB_USERNAME_LENGTH];
extern char admin_pass[YB_PASSWORD_LENGTH];
extern char guest_user[YB_USERNAME_LENGTH];
extern char guest_pass[YB_PASSWORD_LENGTH];
extern bool app_enable_api;
extern bool app_enable_serial;
extern bool app_enable_ota;
extern bool app_enable_ssl;
extern bool app_enable_mfd;
extern bool is_serial_authenticated;
extern UserRole app_default_role;
extern UserRole serial_role;
extern UserRole api_role;
extern float globalBrightness;

extern unsigned int sentMessages;
extern unsigned long totalSentMessages;
extern unsigned int websocketClientCount;
extern unsigned int httpClientCount;

void protocol_setup();
void protocol_loop();

void handleSerialJson();

void handleReceivedJSON(JsonVariantConst input, JsonVariant output, YBMode mode, PsychicWebSocketClient* connection = NULL);
void handleSetBoardName(JsonVariantConst input, JsonVariant output);
void handleSetNetworkConfig(JsonVariantConst input, JsonVariant output);
void handleSetAppConfig(JsonVariantConst input, JsonVariant output);
void handleLogin(JsonVariantConst input, JsonVariant output, YBMode mode, PsychicWebSocketClient* connection = NULL);
void handleLogout(JsonVariantConst input, JsonVariant output, YBMode mode, PsychicWebSocketClient* connection = NULL);
void handleRestart(JsonVariantConst input, JsonVariant output);
void handleFactoryReset(JsonVariantConst input, JsonVariant output);
void handleOTAStart(JsonVariantConst input, JsonVariant output);
void handleConfigPWMChannel(JsonVariantConst input, JsonVariant output);
void handleSetPWMChannel(JsonVariantConst input, JsonVariant output);
void handleTogglePWMChannel(JsonVariantConst input, JsonVariant output);
void handleFadePWMChannel(JsonVariantConst input, JsonVariant output);
void handleConfigRelayChannel(JsonVariantConst input, JsonVariant output);
void handleSetRelayChannel(JsonVariantConst input, JsonVariant output);
void handleToggleRelayChannel(JsonVariantConst input, JsonVariant output);
void handleConfigServoChannel(JsonVariantConst input, JsonVariant output);
void handleSetServoChannel(JsonVariantConst input, JsonVariant output);
void handleSetSwitch(JsonVariantConst input, JsonVariant output);
void handleConfigSwitch(JsonVariantConst input, JsonVariant output);
void handleConfigRGB(JsonVariantConst input, JsonVariant output);
void handleSetRGB(JsonVariantConst input, JsonVariant output);
void handleConfigADC(JsonVariantConst input, JsonVariant output);
void handleSetTheme(JsonVariantConst input, JsonVariant output);
void handleSetBrightness(JsonVariantConst input, JsonVariant output);

void handleStartWatermaker(JsonVariantConst input, JsonVariant output);
void handleFlushWatermaker(JsonVariantConst input, JsonVariant output);
void handlePickleWatermaker(JsonVariantConst input, JsonVariant output);
void handleDepickleWatermaker(JsonVariantConst input, JsonVariant output);
void handleStopWatermaker(JsonVariantConst input, JsonVariant output);
void handleWaterMakerDiverterValve(JsonVariantConst input, JsonVariant output);

void generateHelloJSON(JsonVariant output, UserRole role);
void generateUpdateJSON(JsonVariant output);
void generateUpdateJSON(JsonVariant output);
void generateFastUpdateJSON(JsonVariant output);
void generateConfigJSON(JsonVariant output);
void generateStatsJSON(JsonVariant output);
void generateNetworkConfigJSON(JsonVariant output);
void generateAppConfigJSON(JsonVariant output);
void generateOTAProgressUpdateJSON(JsonVariant output, float progress);
void generateOTAProgressFinishedJSON(JsonVariant output);
void generateErrorJSON(JsonVariant output, const char* error);
void generateSuccessJSON(JsonVariant output, const char* success);
void generateLoginRequiredJSON(JsonVariant output);
void generateInvalidChannelJSON(JsonVariant output, byte cid);
void generatePongJSON(JsonVariant output);

void sendBrightnessUpdate();
void sendThemeUpdate();
void sendFastUpdate();
void sendOTAProgressUpdate(float progress);
void sendOTAProgressFinished();
void sendDebug(const char* format, ...);
void sendToAll(const char* jsonString, UserRole auth_level);

#endif /* !YARR_PROTOCOL_H */