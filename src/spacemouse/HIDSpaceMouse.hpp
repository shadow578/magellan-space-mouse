#pragma once
#include <Arduino.h>
#include <HID.h>
#include <assert.h>

namespace hid_space_mouse_internal
{
  /**
 * HID Report Descriptor to set up communication with the 3DConnexion software.
 */
static const uint8_t hidReportDescriptor[] PROGMEM = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x08,        // Usage (Multi-axis Controller)
  0xA1, 0x01,        // Collection (Application)
  0xA1, 0x00,        //   Collection (Physical)
  0x85, 0x01,        //     Report ID (1)
  0x16, 0x00, 0x80,  //     Logical Minimum (-32768)
  0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
  0x36, 0x00, 0x80,  //     Physical Minimum (-32768)
  0x46, 0xFF, 0x7F,  //     Physical Maximum (32767)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x09, 0x32,        //     Usage (Z)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x03,        //     Report Count (3)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0xA1, 0x00,        //   Collection (Physical)
  0x85, 0x02,        //     Report ID (2)
  0x16, 0x00, 0x80,  //     Logical Minimum (-32768)
  0x26, 0xFF, 0x7F,  //     Logical Maximum (32767)
  0x36, 0x00, 0x80,  //     Physical Minimum (-32768)
  0x46, 0xFF, 0x7F,  //     Physical Maximum (32767)
  0x09, 0x33,        //     Usage (Rx)
  0x09, 0x34,        //     Usage (Ry)
  0x09, 0x35,        //     Usage (Rz)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x03,        //     Report Count (3)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0xA1, 0x00,        //   Collection (Physical)
  0x85, 0x03,        //     Report ID (3)
  0x15, 0x00,        //     Logical Minimum (0)
  0x25, 0x01,        //     Logical Maximum (1)
  0x75, 0x01,        //     Report Size (1)
  0x95, 0x32,        //     Report Count (50)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (0x01)
  0x29, 0x32,        //     Usage Maximum (0x32)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0xC0               // End Collection
};

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
constexpr uint8_t BUTTON_COUNT = 50;

/**
 * range for postion (x,y,z) values when sending to the 3DConnexion software
 */
constexpr int16_t POSITION_RANGE[2] = { -800, +800 };

/**
 * range for rotation (u,v,w) values when sending to the 3DConnexion software
 */
constexpr int16_t ROTATION_RANGE[2] = { -800, +800 };
}; // namespace hid_space_mouse_internal

/**
 * emulate a 3DConnexion SpaceMouse using an Arduino Pro Micro
 * 
 * @note
 * the USB VID and PID must be changed to match the 3DConnexion SpaceMouse, so
 * VID = 0x256f and PID = 0xc631 (SpaceMouse Pro Wireless (cabled))
 */
class HIDSpaceMouse
{
public:
  HIDSpaceMouse()
  {
    memset(buttons, false, sizeof(buttons));
  }

  /**
   * Setup the space mouse.
   * @note call this early in setup()
   */
  void begin();

  /**
   * submit the current state of the space mouse
   */
  void submit();

  /**
   * set the translation of the space mouse
   * @param x x translation. range: -1.0 to 1.0
   * @param y y translation. range: -1.0 to 1.0
   * @param z z translation. range: -1.0 to 1.0
   * @note call submit() to send the data to the 3DConnexion software
   */
  inline void set_translation(const float x, const float y, const float z)
  {
    assert(x >= -1.0f && x <= 1.0f);
    assert(y >= -1.0f && y <= 1.0f);
    assert(z >= -1.0f && z <= 1.0f);

    this->x = x;
    this->y = y;
    this->z = z;
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
    assert(u >= -1.0f && u <= 1.0f);
    assert(v >= -1.0f && v <= 1.0f);
    assert(w >= -1.0f && w <= 1.0f);

    this->u = u;
    this->v = v;
    this->w = w;
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
    assert(button < hid_space_mouse_internal::BUTTON_COUNT);

    buttons[button] = state;
  }

private:
  /**
   * internal state values, normalized to -1.0 to 1.0
   */
  float x = 0.0f,
        y = 0.0f,
        z = 0.0f,
        u = 0.0f, // rx
        v = 0.0f, // ry
        w = 0.0f; // rz

  // TODO: optimize memory by making this a bit map
  /**
   * internal state values for all buttons
   */
  bool buttons[hid_space_mouse_internal::BUTTON_COUNT];

private:
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
   * @note accesses the buttons array directly.
   */
  void submit_buttons();
};

