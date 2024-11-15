#pragma once
#include <Arduino.h>
#include <HID.h>
#include <assert.h>

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

private:
  /**
   * internal state values, normalized to -1.0 to 1.0
   */
  float x = 0.0f,
        y = 0.0f,
        z = 0.0f,
        u = 0.0f,
        v = 0.0f,
        w = 0.0f;

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
};

