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
#include <CircularBuffer.h>
#include <PsychicHttp.h>

#include "protocol.h"
#include "prefs.h"
#include "network.h"
#include "ota.h"
#include "adchelper.h"

//generated at build by running "gulp" in the firmware directory.
#include "index.html.gz.h"

#ifdef YB_HAS_PWM_CHANNELS
  #include "pwm_channel.h"
#endif

#ifdef YB_HAS_FANS
  #include "fans.h"
#endif

extern String server_cert;
extern String server_key;

typedef struct {
  int client_id;
  char buffer[YB_RECEIVE_BUFFER_LENGTH];
} WebsocketRequest;

void server_setup();
void server_loop();

bool isLoggedIn(JsonVariantConst input, byte mode, PsychicHttpWebSocketConnection *connection);
bool logClientIn(PsychicHttpWebSocketConnection *connection);
void closeClientConnection(PsychicHttpWebSocketConnection *connection);
bool addClientToAuthList(PsychicHttpWebSocketConnection *connection);
bool isWebsocketClientLoggedIn(JsonVariantConst input, PsychicHttpWebSocketConnection *connection);
bool isApiClientLoggedIn(JsonVariantConst input);
bool isSerialClientLoggedIn(JsonVariantConst input);


int getFreeSlots();
int getWebsocketRequestSlot();

void sendToAllWebsockets(const char * jsonString);
void handleWebsocketMessageLoop(WebsocketRequest* request);

esp_err_t handleWebServerRequest(JsonVariant input, PsychicHttpServerRequest *request);
esp_err_t handleWebSocketMessage(PsychicHttpWebSocketRequest *request, uint8_t *data, size_t len);

/*
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
*/


#endif /* !YARR_SERVER_H */