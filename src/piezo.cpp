/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "piezo.h"
#include "debug.h"

#ifdef YB_HAS_PIEZO

  #define LEDC_RES_BITS 10
  #define BUZZER_DUTY   512 // ~50% at 10-bit

// our global note buffer
static Note g_noteBuffer[YB_MAX_MELODY_LENGTH]; // pick a safe max size
static size_t g_noteCount = 0;

// ---------- Buzzer task control ----------
static TaskHandle_t buzzerTaskHandle = nullptr;
static const Note* g_seq = nullptr;
static size_t g_len = 0;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

  #ifdef YB_PIEZO_ACTIVE
bool piezoIsActive = true;
  #elif defined(YB_PIEZO_PASSIVE)
bool piezoIsActive = false;
  #else
bool piezoIsActive = false;
  #endif

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
  {3000, 50},
  {0, 50},
  {3000, 50},
  {0, 50},
  {3000, 50},
};

// Short–short–long.
static const Note ACTIVE_SUCCESS[] = {
  {3000, 120},
  {0, 50},
  {3000, 120},
  {0, 50},
  {3000, 250},
};

// Long, short, long with pauses.
static const Note ACTIVE_ERROR[] = {
  {3000, 250},
  {0, 120},
  {3000, 120},
  {0, 120},
  {3000, 350},
};

// Three fast pulses.
static const Note ACTIVE_WARNING[] = {
  {3000, 120},
  {0, 80},
  {3000, 120},
  {0, 80},
  {3000, 120},
};

void piezo_setup()
{
  pinMode(YB_PIEZO_PIN, OUTPUT);

  if (!piezoIsActive) {
    // LEDC once
    if (!ledcAttach(YB_PIEZO_PIN, 1000, LEDC_RES_BITS)) {
      YBP.println("Error attaching piezo to LEDC channel.");
      return;
    } else {
      piezoIsActive = false;
    }
  }

  // shh.
  buzzerMute();

  // Create buzzer task (small stack, low priority)
  // You can pin it to a core if you like; here we let the scheduler decide.
  xTaskCreate(
    BuzzerTask,
    "buzzer_task",
    2048, // stack words
    nullptr,
    1, // low priority is fine
    &buzzerTaskHandle);

  // Kick an example (one-shot)
  if (piezoIsActive)
    playMelodyByName("ACTIVE_STARTUP");
  else
    playMelodyByName("MELODY_STARTUP2");
}

// Signal the task to start (or restart) a melody.
// Safe to call from loop/ISRs (but here we use regular context).
void playMelody(const Note* seq, size_t len)
{
  portENTER_CRITICAL(&g_mux);

  // wait until we're done playing.
  if (g_len > 0 || g_seq != nullptr)
    return;

  // max size
  g_len = len;
  if (g_len > YB_MAX_MELODY_LENGTH)
    return;

  // copy into persistent storage
  for (size_t i = 0; i < len; i++) {
    g_noteBuffer[i] = seq[i];
  }
  g_seq = g_noteBuffer;
  portEXIT_CRITICAL(&g_mux);
  if (buzzerTaskHandle)
    xTaskNotifyGive(buzzerTaskHandle); // wake the task
}

// ---------- Low-level helpers ----------
static inline void buzzerMute()
{
  if (piezoIsActive)
    digitalWrite(YB_PIEZO_PIN, LOW);
  else
    ledcWrite(YB_PIEZO_PIN, 0);
}

static inline void buzzerTone(uint16_t freqHz)
{
  if (freqHz == 0) {
    buzzerMute(); // rest
  } else {
    if (piezoIsActive) {
      digitalWrite(YB_PIEZO_PIN, HIGH);
    } else {
      ledcWriteTone(YB_PIEZO_PIN, freqHz); // hardware retune
    }
  }
}

