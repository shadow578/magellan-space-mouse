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
}

/**
 * parser for the Magellan space mouse
 */
class MagellanParser
{
public:
  /**
   * setup the space mouse and initialize
   * @param serial the serial port to use. must be exclusive to the space mouse
   * @note call in setup()
   * @note 
   * the mouse will be initialize by subsequent calls to update().
   * check is_ready() to see if the mouse is ready.
   */
  void begin(HardwareSerial *serial)
  {
    // setup serial for 9600 baud, 8N1
    this->serial = serial;
    this->serial->begin(9600);

    // reset the state machine
    this->state = INIT_RESET;
    this->ready = false;
  }

  /**
   * update the state machine, read new data, ...
   * @return true if values have changed
   * @note call in loop()
   */
  bool update();

  bool is_ready() const { return ready; }

  float get_x() const { return x; }
  float get_y() const { return y; }
  float get_z() const { return z; }
  float get_u() const { return u; }
  float get_v() const { return v; }
  float get_w() const { return w; }

  bool get_button(const uint8_t button) const
  {
    assert(button < 8);
    return buttons[button];
  }

private:
  enum state_t
  {
    INIT_RESET,                   // send reset command, wait 100ms
    INIT_GET_VERSION,             // send get version command, wait 100ms
    INIT_ENABLE_BUTTON_REPORTING, // send enable button reporting command, wait 100ms
    INIT_SET_MODE,                // send set mode command, wait 100ms
    INIT_ZERO,                    // send zero command, wait 100ms

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
  };

  /**
   * the serial port to use
   */
  HardwareSerial *serial;

  /**
   * current state of the state machine
   */
  state_t state = INIT_RESET;

  /**
   * current message type, when in READ_MESSAGE state
   */
  message_type_t message_type;

  /**
   * wait time before next state advance
   */
  uint32_t wait_until = 0;

  /**
   * buffer for incoming data
   */
  char rx_buffer[256];

  /**
   * length of the string in rx_buffer
   */
  uint8_t rx_len = 0;

  /**
   * is the remote initialized and ready?
   */
  bool ready = false;

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
   * internal state values for all buttons
   */
  bool buttons[8];

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
};
