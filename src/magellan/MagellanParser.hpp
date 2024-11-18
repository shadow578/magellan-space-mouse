#pragma once
#include <Arduino.h>
#include <assert.h>

namespace magellan_internal
{
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
   * magic string that must be included in the version response
   */
  static const char VERSION_MAGIC[] = "MAGELLAN";

  /**
   * timeout for waiting for the space mouse to be ready
   * @note starts at INIT_RESET and is cleared when space mouse reports version and mode 3
   */
  constexpr uint32_t READY_WAIT_TIMEOUT = 5000; // 5 seconds
}

/**
 * parser for the Magellan space mouse
 */
class MagellanParser
{
public:
  MagellanParser(Print *log = nullptr)
  {
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

  bool ready() const 
  {
    return init_state == DONE;
  }

  float get_x() const { return x; }
  float get_y() const { return y; }
  float get_z() const { return z; }
  float get_u() const { return u; }
  float get_v() const { return v; }
  float get_w() const { return w; }

  uint16_t get_buttons() const { return buttons; }
  bool get_button(const uint8_t button) const
  {
    assert(button < 12);
    return buttons & (1 << button);
  }

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
   * update init sequence
   */
  void update_init();

private:
  enum rx_state_t
  {
    IDLE,                         // waiting for start of message
    READ_MESSAGE,                 // read message data until MESSAGE_END separator
    WAIT_MESSAGE_END              // wait for MESSAGE_END separator, drop all data
  };

  enum message_type_t : char
  {
    UNKNOWN = 0,             // unknown message type
    VERSION = 'v',           // version message
    KEYPRESS = 'k',          // keypress message
    POSITION_ROTATION = 'd', // position and rotation message
    MODE_CHANGE = 'm',       // mode change message
    ZERO = 'z'               // zeroed message
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
  char rx_buffer[256];

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
   * internal state values, normalized to -1.0 to 1.0
   */
  float x = 0.0f,
        y = 0.0f,
        z = 0.0f,
        u = 0.0f, // rx
        v = 0.0f, // ry
        w = 0.0f; // rz

  /**
   * internal state values for all buttons.
   * @note up to 12 buttons are supported.
   * @note each button is represented by a single bit. top 4 bits are unused.
   */
  uint16_t buttons = 0;

private:
  /**
   * send a command to the space mouse
   * @param command the command to send
   * @note adding MESSAGE_END is the responsibility of the caller
   */
  void send_command(const char* command);

  /**
   * process a message received from the space mouse
   * @param type the type of the message
   * @param payload the message data. does not include the message type
   * @param len the length of the message data
   * @return were any state values updated?
   */
  bool process_message(const message_type_t type, const char* payload, const uint8_t len);

  // functions to process specific message types
  bool process_version(const char* payload, const uint8_t len);
  bool process_mode_change(const char* payload, const uint8_t len);
  bool process_zero(const char* payload, const uint8_t len);
  bool process_keypress(const char* payload, const uint8_t len);
  bool process_position_rotation(const char* payload, const uint8_t len);

private:
  Print *log = nullptr;
};
