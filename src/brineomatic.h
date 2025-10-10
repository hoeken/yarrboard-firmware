/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

#include "etl/deque.h"
#include "relay_channel.h"
#include "servo_channel.h"
#include <QuickPID.h>

class RelayChannel;
class ServoChannel;

extern etl::deque<float, YB_BOM_DATA_SIZE> motor_temperature_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> water_temperature_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> filter_pressure_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> membrane_pressure_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> product_salinity_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> brine_salinity_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> product_flowrate_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> brine_flowrate_data;
extern etl::deque<float, YB_BOM_DATA_SIZE> tank_level_data;

void brineomatic_setup();
void brineomatic_loop();

void brineomatic_state_machine(void* pvParameters);

void measure_product_flowmeter();
void measure_brine_flowmeter();
void measure_temperature();
void measure_product_salinity(int16_t reading);
void measure_brine_salinity(int16_t reading);
void measure_filter_pressure(int16_t reading);
void measure_membrane_pressure(int16_t reading);

class Brineomatic
{
  public:
    bool isPickled;
    bool autoFlushEnabled;
    bool flushUseHighPressureMotor = false;

    RelayChannel* flushValve = NULL;
    RelayChannel* boostPump = NULL;
    RelayChannel* highPressurePump = NULL;
    RelayChannel* coolingFan = NULL;
    ServoChannel* diverterValve = NULL;
    ServoChannel* highPressureValve = NULL;

    float diverterValveOpenAngle = 35;
    float diverterValveClosedAngle = 125;

    float highPressureValveOpenMin;
    float highPressureValveOpenMax;
    float highPressureValveCloseMin;
    float highPressureValveCloseMax;
    float highPressureValveMaintainOpenMin;
    float highPressureValveMaintainOpenMax;
    float highPressureValveMaintainCloseMin;
    float highPressureValveMaintainCloseMax;

    float KpRamp = 0;
    float KiRamp = 0;
    float KdRamp = 0;
    float KpMaintain = 0;
    float KiMaintain = 0;
    float KdMaintain = 0;

    float currentVolume;

    uint32_t totalCycles;
    float totalVolume;
    uint64_t totalRuntime;

    Brineomatic();
    void init();

    enum class Status {
      STARTUP,
      MANUAL,
      IDLE,
      RUNNING,
      STOPPING,
      FLUSHING,
      PICKLING,
      PICKLED,
      DEPICKLING
    };

    enum class Result {
      STARTUP,
      SUCCESS,
      SUCCESS_TIME,
      SUCCESS_VOLUME,
      SUCCESS_TANK_LEVEL,
      USER_STOP,
      ERR_FLUSH_VALVE_TIMEOUT,
      ERR_FILTER_PRESSURE_TIMEOUT,
      ERR_FILTER_PRESSURE_LOW,
      ERR_FILTER_PRESSURE_HIGH,
      ERR_MEMBRANE_PRESSURE_TIMEOUT,
      ERR_MEMBRANE_PRESSURE_LOW,
      ERR_MEMBRANE_PRESSURE_HIGH,
      ERR_FLOWRATE_TIMEOUT,
      ERR_FLOWRATE_LOW,
      ERR_SALINITY_TIMEOUT,
      ERR_SALINITY_HIGH,
      ERR_PRODUCTION_TIMEOUT,
      ERR_MOTOR_TEMPERATURE_HIGH
    };

    void setFilterPressure(float pressure);
    void setMembranePressure(float pressure);
    void setMembranePressureTarget(float pressure);
    void setProductFlowrate(float flowrate);
    void setBrineFlowrate(float flowrate);
    void setMotorTemperature(float temp);
    void setWaterTemperature(float temp);
    void setProductSalinity(float salinity);
    void setBrineSalinity(float salinity);
    void setTankLevel(float level);

    void idle();
    void manual();
    void start();
    void startDuration(uint64_t duration);
    void startVolume(float volume);
    void flush(uint64_t duration);
    void pickle(uint64_t duration);
    void depickle(uint64_t duration);
    void stop();

    bool initializeHardware();

    bool isBoostPumpOn();
    bool hasBoostPump();
    void enableBoostPump();
    void disableBoostPump();

    bool isHighPressurePumpOn();
    bool hasHighPressurePump();
    void enableHighPressurePump();
    void disableHighPressurePump();

    bool isDiverterValveOpen();
    bool hasDiverterValve();
    void openDiverterValve();
    void closeDiverterValve();

    bool isFlushValveOpen();
    bool hasFlushValve();
    void openFlushValve();
    void closeFlushValve();

    bool isCoolingFanOn();
    bool hasCoolingFan();
    void enableCoolingFan();
    void disableCoolingFan();
    void manageCoolingFan();

    bool hasHighPressureValve();
    void manageHighPressureValve();

    const char* getStatus();
    const char* resultToString(Result result);
    Result getRunResult();
    Result getFlushResult();
    Result getPickleResult();
    Result getDepickleResult();

