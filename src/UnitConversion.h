#ifndef BRINEOMATIC_CONVERSIONS_H
#define BRINEOMATIC_CONVERSIONS_H

#include <string.h>

// --- TEMPERATURE ---
static inline double convertTemperature(double value, const char* start_units, const char* end_units)
{
  if (strcmp(start_units, end_units) == 0)
    return value;

  // Celsius to Fahrenheit
  if (strcmp(start_units, "celsius") == 0 && strcmp(end_units, "fahrenheit") == 0) {
    return (value * 9.0 / 5.0) + 32.0;
  }
  // Fahrenheit to Celsius
  if (strcmp(start_units, "fahrenheit") == 0 && strcmp(end_units, "celsius") == 0) {
    return (value - 32.0) * 5.0 / 9.0;
  }

  return value;
}

// --- PRESSURE ---
static inline double convertPressure(double value, const char* start_units, const char* end_units)
{
  if (strcmp(start_units, end_units) == 0)
    return value;

  // From PSI
  if (strcmp(start_units, "psi") == 0) {
    if (strcmp(end_units, "bar") == 0)
      return value * 0.0689476;
    if (strcmp(end_units, "kilopascal") == 0)
      return value * 6.89476;
  }
  // From Bar
  if (strcmp(start_units, "bar") == 0) {
    if (strcmp(end_units, "psi") == 0)
      return value * 14.5038;
    if (strcmp(end_units, "kilopascal") == 0)
      return value * 100.0;
  }
  // From Kilopascal
  if (strcmp(start_units, "kilopascal") == 0) {
    if (strcmp(end_units, "psi") == 0)
      return value * 0.145038;
    if (strcmp(end_units, "bar") == 0)
      return value * 0.01;
  }

  return value;
}

// --- VOLUME ---
static inline double convertVolume(double value, const char* start_units, const char* end_units)
{
  if (strcmp(start_units, end_units) == 0)
    return value;

  // Gallons to Liters
  if (strcmp(start_units, "gallons") == 0 && strcmp(end_units, "liters") == 0) {
    return value * 3.78541;
  }
  // Liters to Gallons
  if (strcmp(start_units, "liters") == 0 && strcmp(end_units, "gallons") == 0) {
    return value * 0.264172;
  }

  return value;
}

// --- FLOW RATE ---
static inline double convertFlowrate(double value, const char* start_units, const char* end_units)
{
  if (strcmp(start_units, end_units) == 0)
    return value;

  // GPH to LPH
  if (strcmp(start_units, "gph") == 0 && strcmp(end_units, "lph") == 0) {
    return value * 3.78541;
  }
  // LPH to GPH
  if (strcmp(start_units, "lph") == 0 && strcmp(end_units, "gph") == 0) {
    return value * 0.264172;
  }

  return value;
}

#endif // BRINEOMATIC_CONVERSIONS_H