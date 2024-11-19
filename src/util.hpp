#pragma once
#include <Arduino.h>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

/**
 * assert that a condition is true, otherwise print and abort()
 */
#define assert(x, msg)                                \
  if (x)                                              \
  {                                                   \
    if (Serial)                                       \
    {                                                 \
      Serial.println(F("ASSERTION FAILED: " msg "")); \
      Serial.flush();                                 \
    }                                                 \
    abort();                                          \
  }

