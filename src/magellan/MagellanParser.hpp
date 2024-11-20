#pragma once
#include <Arduino.h>
#include "util.hpp"

namespace magellan_internal
{
  /**
   * size of the message RX buffer
   */
  constexpr size_t MESSAGE_BUFFER_SIZE = 128;

  /**
   * number of buttons supported by the space mouse
   */
  constexpr uint8_t BUTTON_COUNT = 9;

  /**
   * message separator, added to the end of each message
   */
  constexpr char MESSAGE_END = '\r';

  /**
   * reset command
   */
  static const char COMMAND_RESET[] = "\rvt\r";

  /**
   * get version command
   */
  static const char COMMAND_GET_VERSION[] = "vQ\r";

  /**
   * enable button reporting command
   */
  static const char COMMAND_ENABLE_BUTTON_REPORTING[] = "kQ\r";

  /**
   * set mode 3 command
   */
  static const char COMMAND_SET_MODE3[] = "m3\r";

  /**
   * zero command
   */
  static const char COMMAND_ZERO[] = "z\r";

  /**
   * beep command
   */
  static const char COMMAND_BEEP[] = "b\r";

  /**
   * magic string that must be included in the version response
   */
  static const char VERSION_MAGIC[] = "MAGELLAN";

  /**
   * timeout for waiting for the space mouse to be ready
   * @note starts at INIT_RESET and is cleared when space mouse reports version and mode 3
   */
  constexpr uint32_t READY_WAIT_TIMEOUT = 5000; // 5 seconds

  struct axis_bounds_t
  {
    int16_t min;
    int16_t max;
  };

  struct axis_calibration_t
  {
    axis_bounds_t x;
    axis_bounds_t y;
    axis_bounds_t z;
    axis_bounds_t u;
    axis_bounds_t v;
    axis_bounds_t w;
  };
}

/**
 * parser for the Magellan space mouse
 */
class MagellanParser
{
public:
  MagellanParser(const magellan_internal::axis_calibration_t *calibration, Print *log = nullptr)
  {
    this->calibration = calibration;
    this->log = log;
  }

  /**
   * setup the space mouse and initialize
   * @param serial the serial port to use. must be exclusive to the space mouse
   * @note call in setup()
   * @note
   * the mouse will be initialize by subsequent calls to update().
   * check is_ready() to see if the mouse is ready.
   */
  inline void begin(HardwareSerial *serial)
  {
    if (this->log != nullptr)
    {
      this->log->println(F("[Magellan] begin()"));
    }

    this->serial = serial;
    this->serial->begin(9600);

    reset();
  }

  /**
   * reset the state machine. This will also cause the space mouse to be re-initialized.
   */
  inline void reset()
  {
    if (this->log != nullptr)
    {
      this->log->println(F("[Magellan] reset()"));
    }

    this->init_state = RESET;
    this->init_wait_until = 0;
    this->last_reset_millis = 0;
    this->mode = 0;

    this->rx_state = IDLE;

    this->x = 0.0f;
    this->y = 0.0f;
    this->z = 0.0f;
    this->u = 0.0f;
    this->v = 0.0f;
    this->w = 0.0f;

    this->buttons = 0;
  }

  /**
   * update the state machine, read new data, ...
   * @return true if values have changed
   * @note call in loop()
   * @note
   * must be called even when ready() returns false.
   * values are only valid when ready() returns true.
   */
  bool update();

  /**
   * make the space mouse beep
   */
  void beep()
  {
    send_command(magellan_internal::COMMAND_BEEP);
    delay(100);
    send_command(magellan_internal::COMMAND_BEEP);
  }

  bool ready() const
  {
    return init_state == DONE;
  }

  // normalize values to be in the range [-1.0, 1.0] using the calibration values
  #define SCALE(axis)                            \
    /*positive?*/((this->axis > 0.0f) ?          \
    (this->axis / this->calibration->axis.max) : \
    /*negative?*/(this->axis < 0.0f) ?           \
    (this->axis / this->calibration->axis.min) : \
    /*zero*/0.0f)

  float get_x() const { return SCALE(x); }
  float get_y() const { return SCALE(y); }
  float get_z() const { return SCALE(z); }
  float get_u() const { return SCALE(u); }
  float get_v() const { return SCALE(v); }
  float get_w() const { return SCALE(w); }

  int16_t get_x_raw() const { return x; }
  int16_t get_y_raw() const { return y; }
  int16_t get_z_raw() const { return z; }
  int16_t get_u_raw() const { return u; }
  int16_t get_v_raw() const { return v; }
  int16_t get_w_raw() const { return w; }

  uint16_t get_buttons() const { return buttons; }

  /**
   * get the state of a button
   * @param button the button to check
   * @note
   * button numbers correspond to the printed numbers on the space mouse, subtracted by 1.
   * only exception is the "*" button, which is button #8.
   */
  bool get_button(const uint8_t button) const
  {
    assert(button < magellan_internal::BUTTON_COUNT, "MagellanParser::get_button() button out of range");
    return buttons & (1 << button);
  }

