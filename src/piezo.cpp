/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "piezo.h"

#ifdef YB_HAS_PIEZO

  #define LEDC_RES_BITS 10
  #define BUZZER_DUTY   512 // ~50% at 10-bit

// Example melody (C scale up, rest, then down)
static const Note MELODY_EXAMPLE[] = {
  {262, 100},
  {294, 100},
  {330, 100},
  {349, 100},
  {392, 100},
  {440, 100},
  {494, 100},
  {523, 100},
};

// ---------- Buzzer task control ----------
static TaskHandle_t buzzerTaskHandle = nullptr;
static const Note* g_seq = nullptr;
static size_t g_len = 0;
static bool g_repeat = false;
static portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;

void piezo_setup()
{
  pinMode(YB_PIEZO_PIN, OUTPUT);

  // LEDC once
  ledcAttach(YB_PIEZO_PIN, 1000, LEDC_RES_BITS);
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

  // Kick an example (one-shot). Use true for repeat.
  playMelody(MELODY_EXAMPLE, sizeof(MELODY_EXAMPLE) / sizeof(MELODY_EXAMPLE[0]), false);
}

// Signal the task to start (or restart) a melody.
// Safe to call from loop/ISRs (but here we use regular context).
void playMelody(const Note* seq, size_t len, bool repeat)
{
  portENTER_CRITICAL(&g_mux);
  g_seq = seq;
  g_len = len;
  g_repeat = repeat;
  portEXIT_CRITICAL(&g_mux);
  if (buzzerTaskHandle)
    xTaskNotifyGive(buzzerTaskHandle); // wake the task
}

void stopMelody()
{
  portENTER_CRITICAL(&g_mux);
  g_seq = nullptr;
  g_len = 0;
  portEXIT_CRITICAL(&g_mux);
  if (buzzerTaskHandle)
    xTaskNotifyGive(buzzerTaskHandle); // wake to stop
}

// ---------- Low-level helpers ----------
static inline void buzzerMute()
{
  ledcWrite(YB_PIEZO_PIN, 0);
}

static inline void buzzerTone(uint16_t freqHz)
{
  if (freqHz == 0) {
    buzzerMute(); // rest
  } else {
    ledcWrite(YB_PIEZO_PIN, BUZZER_DUTY);
    ledcWriteTone(YB_PIEZO_PIN, freqHz); // hardware retune
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
    const Note* seq;
    size_t len;
    bool repeat;
    {
      portENTER_CRITICAL(&g_mux);
      seq = g_seq;
      len = g_len;
      repeat = g_repeat;
      portEXIT_CRITICAL(&g_mux);
    }

    if (!seq || len == 0) {
      // Nothing to play
      buzzerMute();
      continue;
    }

    // Play loop (supports repeat); we also check for new requests between notes
    do {
      for (size_t i = 0; i < len; ++i) {
        // Allow mid-play interruption: if a new request arrived, break out
        const Note* nowSeq;
        size_t nowLen;
        bool nowRepeat;
        portENTER_CRITICAL(&g_mux);
        nowSeq = g_seq;
        nowLen = g_len;
        nowRepeat = g_repeat;
        portEXIT_CRITICAL(&g_mux);
        if (nowSeq != seq || nowLen != len || nowRepeat != repeat) {
          // A new melody was posted; restart outer loop to pick it up
          seq = nowSeq;
          len = nowLen;
          repeat = nowRepeat;
          goto restart_play; // neat & safe in this small scope
        }

        // Do the note
        buzzerTone(seq[i].freqHz);

        // Precise sleep for note duration (only this task sleeps)
        TickType_t xLastWake = xTaskGetTickCount();
        TickType_t durTicks = pdMS_TO_TICKS(seq[i].ms);
        vTaskDelayUntil(&xLastWake, durTicks);
      }
    restart_play:;
    } while (repeat);

    // Done (no repeat)
    buzzerMute();
  }
}

#endif