#include <Arduino.h>
#include "spacemouse/HIDSpaceMouse.hpp"
#include "magellan/MagellanParser.hpp"
#include "magellan/CalibrationUtil.hpp"

#if !defined(GIT_VERSION_STRING)
#define GIT_VERSION_STRING "unknown"
#warning "GIT_VERSION_STRING not defined! check your build environment."
#endif

#define WAIT_FOR_SERIAL 0
#define CALIBRATION 0

// magellan axis calibration values, as reported by CalibrationUtil
static const magellan_internal::axis_calibration_t cal = {
    .x = {-3775, 2173},
    .y = {-3900, 4037},
    .z = {-1682, 3122},
    .u = {-2466, 3537},
    .v = {-3939, 2002},
    .w = {-3839, 1691},
};

// correction factors applied to the values received from the Magellan
// before being sent to the HIDSpaceMouse.
// 1.0f means no correction, -1.0f means invert the value.
// these values be in the range [-1.0f, 1.0f]
constexpr float x_correction = 1.0f;  // x position
constexpr float y_correction = 1.0f;  // y position
constexpr float z_correction = -1.0f; // z position
constexpr float u_correction = 1.0f;  // rotation around x axis
constexpr float v_correction = 1.0f;  // rotation around y axis
constexpr float w_correction = -1.0f; // rotation around z axis

// how long to wait for a double press of the "*" button
constexpr uint32_t STAR_BUTTON_DOUBLE_PRESS_TIMEOUT = 500; // ms

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
    HIDSpaceMouse::MENU   // Key "*" (double press)
};

HIDSpaceMouse spaceMouse(&Serial); // debug output to USB serial port
MagellanParser magellan(&cal, &Serial);  // debug output to USB serial port

#if CALIBRATION == 1
MagellanCalibrationUtil calibration(&Serial, &magellan);
#endif

void handle_buttons(const bool from_event)
{
  constexpr uint8_t STAR_BUTTON_ID = 8;

  // update button states according to mapping
  // only when called from a button event (a button actually changed)
  if (from_event)
  {
    for (uint8_t i = 0; i < magellan_internal::BUTTON_COUNT; i++)
    {
      // skip the "*" button, it's handled separately below
      if (i == STAR_BUTTON_ID)
      {
        continue;
      }

      spaceMouse.set_button(button_mappings[i], magellan.get_button(i));
    }
  }

  // check if the "*" button is pressed, then released, and then pressed again within 500ms
  // runs always, as it needs to handle the timing of the button presses
  // thus, the button state needs to be handled manually
  enum StarButtonState
  {
    Idle,         // wait for first press
    FirstDown,    // button was down the first time
    FirstRelease, // button was released after the first press. millis are recorded in star_first_release_millis
    SecondDown    // button was pressed again. if not within 500ms, go back to Idle
  };

  static StarButtonState star_button_state = Idle;
  static uint32_t star_first_release_millis = 0;

  const uint32_t now = millis();
  const StarButtonState old_state = star_button_state;

  switch (star_button_state)
  {
  case Idle:
  {
    if (magellan.get_button(STAR_BUTTON_ID))
    {
      star_button_state = FirstDown;
    }
    break;
  }
  case FirstDown:
  {
    if (!magellan.get_button(STAR_BUTTON_ID))
    {
      star_button_state = FirstRelease;
      star_first_release_millis = now;
    }
    break;
  }
  case FirstRelease:
  {
    if (magellan.get_button(STAR_BUTTON_ID))
    {
      // button was pressed a second time
      spaceMouse.set_button(button_mappings[STAR_BUTTON_ID], true);
      star_button_state = SecondDown;
    }

    if ((now - star_first_release_millis) > STAR_BUTTON_DOUBLE_PRESS_TIMEOUT)
    {
      star_button_state = Idle;
    }
    break;
  }
  case SecondDown:
  {
    if (!magellan.get_button(STAR_BUTTON_ID))
    {
      // button was released
      spaceMouse.set_button(button_mappings[STAR_BUTTON_ID], false);
      star_button_state = Idle;
    }
    break;
  }
  default:
  {
    star_button_state = Idle;
    break;
  }
  }

  if (star_button_state != old_state)
  {
    Serial.print(F("[Main] STAR button state changed: "));
    Serial.print(old_state);
    Serial.print(F(" -> "));
    Serial.println(star_button_state);
  }
}

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

  // FIXME magellan hangs if we don't wait a bit here
  delay(5000);

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
    static bool was_ready = false;
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

      // update button states
      handle_buttons(true);
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

  handle_buttons(false);

  spaceMouse.update();

  static bool old_led = false;
  if (spaceMouse.get_led() != old_led)
  {
    Serial.print("[Main] LED state changed: ");
    Serial.println(spaceMouse.get_led() ? "on" : "off");
    old_led = spaceMouse.get_led();
  }
}
