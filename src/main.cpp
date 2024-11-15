#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"

// correction factors applied to the values received from the Magellan
// before being sent to the HIDSpaceMouse.
// 1.0f means no correction, -1.0f means invert the value.
// these values be in the range [-1.0f, 1.0f]
constexpr float x_correction = 1.0f; // x position 
constexpr float y_correction = 1.0f; // y position 
constexpr float z_correction = 1.0f; // z position 
constexpr float u_correction = 1.0f; // rotation around x axis
constexpr float v_correction = 1.0f; // rotation around y axis
constexpr float w_correction = 1.0f; // rotation around z axis

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
    // something changed, submit new values
    spaceMouse.set_translation(
      magellan.get_x() * x_correction, 
      magellan.get_y() * y_correction, 
      magellan.get_z() * z_correction
    );
    spaceMouse.set_rotation(
      magellan.get_u() * u_correction, 
      magellan.get_v() * v_correction, 
      magellan.get_w() * w_correction
    );

    // TODO: handle buttons, need mapping for them tho...

    spaceMouse.submit();

    // debug print to console
    Serial.print("[SM submit]: x: ");
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
