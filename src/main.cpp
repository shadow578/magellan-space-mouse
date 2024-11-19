#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"

#if !defined(GIT_VERSION_STRING)
  #define GIT_VERSION_STRING "unknown"
  #warning "GIT_VERSION_STRING not defined! check your build environment."
#endif

#define WAIT_FOR_SERIAL 1

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
MagellanParser magellan(&Serial); // debug output to USB serial port

bool was_ready = false;
bool old_led = false;

void setup()
{
  magellan.begin(&Serial1);

  // note: Serial is the USB serial port, Serial1 is the hardware serial port
  Serial.begin(115200);

  #if WAIT_FOR_SERIAL
    // wait for host to open serial port
    while(!Serial) ;
  #endif

  Serial.println("[Main] setup()");
  Serial.println("[Main] version: " GIT_VERSION_STRING);
}

void loop()
{
  if (magellan.update())
  {
    const bool is_ready = magellan.ready();
    if (is_ready && !was_ready)
    {
      // just became ready
      Serial.println("[Main] magellan is now ready");
      magellan.beep();
    }
    else if (!is_ready && was_ready)
    {
      // no longer ready ?!
      Serial.println("[Main] magellan is no longer ready");
    }
    was_ready = is_ready;

    if (is_ready)
    {
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
    }

    // debug print to console even when not ready
    Serial.print("[Main]: x=");
    Serial.print(magellan.get_x());
    Serial.print(", y=");
    Serial.print(magellan.get_y());
    Serial.print(", z=");
    Serial.print(magellan.get_z());
    Serial.print(", u=");
    Serial.print(magellan.get_u());
    Serial.print(", v=");
    Serial.print(magellan.get_v());
    Serial.print(", w=");
    Serial.print(magellan.get_w());
    Serial.print(", buttons=");
    Serial.print(magellan.get_buttons(), BIN);
    Serial.println();
  }

  spaceMouse.update();

  if (spaceMouse.get_led() != old_led)
  {
    Serial.print("[Main] LED state changed: ");
    Serial.println(spaceMouse.get_led() ? "on" : "off");
    old_led = spaceMouse.get_led();
  }
}
