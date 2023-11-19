/*
  Yarrboard
  
  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#ifndef YARR_PREFS_H
#define YARR_PREFS_H

#include <Preferences.h>

//storage for more permanent stuff.
extern Preferences preferences;

void prefs_setup();

#endif /* !YARR_PREFS_H */