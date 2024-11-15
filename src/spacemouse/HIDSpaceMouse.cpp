#include "HIDSpaceMouse.hpp"

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

constexpr uint8_t TRANSLATION_REPORT_ID = 1;
constexpr uint8_t ROTATION_REPORT_ID = 2;

/**
 * range for postion (x,y,z) values when sending to the 3DConnexion software
 */
constexpr int16_t POSITION_RANGE[2] = { -800, +800 };

/**
 * range for rotation (u,v,w) values when sending to the 3DConnexion software
 */
constexpr int16_t ROTATION_RANGE[2] = { -800, +800 };

/**
 * map a normalised float value to a 16-bit integer
 * @param value the normalised float value, range -1.0 to 1.0
 * @param range the range of the 16-bit integer
 * @return the mapped 16-bit integer
 */
inline int16_t map_normal_float(const float value, const int16_t range[2])
{
  const int16_t span = range[1] - range[0];
  return range[0] + (value + 1.0f) * span / 2;
}

void HIDSpaceMouse::begin()
{
  // setup HID
  static HIDSubDescriptor node(hidReportDescriptor, sizeof(hidReportDescriptor));
  HID().AppendDescriptor(&node);
}

void HIDSpaceMouse::submit()
{
  submit_translation(
    map_normal_float(x, POSITION_RANGE),
    map_normal_float(y, POSITION_RANGE),
    map_normal_float(z, POSITION_RANGE)
  );
  submit_rotation(
    map_normal_float(u, ROTATION_RANGE),
    map_normal_float(v, ROTATION_RANGE),
    map_normal_float(w, ROTATION_RANGE)
  );
}

void HIDSpaceMouse::submit_translation(const int16_t x, const int16_t y, const int16_t z)
{
  const uint8_t translation[6] = {
    static_cast<uint8_t>(x & 0xFF), static_cast<uint8_t>(x >> 8),
    static_cast<uint8_t>(y & 0xFF), static_cast<uint8_t>(y >> 8),
    static_cast<uint8_t>(z & 0xFF), static_cast<uint8_t>(z >> 8)
  };

  HID().SendReport(TRANSLATION_REPORT_ID, translation, 6);
}

void HIDSpaceMouse::submit_rotation(const int16_t u, const int16_t v, const int16_t w)
{
  const uint8_t rotation[6] = {
    static_cast<uint8_t>(u & 0xFF), static_cast<uint8_t>(u >> 8),
    static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>(v >> 8),
    static_cast<uint8_t>(w & 0xFF), static_cast<uint8_t>(w >> 8)
  };

  HID().SendReport(ROTATION_REPORT_ID, rotation, 6);
}
