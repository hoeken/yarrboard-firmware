#include "validate.h"
#include <YarrboardDebug.h>
#include <cstring>

// --------------------------------------------
// Fail helper
// --------------------------------------------
bool fail(const char* msg, char* error, size_t err_size)
{
  if (error && err_size > 0) {
    YBP.println(error);
    snprintf(error, err_size, "%s", msg);
  }
  return false;
}

// --------------------------------------------
// Presence only
// --------------------------------------------
bool checkPresence(JsonVariantConst config, const char* key, char* error, size_t err_size)
{
  if (!config[key]) {
    YBP.println(error);
    snprintf(error, err_size, "Missing required field '%s'", key);
    return false;
  }
  return true;
}

// --------------------------------------------
// Type only
// --------------------------------------------
bool checkIsInteger(JsonVariantConst config, const char* key, char* error, size_t err_size)
{
  if (!config[key].is<int>()) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be an integer", key);
    return false;
  }
  return true;
}

bool checkIsNumber(JsonVariantConst config, const char* key,
  char* error, size_t err_size)
{
  if (!config[key].is<float>() && !config[key].is<int>()) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be numeric", key);
    return false;
  }
  return true;
}

bool checkIsBool(JsonVariantConst config, const char* key,
  char* error, size_t err_size)
{
  JsonVariantConst v = config[key];
  if (!v.is<bool>()) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be boolean", key);
    return false;
  }
  return true;
}

// --------------------------------------------
// Range checks only
// --------------------------------------------
bool checkIntGE(JsonVariantConst config, const char* key, int minv, char* error, size_t err_size)
{
  int v = config[key].as<int>();
  if (v < minv) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be >= %d", key, minv);
    return false;
  }
  return true;
}

bool checkNumGE(JsonVariantConst config, const char* key, float minv,
  char* error, size_t err_size)
{
  float v = config[key].as<float>();
  if (v < minv) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be >= %.3f", key, minv);
    return false;
  }
  return true;
}

bool checkNumGT(JsonVariantConst config, const char* key, float minv,
  char* error, size_t err_size)
{
  float v = config[key].as<float>();
  if (v <= minv) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be > %.3f", key, minv);
    return false;
  }
  return true;
}

bool checkNumRange(JsonVariantConst config,
  const char* key,
  float minv,
  float maxv,
  char* error,
  size_t err_size)
{
  float v = config[key].as<float>();

  if (v < minv) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be >= %.3f", key, minv);
    return false;
  }

  if (v > maxv) {
    YBP.println(error);
    snprintf(error, err_size, "Field '%s' must be <= %.3f", key, maxv);
    return false;
  }

  return true;
}