/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "relay_channel.h"
#include "servo_channel.h"
#include <QuickPID.h>

class RelayChannel;
class ServoChannel;

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
    ServoChannel* diverterValve = NULL;
    ServoChannel* highPressureValve = NULL;

    float diverterValveOpenAngle = 35;
    float diverterValveClosedAngle = 125;

    float highPressureValveOpenMin;
    float highPressureValveOpenMax;
    float highPressureValveCloseMin;
    float highPressureValveCloseMax;

    float currentVolume;
    float totalVolume;

    Brineomatic();

    enum class Status {
      STARTUP,
      IDLE,
      RUNNING,
      STOPPING,
      FLUSHING,
      PICKLING,
      DEPICKLING,
      PICKLED
    };

    enum class Result {
      STARTUP,
      SUCCESS,
      EXTERNAL_STOP,
      ERR_BOOST_PRESSURE_TIMEOUT,
      ERR_FILTER_PRESSURE_LOW,
      ERR_FILTER_PRESSURE_HIGH,
      ERR_MEMBRANE_PRESSURE_LOW,
      ERR_MEMBRANE_PRESSURE_HIGH,
      ERR_FLOWRATE_TIMEOUT,
      ERR_FLOWRATE_LOW,
      ERR_SALINITY_TIMEOUT,
      ERR_SALINITY_HIGH
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

    bool hasHighPressureValve();
    void manageHighPressureValve();

    bool hasBoostPump();
    void enableBoostPump();
    void disableBoostPump();

    const char* getStatus();
    const char* resultToString(Result result);
    Result getRunResult();

    int64_t getNextFlushCountdown();
    int64_t getRuntimeElapsed();
    int64_t getFinishCountdown();
    int64_t getFlushElapsed();
    int64_t getFlushCountdown();
    int64_t getPickleElapsed();
    int64_t getPickleCountdown();

    float getFilterPressure();
    float getFilterPressureMinimum();
    float getMembranePressure();
    float getMembranePressureMinimum();
    float getFlowrate();
    float getFlowrateMinimum();
    float getVolume();
    float getTotalVolume();
    float getTemperature();
    float getSalinity();
    float getSalinityMaximum();

    void runStateMachine();

  private:
    Status currentStatus;
    Result runResult;
    Result flushResult;
    Result pickleResult;
    Result depickleResult;

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

    float totalRuntime;

    float membranePressureTarget;
    float membranePressurePIDOutput;
    QuickPID membranePressurePID;
    float Kp, Ki, Kd;

    float defaultMembranePressureTarget = 750.0; // PSI

    float lowPressureMinimum = 10.0;   // PSI
    float lowPressureMaximum = 60.0;   // PSI
    float highPressureMinimum = 600.0; // PSI
    float highPressureMaximum = 900.0; // PSI
    float flowrateMinimum = 120.0;     // LPH
    // float flowrateMaximum = 200.0;     // LPH
    float salinityMaximum = 200.0; // PPM

    uint8_t membranePressureHighErrorCount = 0;
    uint8_t membranePressureLowErrorCount = 0;
    uint8_t filterPressureHighErrorCount = 0;
    uint8_t filterPressureLowErrorCount = 0;
    uint8_t flowrateLowErrorCount = 0;
    uint8_t salinityHighErrorCount = 0;

    bool checkStopFlag();
    bool checkMembranePressureHigh();
    bool checkMembranePressureLow();
    bool checkFilterPressureHigh();
    bool checkFilterPressureLow();
    bool checkFlowrateLow();
    bool checkSalinityHigh();
    bool checkFlowAndSalinityStable();
};

extern Brineomatic wm;

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */