/*
  Yarrboard

  Author: Zach Hoeken <hoeken@gmail.com>
  Website: https://github.com/hoeken/yarrboard
  License: GPLv3
*/

#include "utility.h"

unsigned int framerate = 0;

double round2(double value)
{
  return (long)(value * 100 + 0.5) / 100.0;
}

double round3(double value)
{
  return (long)(value * 1000 + 0.5) / 1000.0;
}

double round4(double value)
{
  return (long)(value * 10000 + 0.5) / 10000.0;
}