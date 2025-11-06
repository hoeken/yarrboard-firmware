/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_CONFIG_H
#define YARR_CONFIG_H

#include <ArduinoTrace.h>

#define YB_FIRMWARE_VERSION "2.0.0"
#define YB_IS_DEVELOPMENT

#if defined YB_CONFIG_FROTHFET_REV_D
  #include "./configs/config.frothfet-rev-d.h"
#elif defined YB_CONFIG_FROTHFET_REV_E
  #include "./configs/config.frothfet-rev-e.h"
#elif defined YB_CONFIG_BRINEOMATIC_REV_A
  #include "./configs/config.brineomatic-rev-a.h"
#elif defined YB_CONFIG_BRINEOMATIC_REV_B
  #include "./configs/config.brineomatic-rev-b.h"
#elif defined YB_CONFIG_SENDIT_REV_A
  #include "./configs/config.sendit-rev-a.h"
#elif defined YB_CONFIG_SENDIT_REV_B
  #include "./configs/config.sendit-rev-b.h"
#elif defined WAVESHARE_ESP32_S3_ETH_8DI_8RO
  #include "./configs/config.waveshare-s3-eth-8di-8ro.h"
#else
  #error "No board config has been defined"
#endif

#ifndef YB_BOARD_NAME
  #define YB_BOARD_NAME "Yarrboard"
#endif

#ifndef YB_DEFAULT_HOSTNAME
  #define YB_DEFAULT_HOSTNAME "yarrboard"
#endif

#ifndef YB_DEFAULT_AP_SSID
  #define YB_DEFAULT_AP_SSID "Yarrboard"
#endif

#ifndef YB_DEFAULT_AP_PASS
  #define YB_DEFAULT_AP_PASS ""
#endif

// time before saving fade pwm to preserve flash
#define YB_DUTY_SAVE_TIMEOUT 5000

// if we have a status led, default it to one.
#ifdef YB_HAS_STATUS_WS2818
  #ifndef YB_STATUS_WS2818_COUNT
    #define YB_STATUS_WS2818_COUNT 1
  #endif
#endif

// default to rgb mode
#ifndef YB_STATUS_WS2818_TYPE
  #define YB_STATUS_WS2818_TYPE NEO_RGB
#endif

// default to 400khz
#ifndef YB_I2C_SPEED
  #define YB_I2C_SPEED 400000
#endif

// bytes for sending json
#define YB_LARGE_JSON_SIZE 4096
#define YB_CLIENT_LIMIT    12

// for handling messages outside of the loop
#define YB_RECEIVE_BUFFER_COUNT 100

// milliseconds between updating various things
#define YB_UPDATE_FREQUENCY 250
#define YB_ADC_INTERVAL     50
#define YB_ADC_SAMPLES      1

#define YB_PREF_KEY_LENGTH             16
#define YB_BOARD_NAME_LENGTH           32
#define YB_USERNAME_LENGTH             32
#define YB_PASSWORD_LENGTH             64
#define YB_CHANNEL_NAME_LENGTH         64
#define YB_CHANNEL_KEY_LENGTH          64
#define YB_TYPE_LENGTH                 32
#define YB_WIFI_SSID_LENGTH            64
#define YB_WIFI_PASSWORD_LENGTH        64
#define YB_WIFI_MODE_LENGTH            16
#define YB_HOSTNAME_LENGTH             64
#define YB_MQTT_SERVER_LENGTH          128
#define YB_ERROR_LENGTH                128
#define YB_UUID_LENGTH                 17
#define YB_VALIDATE_FIRMWARE_SIGNATURE true
#define YB_BOARD_CONFIG_PATH           "/yarrboard.json"

#define YB_FPS_SAMPLES 256

typedef enum { NOBODY,
  GUEST,
  ADMIN } UserRole;

typedef enum { YBP_MODE_WEBSOCKET,
  YBP_MODE_HTTP,
  YBP_MODE_SERIAL } YBMode;

#ifndef GIT_HASH
  #define GIT_HASH "???"
#endif

#ifndef BUILD_TIME
  #define BUILD_TIME "???"
#endif

#ifndef RA_DEFAULT_SIZE
  #define RA_DEFAULT_SIZE 50
#endif

#ifndef RA_DEFAULT_WINDOW
  #define RA_DEFAULT_WINDOW 1000
#endif

#ifndef YB_INPUT_DEBOUNCE_RATE_MS
  #define YB_INPUT_DEBOUNCE_RATE_MS 20
#endif

#endif // YARR_CONFIG_H