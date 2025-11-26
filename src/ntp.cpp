/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "ntp.h"
#include "debug.h"

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
bool ntp_is_ready = false;

void ntp_setup()
{
  // Setup our NTP to get the current time.
  sntp_set_time_sync_notification_cb(timeAvailable);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

void ntp_loop()
{
}

// Callback function (get's called when time adjusts via NTP)
void timeAvailable(struct timeval* t)
{
  YBP.print("NTP update: ");
  printLocalTime();

  ntp_is_ready = true;
}

void printLocalTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    YBP.println("Failed to obtain time");
    return;
  }

  char buffer[40];
  strftime(buffer, 40, "%FT%T%z", &timeinfo);
  YBP.println(buffer);
}

int64_t ntp_get_time()
{
  time_t now;
  time(&now);

  int64_t seconds = (int64_t)now;

  return (int64_t)now;
}