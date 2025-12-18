/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "config.h"

#include "BrineomaticController.h"
#include <YarrboardApp.h>
#include <YarrboardDebug.h>

BrineomaticController::BrineomaticController(YarrboardApp& app) : BaseController(app, "bus_voltage"),
                                                                  wm(app)
{
}

bool BrineomaticController::setup()
{
  wm.init();

  // Create a FreeRTOS task for the state machine
  xTaskCreatePinnedToCore(
    BrineomaticController::stateMachineTask, // Task function
    "brineomatic_sm",                        // Name of the task
    4096,                                    // Stack size
    this,                                    // Task input parameters
    2,                                       // Priority of the task
    NULL,                                    // Task handle
    1                                        // Core where the task should run
  );

  return true;
}

void BrineomaticController::loop()
{
  wm.loop();
}

void BrineomaticController::stateMachineTask(void* pvParameters)
{
  // Cast the void pointer back to our class type
  BrineomaticController* instance = static_cast<BrineomaticController*>(pvParameters);

  // Call the actual member function
  instance->stateMachine();
}

void BrineomaticController::stateMachine()
{
  while (true) {
    wm.runStateMachine();

    // Add a delay to prevent task starvation
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void BrineomaticController::generateConfigHook(JsonVariant config)
{
}

void BrineomaticController::generateUpdateHook(JsonVariant output) {
};

void BrineomaticController::generateFastUpdateHook(JsonVariant output) {
};

void BrineomaticController::generateStatsHook(JsonVariant output) {
};