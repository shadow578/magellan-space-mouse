#include "HIDSpaceMouse.hpp"

using namespace hid_space_mouse_internal;

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

void HIDSpaceMouse::submit_buttons()
{
  size_t len = BUTTON_COUNT / 8;
  if (BUTTON_COUNT % 8 != 0)
  {
    // add one more byte for the remaining bits
    len++;
  }

  uint8_t data[len];
  memset(data, 0, len);

  // pack buttons array into data bit map
  for (size_t i = 0; i < BUTTON_COUNT; i++)
  {
    if (buttons[i])
    {
      data[i / 8] |= 1 << (i % 8);
    }
  }

  HID().SendReport(BUTTON_REPORT_ID, data, len);
}
