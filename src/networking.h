/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_NETWORK_H
#define YARR_NETWORK_H

#include "config.h"
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ImprovWiFiBLE.h>
#include <ImprovWiFiLibrary.h>
#include <WiFi.h>

extern IPAddress apIP;
extern char wifi_ssid[YB_WIFI_SSID_LENGTH];
extern char wifi_pass[YB_WIFI_PASSWORD_LENGTH];
extern char wifi_mode[YB_WIFI_MODE_LENGTH];
extern char local_hostname[YB_HOSTNAME_LENGTH];
extern char uuid[YB_UUID_LENGTH];

void network_setup();
void network_loop();

void start_network_services();

void setupWifi();
bool connectToWifi(const char* ssid, const char* pass);

void improv_setup();
void improv_loop();
void onImprovWiFiErrorCb(ImprovTypes::Error err);
void onImprovWiFiConnectedCb(const char* ssid, const char* password);

#endif /* !YARR_NETWORK_H */