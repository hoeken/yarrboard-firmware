/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_LOGGING_H
#define YARR_LOGGING_H

#include "etl/circular_buffer.h"
#include <Arduino.h>

// Tune to your budget (≈ N * (7 + MSG_BYTES) bytes total, plus small bookkeeping)
#ifndef BOOT_ERR_MAX_ENTRIES
  #define BOOT_ERR_MAX_ENTRIES 16
#endif
#ifndef BOOT_ERR_MSG_BYTES
  #define BOOT_ERR_MSG_BYTES 128 // includes NUL
#endif

struct BootError {
    uint32_t t_ms;
    char msg[BOOT_ERR_MSG_BYTES];
};

// Global ring (oldest overwritten)
inline etl::circular_buffer<BootError, BOOT_ERR_MAX_ENTRIES> yb_boot_errors;

// Single entry point: formatted log (overwrites oldest when full)
inline void boot_error(const char* error)
{
  BootError e;
  e.t_ms = millis();
  strncpy(e.msg, error, sizeof(e.msg));
  if (yb_boot_errors.full())
    yb_boot_errors.pop();
  yb_boot_errors.push(e);
}

#endif /* !YARR_LOGGING_H */