/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_DEBUG_H
#define YARR_DEBUG_H

#include "etl/vector.h"
#include "protocol.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <esp_core_dump.h>
#include <esp_log.h>
#include <esp_partition.h>
#include <esp_system.h>

#ifdef YB_USB_SERIAL
  #include "USB.h"
extern USBCDC USBSerial;
#endif

extern bool has_coredump;

void debug_setup();

String getResetReason();
bool checkCoreDump();
String readCoreDump();
bool deleteCoreDump();
void crash_me_hard();
int debug_log_vprintf(const char* fmt, va_list args);

class YarrboardPrint : public Print
{
  public:
    YarrboardPrint() {}

    void addPrinter(Print& printer)
    {
      if (_printers.full() == false) {
        _printers.push_back(&printer);
      }
    }

    void removePrinter(Print& printer)
    {
      for (auto it = _printers.begin(); it != _printers.end(); ++it) {
        if (*it == &printer) {
          _printers.erase(it);
          break; // done
        }
      }
    }
    size_t write(uint8_t b) override
    {
      for (auto* p : _printers) {
        p->write(b);
      }
      return 1;
    }

  private:
    static constexpr size_t MAX_PRINTERS = 8; // tweak as needed
    etl::vector<Print*, MAX_PRINTERS> _printers;
};

class StringPrint : public Print
{
  public:
    StringPrint()
    {
      value.reserve(1024); // preallocate default buffer
    }

    String value; // gathered output

    size_t write(uint8_t b) override
    {
      value += (char)b;
      return 1;
    }
};

class WebsocketPrint : public Print
{
  public:
    WebsocketPrint()
    {
      _buf.reserve(256); // default preallocation
    }

    size_t write(uint8_t b) override
    {
      if (b == '\n') {
        if (_buf.length() > 0) {
          sendDebug(_buf.c_str());
          _buf = "";
        }
      } else {
        _buf += (char)b;
      }
      return 1;
    }

  private:
    String _buf;
};

extern YarrboardPrint YBP;
extern StringPrint startupLogger;
extern WebsocketPrint networkLogger;

#endif /* !YARR_PREFS_H */