  uint8_t get_translation_sensitivity() const { return translation_sensitivity; }
  uint8_t get_rotation_sensitivity() const { return rotation_sensitivity; }

private:
  /**
   * the serial port to use
   */
  HardwareSerial *serial;

  enum init_state_t
  {
    RESET,                    // send reset command, wait 100ms
    REQUEST_VERSION,          // send get version command
    WAIT_VERSION,             // wait for version response
    REQUEST_BUTTON_REPORTING, // send enable button reporting command
    WAIT_BUTTON_REPORTING,    // wait for button reporting to be enabled
    REQUEST_SET_MODE,         // send set mode command
    WAIT_SET_MODE,            // wait for mode to be set
    REQUEST_ZERO,             // send zero command
    WAIT_ZERO,                // wait for zero command to be acknowledged
    DONE                      // init sequence is complete
  };

  /**
   * current state of the init sequence state machine
   */
  init_state_t init_state = RESET;

  /**
   * wait time before next state advance in init sequence
   */
  uint32_t init_wait_until = 0;

  /**
   * last time the RESET command was sent
   */
  uint32_t last_reset_millis = 0;

  /**
   * mode as reported by the space mouse.
   * @note expected to be 3 normally.
   */
  uint8_t mode = 0;

  /**
   * translation sensitivity level. 0-7
   */
  uint8_t translation_sensitivity = 0;

  /**
   * rotation sensitivity level. 0-7
   */
  uint8_t rotation_sensitivity = 0;

  /**
   * update init sequence
   */
  void update_init();

private:
  enum rx_state_t
  {
    IDLE,            // waiting for start of message
    READ_MESSAGE,    // read message data until MESSAGE_END separator
    WAIT_MESSAGE_END // wait for MESSAGE_END separator, drop all data
  };

  enum message_type_t : char
  {
    UNKNOWN = 0,             // unknown message type
    VERSION = 'v',           // version message
    KEYPRESS = 'k',          // keypress message
    POSITION_ROTATION = 'd', // position and rotation message
    MODE_CHANGE = 'm',       // mode change message
    ZERO = 'z',              // zeroed message
    SENSITIVITY_CHANGE = 'q' // sensitivity change message
  };

  /**
   * current state of the RX state machine
   */
  rx_state_t rx_state = IDLE;

  /**
   * current message type, when in READ_MESSAGE state
   */
  message_type_t message_type;

  /**
   * buffer for incoming data
   */
  char rx_buffer[magellan_internal::MESSAGE_BUFFER_SIZE];

  /**
   * length of the string in rx_buffer
   */
  uint8_t rx_len = 0;

  /**
   * update RX state machine
   * @param c the character to process
   * @return true if a message was processed
   */
  bool update_rx(const char c);

  /**
   * internal translation and rotation values, raw from the space mouse.
   */
  int16_t x = 0,
          y = 0,
          z = 0,
          u = 0, // rx
      v = 0,     // ry
      w = 0;     // rz

  /**
   * internal state values for all buttons.
   * @note up to 12 buttons are theoretically supported, only 9 are known to be used.
   * @note each button is represented by a single bit. top 4 bits are unused.
   */
  uint16_t buttons = 0;
  static_assert((sizeof(buttons) * 8) >= magellan_internal::BUTTON_COUNT, "MagellanParser::buttons is too small for given BUTTON_COUNT!");

private:
  /**
   * send a command to the space mouse
   * @param command the command to send
   * @note adding MESSAGE_END is the responsibility of the caller
   * @note this function may block for a few hundred milliseconds
   */
  void send_command(const char *command);

  /**
   * process a message received from the space mouse
   * @param type the type of the message
   * @param payload the message data. does not include the message type
   * @param len the length of the message data
   * @return were any state values updated?
   */
  bool process_message(const message_type_t type, const char *payload, const uint8_t len);

  // functions to process specific message types
  bool process_version(const char *payload, const uint8_t len);
  bool process_mode_change(const char *payload, const uint8_t len);
  bool process_sensitivity_change(const char *payload, const uint8_t len);
  bool process_zero(const char *payload, const uint8_t len);
  bool process_keypress(const char *payload, const uint8_t len);
  bool process_position_rotation(const char *payload, const uint8_t len);

  /**
   * decode a character into a nibble
   * @param c the character to decode
   * @return the decoded nibble
   */
  uint8_t decode_nibble(const char c);

  /**
   * decode a signed 16-bit word from a buffer of 4 characters
   * @param buffer the buffer to decode
   * @return the decoded value
   */
  int16_t decode_signed_word(const char *buffer);

private:
  const magellan_internal::axis_calibration_t *calibration;
  Print *log = nullptr;
};
