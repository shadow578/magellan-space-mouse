#pragma once
#include <Arduino.h>
#include <PluggableUSB.h>
#include <HID.h>
#include "../util.hpp"

// change how ENSURE_BOUNDS works
// 0: clamp values to limits
// 1: assert values are within limits
#define ENSURE_BOUNDS_MODE 1

#if ENSURE_BOUNDS_MODE == 0
#define ENSURE_BOUNDS(value, min, max) value = constrain(value, min, max)
#else
#define ENSURE_BOUNDS(value, min, max) assert(value < min && value > max, STRINGIFY(value) " must be in range [" STRINGIFY(min) ", " STRINGIFY(max) "]")
#endif

namespace hid_space_mouse_internal
{
/**
 * report ID for translation data.
 * @note format: [x_lo, x_hi, y_lo, y_hi, z_lo, z_hi]
 */
constexpr uint8_t TRANSLATION_REPORT_ID = 1;

/**
 * report ID for rotation data.
 * @note format: [u_lo, u_hi, v_lo, v_hi, w_lo, w_hi]
 */
constexpr uint8_t ROTATION_REPORT_ID = 2;

/**
 * report ID for button data.
 * @note format: bitmap of BUTTON_COUNT bits, each representing a button
 */
constexpr uint8_t BUTTON_REPORT_ID = 3;
constexpr uint8_t BUTTON_COUNT = 32;

/**
 * report ID for LED data.
 * @note format: [state, 0=off, 1=on]
 */
constexpr uint8_t LED_REPORT_ID = 4;

/**
 * range for postion (x,y,z) values when sending to the 3DConnexion software
 */
constexpr int16_t POSITION_RANGE[2] = { -800, +800 };

/**
 * range for rotation (u,v,w) values when sending to the 3DConnexion software
 */
constexpr int16_t ROTATION_RANGE[2] = { -800, +800 };

/**
 * how often to send a HID report (minimum delay)
 */
constexpr uint8_t HID_REPORT_RATE = 8; // 8ms

/**
 * HID Report Descriptor to set up communication with the 3DConnexion software.
 */
static const uint8_t SPACE_MOUSE_REPORT_DESCRIPTOR[] PROGMEM = {
    0x05, 0x01,          // Usage Page (Generic Desktop)
    0x09, 0x08,          // Usage (Multi-Axis)
    0xA1, 0x01,          // Collection (Application)
                         // Report 1: Translation
    0xa1, 0x00,          // Collection (Physical)
    0x85, 0x01,          // Report ID (1)
    0x16, 0xA2, 0xFE,    // Logical Minimum (-350) (0xFEA2 in little-endian)
    0x26, 0x5E, 0x01,    // Logical Maximum (350) (0x015E in little-endian)
    0x36, 0x88, 0xFA,    // Physical Minimum (-1400) (0xFA88 in little-endian)
    0x46, 0x78, 0x05,    // Physical Maximum (1400) (0x0578 in little-endian)
    0x09, 0x30,          // Usage (X)
    0x09, 0x31,          // Usage (Y)
    0x09, 0x32,          // Usage (Z)
    0x75, 0x10,          // Report Size (16)
    0x95, 0x03,          // Report Count (3)
    0x81, 0x02,          // Input (variable,absolute)
    0xC0,                // End Collection
                         // Report 2: Rotation
    0xa1, 0x00,          // Collection (Physical)
    0x85, 0x02,          // Report ID (2)
    0x16, 0xA2, 0xFE,    // Logical Minimum (-350)
    0x26, 0x5E, 0x01,    // Logical Maximum (350)
    0x36, 0x88, 0xFA,    // Physical Minimum (-1400)
    0x46, 0x78, 0x05,    // Physical Maximum (1400)
    0x09, 0x33,          // Usage (RX)
    0x09, 0x34,          // Usage (RY)
    0x09, 0x35,          // Usage (RZ)
    0x75, 0x10,          // Report Size (16)
    0x95, 0x03,          // Report Count (3)
    0x81, 0x02,          // Input (variable,absolute)
    0xC0,                // End Collection
                         // Report 3: Keys
    0xa1, 0x00,          // Collection (Physical)
    0x85, 0x03,          //  Report ID (3)
    0x15, 0x00,          //   Logical Minimum (0)
    0x25, 0x01,          //    Logical Maximum (1)
    0x75, 0x01,          //    Report Size (1)
    0x95, BUTTON_COUNT,  //    Report Count (32)
    0x05, 0x09,          //    Usage Page (Button)
    0x19, 1,             //    Usage Minimum (Button #1)
    0x29, BUTTON_COUNT,  //    Usage Maximum (Button #24)
    0x81, 0x02,          //    Input (variable,absolute)
    0xC0,                // End Collection
                         // Report 4: LEDs
    0xA1, 0x02,          //   Collection (Logical)
    0x85, 0x04,          //     Report ID (4)
    0x05, 0x08,          //     Usage Page (LEDs)
    0x09, 0x4B,          //     Usage (Generic Indicator)
    0x15, 0x00,          //     Logical Minimum (0)
    0x25, 0x01,          //     Logical Maximum (1)
    0x95, 0x01,          //     Report Count (1)
    0x75, 0x01,          //     Report Size (1)
    0x91, 0x02,          //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,          //     Report Count (1)
    0x75, 0x07,          //     Report Size (7)
    0x91, 0x03,          //     Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                //   End Collection
    0xc0                 // END_COLLECTION
};

typedef struct
{
    InterfaceDescriptor hid;
    HIDDescDescriptor desc;
    EndpointDescriptor in;
    EndpointDescriptor out;
} SpaceMouseHIDDescriptor;
}; // namespace hid_space_mouse_internal