    int64_t getNextFlushCountdown();
    int64_t getRuntimeElapsed();
    int64_t getFinishCountdown();
    int64_t getFlushElapsed();
    int64_t getFlushCountdown();
    int64_t getPickleElapsed();
    int64_t getPickleCountdown();
    int64_t getDepickleElapsed();
    int64_t getDepickleCountdown();

    float getFilterPressure();
    float getFilterPressureMinimum();
    float getMembranePressure();
    float getMembranePressureMinimum();
    float getProductFlowrate();
    float getProductFlowrateMinimum();
    float getBrineFlowrate();
    float getBrineFlowrateMinimum();
    float getTotalFlowrate();
    float getTotalFlowrateMinimum();
    uint32_t getTotalCycles();
    float getVolume();
    float getTotalVolume();
    uint64_t getTotalRuntime();
    float getMotorTemperature();
    float getWaterTemperature();
    float getProductSalinity();
    float getProductSalinityMaximum();
    float getBrineSalinity();
    float getTankLevel();
    float getTankCapacity();
    float getMotorTemperatureMaximum();

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
    uint64_t runtimeStart = 0;
    uint64_t runtimeElapsed = 0;
    uint64_t flushStart = 0;
    uint64_t flushDuration = 3ULL * 60 * 1000000; // 3 minute default, in microseconds
    uint64_t nextFlushTime = 0;
    uint64_t flushInterval = 3ULL * 24 * 60 * 60 * 1000000; // 3 day default, in microseconds
    uint64_t pickleStart = 0;
    uint64_t pickleDuration = 5ULL * 60 * 1000000; // 5 minute default, in microseconds
    uint64_t depickleStart = 0;
    uint64_t depickleDuration = 15ULL * 60 * 1000000; // 15 minute default, in microseconds

    bool boostPumpOnState;
    bool highPressurePumpOnState;
    bool diverterValveOpenState;
    bool flushValveOpenState;
    bool coolingFanOnState;

    float currentTankLevel;
    float currentWaterTemperature;
    float currentMotorTemperature;
    float currentProductFlowrate;
    float currentBrineFlowrate;
    float currentProductSalinity;
    float currentBrineSalinity;
    float currentFilterPressure;
    float currentMembranePressure;

    float membranePressureTarget;
    float membranePressurePIDOutput;
    QuickPID membranePressurePID;

    float lowPressureMinimum = 2.5;              // PSI
    float lowPressureMaximum = 60.0;             // PSI
    float highPressureMinimum = 600.0;           // PSI
    float defaultMembranePressureTarget = 775.0; // PSI
    float highPressureMaximum = 950.0;           // PSI
    float productFlowrateMinimum = 100.0;        // LPH
    float productFlowrateMaximum = 300.0;        // LPH
    float brineFlowrateMinimum = 0.0;            // LPH
    float brineFlowrateMaximum = 500.0;          // LPH
    float productSalinityMaximum = 500.0;               // PPM
    float motorTemperatureMaximum = 65.0;        // Celcius
    float tankLevelFull = 0.99;                  // 0 = empty, 1 = full
    float tankCapacity = 780;                    // Liters
    float coolingFanOnTemperature = 35.0;        // Celcius
    float coolingFanOffTemperature = 34.0;       // Celcius

    uint8_t membranePressureHighErrorCount = 0;
    uint8_t membranePressureLowErrorCount = 0;
    uint8_t filterPressureHighErrorCount = 0;
    uint8_t filterPressureLowErrorCount = 0;
    uint8_t productFlowrateLowErrorCount = 0;
    uint8_t brineFlowrateLowErrorCount = 0;
    uint8_t productSalinityHighErrorCount = 0;
    uint8_t motorTemperatureErrorCount = 0;

    uint64_t flushValveTimeout = 1ULL * 60 * 1000000;       // 1 minute default, in microseconds
    uint64_t filterPressureTimeout = 1ULL * 60 * 1000000;   // 1 minute default, in microseconds
    uint64_t membranePressureTimeout = 1ULL * 60 * 1000000; // 1 minute default, in microseconds
    uint64_t productFlowRateTimeout = 2ULL * 60 * 1000000;  // 2 minute default, in microseconds
    uint64_t brineFlowRateTimeout = 2ULL * 60 * 1000000;    // 2 minute default, in microseconds
    uint64_t productSalinityTimeout = 5ULL * 60 * 1000000;         // 5 minute default, in microseconds
    uint64_t productionTimeout = 12ULL * 60 * 60 * 1000000; // 12 hour default, in microseconds

    bool checkStopFlag();
    bool checkMembranePressureHigh();
    bool checkMembranePressureLow();
    bool checkFilterPressureHigh();
    bool checkFilterPressureLow();
    bool checkProductFlowrateLow();
    bool checkBrineFlowrateLow();
    bool checkProductSalinityHigh();
    bool waitForProductFlowrate();
    bool waitForProductSalinity();
    bool checkTankLevel();
    bool checkMotorTemperature();
};

extern Brineomatic wm;

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */