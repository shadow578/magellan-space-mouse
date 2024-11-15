#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"

HIDSpaceMouse spaceMouse;
MagellanParser magellan;

uint32_t setup_end_millis;

void setup()
{
  spaceMouse.begin();
  magellan.begin(&Serial1);

  Serial.begin(115200);
  Serial.println("Hello, world!");

  spaceMouse.set_translation(0.0f, 0.0f, 0.0f);
  spaceMouse.set_rotation(0.0f, 0.0f, 0.0f);
  spaceMouse.set_button(1, false);
  spaceMouse.set_button(HIDSpaceMouse::KnownButton::DUMMY, false);

  spaceMouse.submit();

  setup_end_millis = millis();
}

void loop()
{
  if (magellan.update())
  {
    // something changed!
    Serial.print("x: ");
    Serial.print(magellan.get_x());
    Serial.print(", y: ");
    Serial.print(magellan.get_y());
    Serial.print(", z: ");
    Serial.print(magellan.get_z());
    Serial.print(", u: ");
    Serial.print(magellan.get_u());
    Serial.print(", v: ");
    Serial.print(magellan.get_v());
    Serial.print(", w: ");
    Serial.print(magellan.get_w());
    Serial.print(", buttons: ");
    Serial.print(magellan.get_buttons(), BIN);
    Serial.println();
  }

  if (!magellan.is_ready() && (millis() - setup_end_millis > 5000))
  {
    // not ready after 5 seconds, something is wrong!
    Serial.println("Magellan is not ready after >5 seconds!");
    setup_end_millis = millis();

    magellan.reset();
  }
}
