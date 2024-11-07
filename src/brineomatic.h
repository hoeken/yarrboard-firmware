/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_BRINEOMATIC_H
#define YARR_BRINEOMATIC_H

extern float temperatureReading;
extern float flowrateReading;
extern float tdsReading;
extern float lowPressureReading;
extern float highPressureReading;

void brineomatic_setup();
void brineomatic_loop();

void brineomatic_state_machine(void* pvParameters);

void measure_flowmeter();
void measure_temperature();
void measure_tds();
void measure_lp_sensor();
void measure_hp_sensor();

enum class WatermakerStatus {
  STARTUP,
  IDLE,
  RUNNING,
  FLUSHING,
  PICKLING,
  PICKLED
};

template <class X, class M, class N, class O, class Q>
X map_generic(X x, M in_min, N in_max, O out_min, Q out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#endif /* !YARR_BRINEOMATIC_H */