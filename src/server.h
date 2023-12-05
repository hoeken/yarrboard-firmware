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
#include <freertos/queue.h>

#include "protocol.h"
#include "prefs.h"
#include "network.h"
#include "ota.h"
#include "adchelper.h"

//generated at build by running "gulp" in the firmware directory.
#include "index.html.gz.h"
#include "logo-navico.png.gz.h"

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

typedef struct  {
  int connection_id;
  UserRole role;
} AuthenticatedConnection;

typedef struct {
  int client_id;
  char *buffer;
  size_t len;
} WebsocketRequest;

extern String server_cert;
extern String server_key;

void server_setup();
void server_loop();

bool isLoggedIn(JsonVariantConst input, byte mode, PsychicHttpWebSocketConnection *connection);
bool logClientIn(PsychicHttpWebSocketConnection *connection, UserRole role);
bool addClientToAuthList(PsychicHttpWebSocketConnection *connection, UserRole role);
void removeClientFromAuthList(PsychicHttpWebSocketConnection *connection);
bool isWebsocketClientLoggedIn(JsonVariantConst input, PsychicHttpWebSocketConnection *connection);
bool isApiClientLoggedIn(JsonVariantConst doc);
bool isSerialClientLoggedIn(JsonVariantConst input);
bool checkLoginCredentials(JsonVariantConst doc, UserRole &role);
UserRole getUserRole(JsonVariantConst input, byte mode, PsychicHttpWebSocketConnection *connection);
UserRole getWebsocketRole(JsonVariantConst doc, PsychicHttpWebSocketConnection *connection);

void sendToAllWebsockets(const char * jsonString);
void handleWebsocketMessageLoop(WebsocketRequest* request);

esp_err_t handleWebServerRequest(JsonVariant input, PsychicHttpServerRequest *request);
void handleWebSocketMessage(PsychicHttpWebSocketRequest *request, uint8_t *data, size_t len);

#endif /* !YARR_SERVER_H */