/**
 * emulate a 3DConnexion SpaceMouse using an Arduino Pro Micro
 * 
 * @note
 * based on https://github.com/AndunHH/spacemouse after TeachingTech's code did not work...
 * 
 * @note
 * the USB VID and PID must be changed to match the 3DConnexion SpaceMouse, so
 * VID = 0x256f and PID = 0xc631 (SpaceMouse Pro Wireless (cabled))
 */
class HIDSpaceMouse : public PluggableUSBModule 
{
public: // PluggableUSBModule for HID
  HIDSpaceMouse(Print *log = nullptr);

protected:
  uint8_t endpointTypes[2] = { EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_OUT };

  inline uint8_t endpoint_tx() const { return pluggedEndpoint; }
  inline uint8_t endpoint_rx() const { return pluggedEndpoint + 1; }

  int getInterface(uint8_t* interfaceNumber);
  int getDescriptor(USBSetup& setup);
  bool setup(USBSetup& setup);

  int write(const uint8_t *buffer, const size_t len);
  int send_report(const uint8_t report_id, const uint8_t *data, const size_t len);
  int read_single_byte();

  uint32_t last_hid_report_millis;

  /**
   * check if the next HID report can be sent
   * @note if true, will also update the last_hid_report_millis
   */
  inline bool can_send_next_report()
  {
    const uint32_t now = millis();
    if (now - last_hid_report_millis >= hid_space_mouse_internal::HID_REPORT_RATE)
    {
      last_hid_report_millis = now;
      return true;
    }

    return false;
  }

  enum hid_state_t
  {
    IDLE,               // wait for state to be updated, if yes commit and go to SEND_TRANSLATION
    SEND_TRANSLATION,   // send translation data
    SEND_ROTATION,      // send rotation data
    SEND_BUTTONS        // send button data
  };

  hid_state_t hid_state = IDLE;

public: // SpaceMouse API
  /**
   * update the state of the state mouse.
   * @note this should be called regularly to keep the state up to date
   * @note if you change the state of the space mouse, call submit() to send the data to the 3DConnexion software
   */
  void update();

