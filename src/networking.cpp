/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "networking.h"
#include "debug.h"
#include "prefs.h"
#include "yb_server.h"

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
bool is_first_boot = true;

void network_setup()
{
  uint64_t chipid = ESP.getEfuseMac(); // unique 48-bit MAC base ID
  snprintf(uuid, sizeof(uuid), "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  // get an IP address
  setupWifi();
}

void network_loop()
{
  // run our dns... for AP mode
  if (!strcmp(wifi_mode, "ap"))
    dnsServer.processNextRequest();
}

void setupWifi()
{
  // some global config
  // WiFi.setSleep(false);
  // WiFi.useStaticBuffers(true);  //from: https://github.com/espressif/arduino-esp32/issues/7183
  WiFi.setHostname(local_hostname);

  YBP.print("Hostname: ");
  YBP.print(local_hostname);
  YBP.println(".local");

  // which mode do we want?
  if (!strcmp(wifi_mode, "client")) {
    YBP.print("Client mode: ");
    YBP.print(wifi_ssid);
    YBP.print(" / ");
    YBP.println(wifi_pass);

    // try and connect
    connectToWifi(wifi_ssid, wifi_pass);
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

  // setup our local name.
  if (!MDNS.begin(local_hostname)) {
    YBP.println("Error starting mDNS");
    return;
  }
  MDNS.addService("http", "tcp", 80);
}

bool connectToWifi(const char* ssid, const char* pass)
{
  YBP.print("[WiFi] Connecting to ");
  YBP.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, pass);

  // How long to try for?
  int tryDelay = 500;
  int numberOfTries = 60;

  // Wait for the WiFi event
  while (true) {
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        YBP.println("[WiFi] SSID not found");
        return false;
        break;
      case WL_CONNECT_FAILED:
        YBP.print("[WiFi] Failed");
        break;
      case WL_CONNECTION_LOST:
        YBP.println("[WiFi] Connection was lost");
        break;
      case WL_SCAN_COMPLETED:
        YBP.println("[WiFi] Scan is completed");
        break;
      case WL_DISCONNECTED:
        YBP.println("[WiFi] WiFi is disconnected");
        break;
      case WL_CONNECTED:
        YBP.println("[WiFi] WiFi is connected!");
        YBP.print("[WiFi] IP address: ");
        YBP.println(WiFi.localIP());

        // #ifdef YB_HAS_STATUS_WS2818
        //         status_led.setPixelColor(0, status_led.Color(0, 255, 0));
        //         status_led.show();
        // #endif

        return true;
        break;
      default:
        YBP.print("[WiFi] WiFi Status: ");
        YBP.println(WiFi.status());
        break;
    }

    // have we hit our limit?
    if (numberOfTries <= 0) {
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return false;
    } else {
      numberOfTries--;
    }

    delay(tryDelay);
  }

  return false;
}