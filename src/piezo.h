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

void playMelody(const Note* seq, size_t len, bool repeat = false);
static inline void buzzerMute();
static inline void buzzerTone(uint16_t freqHz);
void BuzzerTask(void* /*pv*/);

// Example melody (C scale up)
static const Note MELODY_STARTUP[] = {
  {262, 100},
  {294, 100},
  {330, 100},
  {349, 100},
  {392, 100},
  {440, 100},
  {494, 100},
  {523, 100},
};

static const Note MELODY_STARTUP2[] = {
  {330, 120}, // E4
  {392, 120}, // G4
  {523, 150}, // C5
  {659, 200}, // E5
};

static const Note JINGLE_CHEERFUL[] = {
  {523, 150}, // C5
  {659, 150}, // E5
  {784, 180}, // G5
  {988, 180}, // B5

  {1046, 220}, // C6
  {988, 160},  // B5
  {784, 160},  // G5
  {659, 180},  // E5

  {784, 200},  // G5
  {988, 200},  // B5
  {1175, 260}, // D6
  {1319, 320}, // E6

  {0, 80},     // pause
  {1175, 250}, // D6
  {988, 200},  // B5
  {784, 220},  // G5
};

static const Note JINGLE_ADVENTURE[] = {
  {392, 120}, // G4
  {523, 160}, // C5
  {659, 160}, // E5
  {784, 200}, // G5

  {880, 200},  // A5
  {988, 220},  // B5
  {1046, 260}, // C6
  {0, 80},     // rest

  {1046, 200}, // C6
  {1175, 200}, // D6
  {1319, 260}, // E6
  {0, 60},

  {1319, 220}, // E6
  {1175, 200}, // D6
  {988, 200},  // B5
  {784, 260},  // G5
};

static const Note JINGLE_PLAYFUL[] = {
  {659, 120}, // E5
  {784, 120}, // G5
  {880, 150}, // A5
  {988, 150}, // B5

  {880, 120}, // A5
  {784, 120}, // G5
  {659, 150}, // E5
  {523, 180}, // C5

  {659, 160},  // E5
  {784, 160},  // G5
  {988, 180},  // B5
  {1175, 260}, // D6

  {0, 80},
  {1175, 240}, // D6
  {988, 180},  // B5
  {784, 200},  // G5
};

static const Note JINGLE_SHANTY[] = {
  {392, 200}, // G4
  {440, 200}, // A4
  {494, 250}, // B4
  {523, 250}, // C5

  {494, 200}, // B4
  {440, 200}, // A4
  {392, 250}, // G4
  {330, 280}, // E4

  {392, 200}, // G4
  {494, 200}, // B4
  {523, 250}, // C5
  {587, 300}, // D5

  {0, 80},
  {523, 240}, // C5
  {494, 220}, // B4
  {440, 260}, // A4
};

static const Note MELODY_SUCCESS[] = {
  {523, 90},   // C5
  {659, 90},   // E5
  {784, 140},  // G5
  {1046, 160}, // C6
};

static const Note MELODY_ERROR[] = {
  {392, 160}, // G4
  {330, 160}, // E4
  {262, 220}, // C4
  {0, 120},   // brief rest
  {196, 260}, // G3 (low "bonk")
};

static const Note MELODY_WARNING[] = {
  {988, 120}, // B5
  {0, 80},
  {988, 120},
  {0, 80},
  {988, 200},
};

static const Note MELODY_JINGLE1[] = {
  {784, 150},  // G5
  {880, 150},  // A5
  {988, 180},  // B5
  {784, 150},  // G5
  {988, 180},  // B5
  {1175, 250}, // D6
  {0, 80},
  {1175, 300}, // D6 sustain
};

static const Note MELODY_VICTORY[] = {
  {659, 100},  // E5
  {784, 100},  // G5
  {988, 160},  // B5
  {1319, 220}, // E6
  {0, 60},
  {988, 160},  // B5
  {1175, 220}, // D6
};

static const Note MELODY_GOOFY[] = {
  {523, 120}, // C5
  {659, 120}, // E5
  {392, 180}, // G4
  {0, 70},
  {784, 200}, // G5
};

static const Note JINGLE_DUBSTEP[] = {
  // Intro stab
  {1046, 120}, // C6
  {1175, 120}, // D6
  {1319, 180}, // E6
  {0, 60},

  // Wub-wub-wub pattern (fast gated notes)
  {392, 90}, // G4
  {0, 40},
  {392, 90},
  {0, 40},
  {392, 120},
  {0, 80},

  // Wobble pitch glide 1 (descending)
  {523, 90}, // C5
  {494, 90},
  {440, 90},
  {392, 120},

  // Wub-wub again but lower
  {349, 90}, // F4
  {0, 40},
  {349, 90},
  {0, 40},
  {349, 120},
  {0, 80},

  // Build-up rising pitch
  {440, 90},  // A4
  {523, 90},  // C5
  {587, 100}, // D5
  {659, 120}, // E5
  {784, 140}, // G5
  {988, 160}, // B5

  // DROP!
  {196, 200}, // G3 (sub buzz)
  {0, 60},
  {220, 200}, // A3
  {0, 60},
  {196, 260}, // G3
  {0, 100},

  // Final punchy stabs
  {523, 150}, // C5
  {392, 150}, // G4
  {659, 200}, // E5
};

static const Note JINGLE_DUBSTEP_MELODIC[] = {
  // ==== Melodic Hook ====
  {523, 150}, // C5
  {659, 150}, // E5
  {784, 180}, // G5
  {988, 180}, // B5
  {880, 150}, // A5
  {784, 160}, // G5
  {659, 180}, // E5
  {523, 200}, // C5
  {0, 60},

  // ==== Melodic Wub Pattern ====
  {440, 100}, // A4 (wub 1)
  {0, 40},
  {440, 100}, // A4
  {0, 40},
  {494, 120}, // B4
  {0, 50},

  {523, 130}, // C5 (lift)
  {0, 40},
  {587, 150}, // D5
  {0, 50},

  // ==== Mini Glide Down (EDM style) ====
  {659, 120}, // E5
  {622, 120}, // Eb5
  {587, 130}, // D5
  {523, 150}, // C5
  {0, 70},

  // ==== Build-up (melodic rising arpeggio) ====
  {523, 100},  // C5
  {659, 100},  // E5
  {784, 120},  // G5
  {988, 130},  // B5
  {1175, 140}, // D6
  {1319, 200}, // E6
  {0, 80},

  // ==== Melodic Drop ====
  {392, 150}, // G4 (warm sub-like)
  {0, 40},
  {440, 150}, // A4
  {0, 40},
  {494, 200}, // B4
  {0, 40},

  {784, 240}, // G5 (drop peak)
  {659, 220}, // E5
  {523, 260}, // C5
};

// Example melody (3 beeps)
static const Note ACTIVE_STARTUP[] = {
  {100, 50},
  {0, 50},
  {100, 50},
  {0, 50},
  {100, 50},
};

// Short–short–long.
static const Note ACTIVE_SUCCESS[] = {
  {1000, 120},
  {0, 50},
  {1000, 120},
  {0, 50},
  {1000, 250},
  {0, 0},
};

// Long, short, long with pauses.
static const Note ACTIVE_ERROR[] = {
  {1000, 250},
  {0, 120},
  {1000, 120},
  {0, 120},
  {1000, 350},
  {0, 0},
};

// Three fast pulses.
static const Note ACTIVE_WARNING[] = {
  {1000, 120},
  {0, 80},
  {1000, 120},
  {0, 80},
  {1000, 120},
  {0, 0},
};

#endif

#endif /* !YARR_PIEZO_H */