/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_CONFIG_H
#define YARR_CONFIG_H

#define MAX_JSON_LENGTH 2000

// enable one of these
//#define CONFIG_8CH_MOSFET_REVA
//#define CONFIG_8CH_MOSFET_REVB

#if defined CONFIG_8CH_MOSFET_REVA
  #include "./configs/config.8ch-mosfet-reva.h"
#elif defined CONFIG_8CH_MOSFET_REVB
  #include "./configs/config.8ch-mosfet-revb.h"
#endif

#ifndef CHANNEL_PWM_FREQUENCY
  #define CHANNEL_PWM_FREQUENCY 1000
#endif

#ifndef CHANNEL_PWM_RESOLUTION
  #define CHANNEL_PWM_RESOLUTION 10
#endif
#endif // YARR_CONFIG_H