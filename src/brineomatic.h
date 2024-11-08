/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

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
    void setDiversion(bool value);
    void setFlowrate(float flowrate);
    void setTemperature(float temp);
    void setSalinity(float salinity);

    void disableHighPressurePump();
    void disableBoostPump();
    void enableHighPressurePump();
    void enableBoostPump();
    bool hasBoostPump();

    float getFilterPressure();
    float getFilterPressureMinimum();
    float getMembranePressure();
    float getMembranePressureMinimum();
    float getFlowrate();
    float getFlowrateMinimum();
    float getTemperature();
    float getSalinity();
    float getSalinityMaximum();

    void openFlushValve();
    void closeFlushValve();

  private:
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