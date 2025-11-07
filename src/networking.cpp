/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "networking.h"
#include "debug.h"
#include "prefs.h"
#include "rgb.h"
#include "setup.h"

ImprovWiFi improvSerial(&Serial);
ImprovWiFiBLE improvBLE;

// for making a captive portal
//  The default android DNS
IPAddress apIP(8, 8, 4, 4);
const byte DNS_PORT = 53;
DNSServer dnsServer;

// default config info for our wifi
char wifi_ssid[YB_WIFI_SSID_LENGTH] = YB_DEFAULT_AP_SSID;
char wifi_pass[YB_WIFI_PASSWORD_LENGTH] = YB_DEFAULT_AP_PASS;
char wifi_mode[YB_WIFI_MODE_LENGTH] = "ap";
char local_hostname[YB_HOSTNAME_LENGTH] = YB_DEFAULT_HOSTNAME;

// identify yourself!
char uuid[YB_UUID_LENGTH];

void network_setup()
{
  uint64_t chipid = ESP.getEfuseMac(); // unique 48-bit MAC base ID
  snprintf(uuid, sizeof(uuid), "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  setupWifi();
}

void improv_setup()
{
  YBP.println("First Boot: starting Improv");

  String device_url = "http://";
  device_url.concat(local_hostname);
  device_url.concat(".local");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Identify this device
  improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32,
    board_name,
    YB_FIRMWARE_VERSION,
    board_name,
    device_url.c_str());

  improvSerial.onImprovError(onImprovWiFiErrorCb);
  improvSerial.onImprovConnected(onImprovWiFiConnectedCb);
  improvSerial.setCustomConnectWiFi(connectToWifi);

  // Identify this device
  improvBLE.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32,
    board_name,
    YB_FIRMWARE_VERSION,
    board_name,
    device_url.c_str());

  improvBLE.onImprovError(onImprovWiFiErrorCb);
  improvBLE.onImprovConnected(onImprovWiFiConnectedCb);
  improvBLE.setCustomConnectWiFi(connectToWifi);
}

void onImprovWiFiErrorCb(ImprovTypes::Error err)
{
  YBP.printf("wifi error: %d\n", err);

  rgb_set_status_color(CRGB::Red);
}

void onImprovWiFiConnectedCb(const char* ssid, const char* password)
{
  YBP.printf("Improv Successful: %s / %s\n", ssid, password);

  strncpy(wifi_mode, "client", sizeof(wifi_mode));
  strncpy(wifi_ssid, ssid, sizeof(wifi_ssid));
  strncpy(wifi_pass, password, sizeof(wifi_pass));

  char error[128];
  saveConfig(error, sizeof(error));

  full_setup();
  is_first_boot = false;
}

void network_loop()
{
}

void improv_loop()
{
  if (is_first_boot)
    improvSerial.handleSerial();
}

void setupWifi()
{
  // which mode do we want?
  if (!strcmp(wifi_mode, "client")) {
    YBP.print("Client mode: ");
    YBP.print(wifi_ssid);
    YBP.print(" / ");
    YBP.println(wifi_pass);

    // try and connect
    if (connectToWifi(wifi_ssid, wifi_pass))
      start_network_services();
  }
  // default to AP mode.
  else {
    YBP.print("AP mode: ");
    YBP.print(wifi_ssid);
    YBP.print(" / ");
    YBP.println(wifi_pass);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifi_ssid, wifi_pass);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    YBP.print("AP IP address: ");
    YBP.println(apIP);

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);
  }
}

bool connectToWifi(const char* ssid, const char* pass)
{
  rgb_set_status_color(CRGB::Yellow);

  // reset our wifi to a clean state
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.mode(WIFI_STA);

  // some tuning
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false); // optional but usually helps reliability

  // How long to try for?
  int tryDuration = 15000;
  int tryDelay = 50;
  int numberOfTries = tryDuration / tryDelay;

  YBP.print("[WiFi] Connecting to ");
  YBP.println(ssid);
  WiFi.begin(ssid, pass);

  // attempt to connect
  while (numberOfTries > 0) {
    if (WiFi.status() == WL_CONNECTED) {
      YBP.println("\n[WiFi] WiFi is connected!");
      YBP.print("[WiFi] IP address: ");
      YBP.println(WiFi.localIP());
      rgb_set_status_color(CRGB::Green);
      return true;
    }

    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      YBP.println("[WiFi] SSID not found");
      break;
    }

    YBP.print(".");

    numberOfTries--;

    delay(tryDelay);
    yield();
  }

  YBP.println("\n[WiFi] WiFi failed to connect");
  WiFi.disconnect(true, true);
  rgb_set_status_color(CRGB::Red);

  return false;
}

void start_network_services()
{
  // some global config
  WiFi.setHostname(local_hostname);

  YBP.print("Hostname: ");
  YBP.print(local_hostname);
  YBP.println(".local");

  // setup our local name.
  if (!MDNS.begin(local_hostname))
    YBP.println("Error starting mDNS");
  MDNS.addService("http", "tcp", 80);
}