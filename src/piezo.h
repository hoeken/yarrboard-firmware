/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PIEZO_H
#define YARR_PIEZO_H

#include "config.h"
#include "driver/ledc.h"

#ifdef YB_HAS_PIEZO
  #define YB_MAX_MELODY_LENGTH 100

struct Note {
    uint16_t freqHz;
    uint16_t ms;
}; // freq=0 => rest

void piezo_setup();

void playMelody(const Note* seq, size_t len);
bool playMelodyByName(const char* melody);
static inline void buzzerMute();
static inline void buzzerTone(uint16_t freqHz);
void BuzzerTask(void* /*pv*/);

#endif

#endif /* !YARR_PIEZO_H */