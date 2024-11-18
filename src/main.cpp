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

void setup()
{
  spaceMouse.begin();
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
    if (magellan.ready())
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

      spaceMouse.submit();
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
}
