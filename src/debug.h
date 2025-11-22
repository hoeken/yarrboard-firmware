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

#define ARDUINOTRACE_SERIAL YBP
#include <ArduinoTrace.h>

#ifdef YB_USB_SERIAL
  #include "USB.h"
extern USBCDC USBSerial;
#endif

extern bool has_coredump;

void debug_setup();

String getResetReason();
bool checkCoreDump();
bool saveCoreDumpToFile(const char* path);
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
    StringPrint() : _pos(0) {}

    size_t write(uint8_t b) override
    {
      if (_pos < sizeof(_buf) - 1) {
        _buf[_pos++] = (char)b;
        _buf[_pos] = '\0';
      }
      return 1;
    }

    const char* c_str() const { return _buf; }

  private:
    static constexpr size_t BUF_SIZE = 1024;
    char _buf[BUF_SIZE];
    size_t _pos;
};

class WebsocketPrint : public Print
{
  public:
    WebsocketPrint() : _pos(0) {}

    size_t write(uint8_t b) override
    {
      if (b == '\n') {
        if (_pos > 0) {
          _buf[_pos] = '\0';
          sendDebug(_buf);
          _pos = 0; // reset buffer
        }
      } else {
        if (_pos < sizeof(_buf) - 1) {
          _buf[_pos++] = (char)b;
        }
        // else drop chars on overflow (optional)
      }
      return 1;
    }

  private:
    static constexpr size_t BUF_SIZE = 256;
    char _buf[BUF_SIZE];
    size_t _pos;
};

extern YarrboardPrint YBP;
extern StringPrint startupLogger;
extern WebsocketPrint networkLogger;

#endif /* !YARR_PREFS_H */