#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"

HIDSpaceMouse spaceMouse;
MagellanParser magellan;

void setup()
{
  spaceMouse.begin();
  magellan.begin(&Serial1);

  spaceMouse.set_translation(0.0f, 0.0f, 0.0f);
  spaceMouse.set_rotation(0.0f, 0.0f, 0.0f);
  spaceMouse.set_button(1, false);
  spaceMouse.set_button(HIDSpaceMouse::KnownButton::DUMMY, false);

  spaceMouse.submit();
}

void loop()
{
  magellan.update();

}
