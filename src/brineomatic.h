/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "relay_channel.h"

class RelayChannel;

void brineomatic_setup();
void brineomatic_loop();

void brineomatic_state_machine(void* pvParameters);

void measure_flowmeter();
void measure_temperature();
void measure_salinity(int16_t reading);
void measure_filter_pressure(int16_t reading);
void measure_membrane_pressure(int16_t reading);

class Brineomatic
{
  public:
    bool isPickled;
    bool autoFlushEnabled;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;

    Brineomatic();

    enum class Status {
      STARTUP,
      IDLE,
      RUNNING,
      FLUSHING,
      PICKLING,
      PICKLED
    };

    void setFilterPressure(float pressure);
    void setMembranePressure(float pressure);
    void setMembranePressureTarget(float pressure);
    void setFlowrate(float flowrate);
    void setTemperature(float temp);
    void setSalinity(float salinity);

    void start();
    void startDuration(uint64_t duration);
    void startVolume(float volume);
    void flush(uint64_t duration);
    void pickle(uint64_t duration);
    void stop();

    void initializeHardware();

    bool hasDiverterValve();
    void openDiverterValve();
    void closeDiverterValve();

    bool hasFlushValve();
    void openFlushValve();
    void closeFlushValve();

    bool hasHighPressurePump();
    void enableHighPressurePump();
    void disableHighPressurePump();

    bool hasBoostPump();
    void enableBoostPump();
    void disableBoostPump();

    const char* getStatus();
    uint64_t getNextFlushCountdown();
    uint64_t getRuntimeElapsed();
    uint64_t getFinishCountdown();
    uint64_t getFlushElapsed();
    uint64_t getFlushCountdown();
    uint64_t getPickleElapsed();
    uint64_t getPickleCountdown();

    float getFilterPressure();
    float getFilterPressureMinimum();
    float getMembranePressure();
    float getMembranePressureMinimum();
    float getFlowrate();
    float getFlowrateMinimum();
    float getTemperature();
    float getSalinity();
    float getSalinityMaximum();

    void runStateMachine();

  private:
    Status currentStatus;
    bool stopFlag = false;

    float desiredVolume = 0;

    // all these times are in microseconds
    uint64_t desiredRuntime = 0;
    uint64_t runtimeStart;
    uint64_t flushStart = 0;
    uint64_t flushDuration = 5ULL * 60 * 1000000; // 5 minute default, in microseconds
    uint64_t nextFlushTime = 0;
    uint64_t flushInterval = 5ULL * 24 * 60 * 60 * 1000000; // 5 day default, in microseconds
    uint64_t pickleStart = 0;
    uint64_t pickleDuration = 5ULL * 60 * 1000000; // 5 minute default, in microseconds

    bool highPressurePumpEnabled;
    bool boostPumpEnabled;
    bool diversionValveOpen;
    bool flushValveOpen;

    float currentTemperature;
    float currentFlowrate;
    float currentSalinity;
    float currentFilterPressure;
    float currentMembranePressure;

    float membranePressureTarget;

    const float lowPressureMinimum = 10.0;   // Example value
    const float highPressureMinimum = 700.0; // Example value
    const float flowrateMinimum = 5.0;       // Example value
    const float salinityMaximum = 500.0;     // Example value
};

extern Brineomatic wm;

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */