/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_SERVER_H
#define YARR_SERVER_H

#include "config.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PsychicHttp.h>
#include <PsychicHttpsServer.h>
#include <freertos/queue.h>

#include "adchelper.h"
#include "network.h"
#include "ota.h"
#include "prefs.h"
#include "protocol.h"

// generated at build by running "gulp" in the firmware directory.
#include "index.html.gz.h"

#ifdef YB_CONFIG_FROTHFET_REV_D
  #include "logo-frothfet.png.gz.h"
#elif YB_CONFIG_BRINEOMATIC_REV_A
  #include "logo-brineomatic.png.gz.h"
#else
  #include "logo-navico.png.gz.h"
#endif

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

typedef struct {
    int socket;
    UserRole role;
} AuthenticatedClient;

typedef struct {
    int socket;
    char* buffer;
    size_t len;
} WebsocketRequest;

extern String server_cert;
extern String server_key;

void server_setup();
void server_loop();

bool isLoggedIn(JsonVariantConst input, byte mode, PsychicWebSocketClient* connection);
bool logClientIn(PsychicWebSocketClient* connection, UserRole role);
bool addClientToAuthList(PsychicWebSocketClient* connection, UserRole role);
void removeClientFromAuthList(PsychicWebSocketClient* connection);
bool isWebsocketClientLoggedIn(JsonVariantConst input, PsychicWebSocketClient* connection);
bool isApiClientLoggedIn(JsonVariantConst doc);
bool isSerialClientLoggedIn(JsonVariantConst input);
bool checkLoginCredentials(JsonVariantConst doc, UserRole& role);
UserRole getUserRole(JsonVariantConst input, byte mode, PsychicWebSocketClient* connection);
UserRole getWebsocketRole(JsonVariantConst doc, PsychicWebSocketClient* connection);

void sendToAllWebsockets(const char* jsonString, UserRole auth_level);
void handleWebsocketMessageLoop(WebsocketRequest* request);

esp_err_t handleWebServerRequest(JsonVariant input, PsychicRequest* request);
void handleWebSocketMessage(PsychicWebSocketRequest* request, uint8_t* data, size_t len);

#endif /* !YARR_SERVER_H */