// ---------- FreeRTOS task ----------
void BuzzerTask(void* /*pv*/)
{
  // Sleep until someone calls playMelody()
  for (;;) {
    // Block here until notified OR timeout (so we can gracefully mute if nothing to do)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // Snapshot the current request
    portENTER_CRITICAL(&g_mux);
    const Note* seq = g_seq;
    size_t len = g_len;
    portEXIT_CRITICAL(&g_mux);

    if (!seq || len == 0) {
      // Nothing to play
      buzzerMute();
      continue;
    }

    for (size_t i = 0; i < len; ++i) {
      // Do the note
      buzzerTone(seq[i].freqHz);

      // Precise sleep for note duration (only this task sleeps)
      TickType_t xLastWake = xTaskGetTickCount();
      TickType_t durTicks = pdMS_TO_TICKS(seq[i].ms);
      if (durTicks > 0)
        vTaskDelayUntil(&xLastWake, durTicks);
    }

    buzzerMute();

    // we're done.
    portENTER_CRITICAL(&g_mux);
    g_seq = nullptr;
    g_len = 0;
    portEXIT_CRITICAL(&g_mux);
  }
}

bool playMelodyByName(const char* melody)
{
  const Note* seq = nullptr;
  size_t len = 0;

  // --- Match name to melody table ---
  if (!strcmp(melody, "MELODY_STARTUP")) {
    seq = MELODY_STARTUP;
    len = sizeof(MELODY_STARTUP) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_STARTUP2")) {
    seq = MELODY_STARTUP2;
    len = sizeof(MELODY_STARTUP2) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_CHEERFUL")) {
    seq = JINGLE_CHEERFUL;
    len = sizeof(JINGLE_CHEERFUL) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_ADVENTURE")) {
    seq = JINGLE_ADVENTURE;
    len = sizeof(JINGLE_ADVENTURE) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_PLAYFUL")) {
    seq = JINGLE_PLAYFUL;
    len = sizeof(JINGLE_PLAYFUL) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_SHANTY")) {
    seq = JINGLE_SHANTY;
    len = sizeof(JINGLE_SHANTY) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_SUCCESS")) {
    seq = MELODY_SUCCESS;
    len = sizeof(MELODY_SUCCESS) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_ERROR")) {
    seq = MELODY_ERROR;
    len = sizeof(MELODY_ERROR) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_WARNING")) {
    seq = MELODY_WARNING;
    len = sizeof(MELODY_WARNING) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_JINGLE1")) {
    seq = MELODY_JINGLE1;
    len = sizeof(MELODY_JINGLE1) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_VICTORY")) {
    seq = MELODY_VICTORY;
    len = sizeof(MELODY_VICTORY) / sizeof(Note);
  } else if (!strcmp(melody, "MELODY_GOOFY")) {
    seq = MELODY_GOOFY;
    len = sizeof(MELODY_GOOFY) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_DUBSTEP")) {
    seq = JINGLE_DUBSTEP;
    len = sizeof(JINGLE_DUBSTEP) / sizeof(Note);
  } else if (!strcmp(melody, "JINGLE_DUBSTEP_MELODIC")) {
    seq = JINGLE_DUBSTEP_MELODIC;
    len = sizeof(JINGLE_DUBSTEP_MELODIC) / sizeof(Note);
  } else if (!strcmp(melody, "ACTIVE_STARTUP")) {
    seq = ACTIVE_STARTUP;
    len = sizeof(ACTIVE_STARTUP) / sizeof(Note);
  } else if (!strcmp(melody, "ACTIVE_SUCCESS")) {
    seq = ACTIVE_SUCCESS;
    len = sizeof(ACTIVE_SUCCESS) / sizeof(Note);
  } else if (!strcmp(melody, "ACTIVE_ERROR")) {
    seq = ACTIVE_ERROR;
    len = sizeof(ACTIVE_ERROR) / sizeof(Note);
  } else if (!strcmp(melody, "ACTIVE_WARNING")) {
    seq = ACTIVE_WARNING;
    len = sizeof(ACTIVE_WARNING) / sizeof(Note);
  } else {
    return false;
  }

  playMelody(seq, len);

  return true;
}

#endif