/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "navico.h"

const int PUBLISH_PORT = 2053;
IPAddress MULTICAST_GROUP_IP(239, 2, 1, 1);

int port;
String protocol;
String url;

unsigned long lastNavicoPublishMillis = 0;

WiFiUDP Udp;

// This code borrowed from the SignalK project:
// https://github.com/SignalK/signalk-server/blob/master/src/interfaces/mfd_webapp.ts
void navico_loop()
{
  if (millis() - lastNavicoPublishMillis > 10000) {
    // which protocol to use?
    if (app_enable_ssl)
      url = "https://" + WiFi.localIP().toString() + ":443";
    else
      url = "http://" + WiFi.localIP().toString() + ":80";

    // generate our config JSON
    JsonDocument doc;

    doc["Version"] = "1";
    doc["Source"] = board_name;
    doc["IP"] = WiFi.localIP();
    doc["FeatureName"] = String(board_name) + " Webapp";

    JsonObject Text_0 = doc["Text"].add<JsonObject>();
    Text_0["Language"] = "en";
    Text_0["Name"] = board_name;
    Text_0["Description"] = String(board_name) + " Webapp";
    doc["Icon"] = url + "/logo.png";
    doc["URL"] = url + "/";
    doc["OnlyShowOnClientIP"] = "true";

    JsonObject BrowserPanel = doc["BrowserPanel"].to<JsonObject>();
    BrowserPanel["Enable"] = true;
    BrowserPanel["ProgressBarEnable"] = true;

    JsonObject BrowserPanel_MenuText_0 =
      BrowserPanel["MenuText"].add<JsonObject>();
    BrowserPanel_MenuText_0["Language"] = "en";
    BrowserPanel_MenuText_0["Name"] = "Home";

    // make our dynamic buffer for the output
    size_t jsonSize = measureJson(doc);
    char* jsonBuffer = (char*)malloc(jsonSize + 1);
    jsonBuffer[jsonSize] = '\0'; // null terminate

    // did we get anything?
    if (jsonBuffer != NULL) {
      serializeJson(doc, jsonBuffer, jsonSize + 1);

      Udp.beginPacket(MULTICAST_GROUP_IP, PUBLISH_PORT);
      Udp.printf(jsonBuffer);
      Udp.endPacket();
    }
    free(jsonBuffer);

    lastNavicoPublishMillis = millis();
  }
}