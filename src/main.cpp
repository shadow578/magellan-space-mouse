#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"

HIDSpaceMouse spaceMouse;

void setup()
{
  spaceMouse.begin();
  spaceMouse.submit();
}

void loop()
{

}
