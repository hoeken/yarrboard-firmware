#ifndef YB_VALIDATE_H
#define YB_VALIDATE_H

#include <ArduinoJson.h>
#include <stddef.h>

// ===========================================================
// Validation Helper Function Prototypes
// Each function performs ONE single responsibility.
// ===========================================================

// Writes msg into (error) safely and returns false
bool fail(const char* msg, char* error, size_t err_size);

// -------------------- Presence -----------------------------

// Returns false if the key is missing
bool checkPresence(JsonVariantConst config, const char* key,
  char* error, size_t err_size);

// -------------------- Type Checking ------------------------

// Returns false if config[key] is not an integer
bool checkIsInteger(JsonVariantConst config, const char* key,
  char* error, size_t err_size);

// Returns false if config[key] is not numeric (int or float)
bool checkIsNumber(JsonVariantConst config, const char* key,
  char* error, size_t err_size);

// Returns false if config[key] is not boolean
bool checkIsBool(JsonVariantConst config, const char* key,
  char* error, size_t err_size);

// -------------------- Inclusion ----------------------------

// Returns false if config[key] is not one of the allowed strings
template <size_t N>
bool checkInclusion(JsonVariantConst config, const char* key, const char* const (&allowed)[N], char* error, size_t err_size)
{
  const char* v = config[key].as<const char*>();
  if (!v) {
    snprintf(error, err_size, "Field '%s' must be a string", key);
    return false;
  }

  for (size_t i = 0; i < N; i++) {
    if (strcmp(v, allowed[i]) == 0)
      return true;
  }

  snprintf(error, err_size, "Invalid value '%s' for field '%s'", v, key);
  return false;
}

// -------------------- Range Checking -----------------------

// Returns false if integer value < minv
bool checkIntGE(JsonVariantConst config, const char* key, int minv,
  char* error, size_t err_size);

// Returns false if numeric value < minv
bool checkNumGE(JsonVariantConst config, const char* key, float minv,
  char* error, size_t err_size);

// Returns false if numeric value <= minv
bool checkNumGT(JsonVariantConst config, const char* key, float minv,
  char* error, size_t err_size);

// Returns false if minv ≤ v ≤ maxv
bool checkNumRange(JsonVariantConst config,
  const char* key,
  float minv,
  float maxv,
  char* error,
  size_t err_size);

#endif // YB_VALIDATE_H