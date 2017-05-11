#pragma once

#include "global.h"

#define max(x,y) ( \
    { __auto_type __x = (x); __auto_type __y = (y); \
      __x > __y ? __x : __y; })
#define min(x,y) ( \
    { __auto_type __x = (x); __auto_type __y = (y); \
      __x < __y ? __x : __y; })

inline u8 isEmptyString(const char* buffer)
{
  return !buffer[0];
}
inline double ceiling(double value)
{
  return (value > 0 && (value - (int)value) > 0) ?
    (int)(value + 1) : (int)value;
}

size_t dtoa(double value, char* buffer, u8 precision);
