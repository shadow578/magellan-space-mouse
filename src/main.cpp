#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"
#include "magellan/CalibrationUtil.hpp"

#if !defined(GIT_VERSION_STRING)
#define GIT_VERSION_STRING "unknown"
#warning "GIT_VERSION_STRING not defined! check your build environment."
#endif

#define WAIT_FOR_SERIAL 1
#define CALIBRATION 0

// magellan axis calibration values, as reported by CalibrationUtil
static const magellan_internal::axis_calibration_t cal = {
  .x={-4092, 4029}, 
  .y={-3995, 3658}, 
  .z={-3706, 4017}, 
  .u={-4077, 3881}, 
  .v={-4063, 3812}, 
  .w={-3828, 4089}
};

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

// mapping of Magellan buttons to HIDSpaceMouse buttons
static const HIDSpaceMouse::KnownButton button_mappings[magellan_internal::BUTTON_COUNT] = {
    HIDSpaceMouse::ONE,   // Key "1"
    HIDSpaceMouse::TWO,   // Key "2"
    HIDSpaceMouse::THREE, // Key "3"
    HIDSpaceMouse::FOUR,  // Key "4"
    HIDSpaceMouse::FIT,   // Key "5"
    HIDSpaceMouse::TOP,   // Key "6"
    HIDSpaceMouse::RIGHT, // Key "7"
    HIDSpaceMouse::FRONT, // Key "8"
    HIDSpaceMouse::MENU   // Key "*"
};

HIDSpaceMouse spaceMouse(&Serial); // debug output to USB serial port
MagellanParser magellan(&cal, &Serial);  // debug output to USB serial port

#if CALIBRATION == 1
MagellanCalibrationUtil calibration(&Serial, &magellan);
#endif

bool was_ready = false;
bool old_led = false;

void setup()
{
  magellan.begin(&Serial1);

  // note: Serial is the USB serial port, Serial1 is the hardware serial port
  Serial.begin(115200);

#if WAIT_FOR_SERIAL
  // wait for host to open serial port
  while (!Serial)
    ;
#endif

  Serial.println("[Main] setup()");
  Serial.println("[Main] version: " GIT_VERSION_STRING);
}

void loop()
{
  const bool did_update = magellan.update();

#if CALIBRATION == 1
  calibration.update();
  return; // don't use any of the data when in calibration mode
#endif

  if (did_update)
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
      // update rotation and translation values
      spaceMouse.set_translation(
          magellan.get_x() * x_correction,
          magellan.get_y() * y_correction,
          magellan.get_z() * z_correction);
      spaceMouse.set_rotation(
          magellan.get_u() * u_correction,
          magellan.get_v() * v_correction,
          magellan.get_w() * w_correction);

      // update button states according to mapping
      for (uint8_t i = 0; i < magellan_internal::BUTTON_COUNT; i++)
      {
        spaceMouse.set_button(button_mappings[i], magellan.get_button(i));
      }
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