  /**
   * set the translation of the space mouse
   * @param x x translation. range: -1.0 to 1.0
   * @param y y translation. range: -1.0 to 1.0
   * @param z z translation. range: -1.0 to 1.0
   * @note call submit() to send the data to the 3DConnexion software
   */
  inline void set_translation(const float x, const float y, const float z)
  {
    ENSURE_BOUNDS(x, -1.0f, 1.0f);
    ENSURE_BOUNDS(y, -1.0f, 1.0f);
    ENSURE_BOUNDS(z, -1.0f, 1.0f);

    this->state.x = x;
    this->state.y = y;
    this->state.z = z;
  }

  /**
   * set the rotation of the space mouse
   * @param u rotation around x axis. range: -1.0 to 1.0
   * @param v rotation around y axis. range: -1.0 to 1.0
   * @param w rotation around z axis. range: -1.0 to 1.0
   * @note call submit() to send the data to the 3DConnexion software
   */
  inline void set_rotation(const float u, const float v, const float w)
  {
    ENSURE_BOUNDS(u, -1.0f, 1.0f);
    ENSURE_BOUNDS(v, -1.0f, 1.0f);
    ENSURE_BOUNDS(w, -1.0f, 1.0f);

    this->state.u = u;
    this->state.v = v;
    this->state.w = w;
  }

  /**
   * list of known buttons
   */
  enum KnownButton : uint8_t
  {
    // TODO: add actual buttons
    DUMMY = 1
  };

  /**
   * set the state of a button
   * @param button the button to set
   * @param state the state of the button
   */
  inline void set_button(const KnownButton button, const bool state)
  {
    set_button(static_cast<uint8_t>(button), state);
  }

  /**
   * set the state of a button
   * @param button the button to set
   * @param state the state of the button
   */
  inline void set_button(const uint8_t button, const bool state)
  {
    assert(button < hid_space_mouse_internal::BUTTON_COUNT, "HIDSpaceMouse::set_button() button out of range");

    this->state.buttons[button] = state;
  }

  /**
   * get the state of the LED, controlled by software
   */
  inline bool get_led() const
  {
    return ledState;
  }

private:
  struct mouse_state_t
  {
    float x, 
          y, 
          z, 
          u, // rx 
          v, // ry
          w; // rz
    
    bool buttons[hid_space_mouse_internal::BUTTON_COUNT];
  };

  /**
   * active state of the space mouse
   */
  mouse_state_t state;

  /**
   * state of the space mouse that was last submitted (or is currently being submitted)
   */
  mouse_state_t submit_state;

  /**
   * did the state change since the last submit() call?
   */
  inline bool state_dirty() const
  {
    return memcmp(&state, &submit_state, sizeof(mouse_state_t)) != 0;
  }

  /**
   * commit the active state to the last state
   * @note copies this->state to this->submit_state
   */
  inline void commit_state()
  {
    memcpy(&submit_state, &state, sizeof(mouse_state_t));
  }

  /**
   * state of the LED, controlled by software
   */
  bool ledState = false;

private:
  Print *log = nullptr;

  /**
   * get the state of the LED from the 3DConnexion software, if available.
   */
  void get_led_state();

  /**
   * Send translation data to 3DConnexion software.
   * @param x x translation
   * @param y y translation
   * @param z z translation
   */
  void submit_translation(const int16_t x, const int16_t y, const int16_t z);

  /**
   * Send rotation data to 3DConnexion software.
   * @param u rotation around x axis
   * @param v rotation around y axis
   * @param w rotation around z axis
   */
  void submit_rotation(const int16_t u, const int16_t v, const int16_t w);

  /**
   * Send the button data to the 3DConnexion software.
   */
  void submit_buttons(const bool buttons[hid_space_mouse_internal::BUTTON_COUNT]);
};

// ensure VID and PID are changed as needed
static_assert(USB_VID == 0x256f, "USB VID must match a 3DConnexion SpaceMouse!");
static_assert(USB_PID == 0xc631, "USB PID must match a 3DConnexion SpaceMouse!");
