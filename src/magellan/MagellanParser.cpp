#include "MagellanParser.hpp"

// delay between sending each character of a command
// can be used to slow down communication to the 
// Magellan to compensate for missing flow control
#define SEND_INTER_CHARACTER_DELAY 5 // ms; 0 to disable

using namespace magellan_internal;

uint8_t MagellanParser::decode_nibble(const char c)
{
  switch(c)
  {
    case '0': return 0;
    case 'A': return 1;
    case 'B': return 2;
    case '3': return 3;
    case 'D': return 4;
    case '5': return 5;
    case '6': return 6;
    case 'G': return 7;
    case 'H': return 8;
    case '9': return 9;
    case ':': return 10;
    case 'K': return 11;
    case '<': return 12;
    case 'M': return 13;
    case 'N': return 14;
    case '?': return 15;
    default: 
    {
      if (this->log != nullptr)
      {
        this->log->print(F("[Magellan] decode_nibble() got unknown character: \""));
        this->log->print(c);
        this->log->println(F("\""));
      }

      return 0;
    }
  }
}

int16_t MagellanParser::decode_signed_word(const char* buffer)
{
  const uint8_t n0 = decode_nibble(buffer[0]);
  const uint8_t n1 = decode_nibble(buffer[1]);
  const uint8_t n2 = decode_nibble(buffer[2]);
  const uint8_t n3 = decode_nibble(buffer[3]);

  // combine nibbles
  int16_t value = static_cast<int16_t>(n1 << 8 | n2 << 4 | n3);

  // apply sign
  if ((n0 & 0x08) == 0)
  {
    value = -(4096 - value);
  }

  return value;
}

bool MagellanParser::update()
{
  update_init();

  if (this->serial->available() != 0)
  {
    const char c = this->serial->read();
    return update_rx(c);
  }

  return false;
}

void MagellanParser::update_init()
{
  // should we wait?
  const uint32_t now = millis();
  if (now < this->init_wait_until)
  {
    return;
  }

  // if more than 5 seconds have passed since the last reset, we're probably stuck
  // try resetting the device and starting over
  // do not trigger a re-init when we're already in the reset state OR when we're done
  const bool stuck = (now - this->last_reset_millis) > READY_WAIT_TIMEOUT;
  if (stuck && this->init_state != RESET && this->init_state != DONE)
  {
    if (this->log != nullptr)
    {
      this->log->print(F("[Magellan] seems stuck at init_state="));
      this->log->print(this->init_state);
      this->log->println(F(", re-initializing..."));
    }

    this->reset();
    return;
  }

  switch(this->init_state)
  {
    case RESET:
    {
      this->send_command(COMMAND_RESET);
      this->init_wait_until = now + 500; // wait 500 ms
      this->last_reset_millis = now;
      this->init_state = REQUEST_VERSION;
      break;
    }
    case REQUEST_VERSION:
    {
      this->send_command(COMMAND_GET_VERSION);
      this->init_state = WAIT_VERSION;
      break;
    }
    case WAIT_VERSION:
    {
      // wait for version message
      // this state is advanced in process_message()
      break;
    }
    case REQUEST_BUTTON_REPORTING:
    {
      this->send_command(COMMAND_ENABLE_BUTTON_REPORTING);
      
      // FIXME: can't implement WAIT_BUTTON_REPORTING since idk how the space mouse
      // ACKs the command, so just wait a bit and hope for the best...

      // this->init_state = WAIT_BUTTON_REPORTING;
      this->init_wait_until = now + 500; // wait 500 ms
      this->init_state = REQUEST_SET_MODE;

      break;
    }
    case WAIT_BUTTON_REPORTING:
    {
      // wait for button reporting to be enabled
      // this state is advanced in process_message()
      break;
    }
    case REQUEST_SET_MODE:
    {
      this->send_command(COMMAND_SET_MODE3);
      this->init_state = WAIT_SET_MODE;
      break;
    }
    case WAIT_SET_MODE:
    {
      // wait for mode to be set
      // mode is parsed in process_message()
      if (this->mode == 3)
      {
        this->init_state = REQUEST_ZERO;
      }
      break;
    }
    case REQUEST_ZERO:
    {
      this->send_command(COMMAND_ZERO);
      this->init_state = WAIT_ZERO;
      break;
    }
    case WAIT_ZERO:
    {
      // wait for zero command to be acknowledged
      // this state is advanced in process_message()
      break;
    }
    case DONE:
    {
      // initialization is complete
      break;
    }
    default:
    {
      // unknown state, reset to RESET
      this->init_state = RESET;
      break;
    }
  }
}

bool MagellanParser::update_rx(const char c)
{
  switch(this->rx_state)
  {
    case IDLE:
    {
      // reset message buffer
      rx_len = 0;
      
      switch(c)
      {
        case 'v':
        {
          // version message
          this->message_type = VERSION;
          break;
        }
        case 'k':
        {
          // keypress message
          this->message_type = KEYPRESS;
          break;
        }
        case 'd':
        {
          // position and rotation message
          this->message_type = POSITION_ROTATION;
          break;
        }
        case 'm':
        {
          // mode change message
          this->message_type = MODE_CHANGE;
          break;
        }
        case 'z':
        {
          // zeroed message
          this->message_type = ZERO;
          break;
        }
        case 'q':
        {
          // sensitivity change message
          this->message_type = SENSITIVITY_CHANGE;
          break;
        }
        default:
        {
          // unknown message
          this->message_type = UNKNOWN;

          if (this->log != nullptr)
          {
            this->log->print(F("[Magellan] got unknown message type: \""));
            this->log->print(c);
            this->log->println(F("\""));
          }

          break;
        }
      }

      if (this->log != nullptr)
      {
        this->log->print("[Magellan] got message type: ");
        this->log->println(this->message_type);
      }
      
      this->rx_state = READ_MESSAGE;
      return false;
    }
    case READ_MESSAGE:
    {
      // is this the end of the message?
      if (c == MESSAGE_END)
      {
        this->rx_state = IDLE; // prepare for next message

        rx_buffer[rx_len] = '\0'; // ensure null-termination
        return process_message(this->message_type, rx_buffer, rx_len);
      }

      // add character to buffer
      // always leave one character for null-termination
      if (rx_len < (sizeof(rx_buffer) - 1))
      {
        rx_buffer[rx_len++] = c;
      }
      else
      {
        // buffer overflow, wait until the message ends and drop it
        this->rx_state = WAIT_MESSAGE_END;

        if (this->log != nullptr)
        {
          this->log->println(F("[Magellan] buffer overflow, entering WAIT_MESSAGE_END state"));
        }
      }
      return false;
    }
    case WAIT_MESSAGE_END:
    {
      if (c == MESSAGE_END)
      {
        this->rx_state = IDLE;
      }
      return false;
    }
    default:
    {
      // unknown state, reset to IDLE
      this->rx_state = IDLE;
      return false;
    }
  }
}

void MagellanParser::send_command(const char* command)
{
  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] send_command("));
    this->log->print(command);
    this->log->println(F(")"));
  }

  #if SEND_INTER_CHARACTER_DELAY > 0
    // write each character individually, with a delay between each
    // the Magellan normally uses hardware flow control, but the hardware 
    // isn't set up for it...
    for (uint8_t i = 0; command[i] != '\0'; i++)
    {
      this->serial->write(command[i]);
      this->serial->flush();
      delay(1);
    } 
  #else
    this->serial->print(command);
    this->serial->flush();
  #endif
}

bool MagellanParser::process_message(const message_type_t type, const char* payload, const uint8_t len)
{
  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] process_message("));
    this->log->print(static_cast<char>(type));
    this->log->print(F(", \""));
    this->log->print(payload);
    this->log->print(F("\", "));
    this->log->print(len);
    this->log->println(F(")"));
  }

  switch (type)
  {
    case VERSION:
    {
      return process_version(payload, len);
    }
    case KEYPRESS:
    {
      return process_keypress(payload, len);
    }
    case POSITION_ROTATION:
    {
      return process_position_rotation(payload, len);
    }
    case MODE_CHANGE:
    {
      return process_mode_change(payload, len);
    }
    case ZERO:
    {
      return process_zero(payload, len);
    }
    case SENSITIVITY_CHANGE:
    {
      return process_sensitivity_change(payload, len);
    }
    default:
    {
      // unknown message type
      return false;
    }
  }
}

bool MagellanParser::process_version(const char* payload, const uint8_t len)
{
  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] got version \""));
    this->log->print(payload);
  }

  // validate version includes 'MAGELLAN'
  if (strstr(payload, VERSION_MAGIC) == nullptr)
  {
    if (this->log != nullptr)
    {
      this->log->println(F("\" (FAULT)"));
    }

    return true;
  }

  if (this->log != nullptr)
  {
    this->log->println(F("\" (OK)"));
  }

  // advance init state if waiting for version
  if (this->init_state == WAIT_VERSION)
  {
    this->init_state = REQUEST_BUTTON_REPORTING;
  }
  return true;
}

bool MagellanParser::process_mode_change(const char* payload, const uint8_t len)
{
  // expect 1 character in the payload
  if (len != 1)
  {
    return false;
  }

  this->mode = decode_nibble(payload[0]);

  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] got mode: "));
    this->log->println(this->mode);
  }

  return true;
}

bool MagellanParser::process_sensitivity_change(const char* payload, const uint8_t len)
{
  // expect 2 characters in the payload
  if (len != 2)
  {
    return false;
  }

  const uint8_t s0 = decode_nibble(payload[0]);
  const uint8_t s1 = decode_nibble(payload[1]);

  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] got sensitivity: "));
    this->log->print(s0);
    this->log->print(F(", "));
    this->log->println(s1);
  }

  return true;
}

bool MagellanParser::process_zero(const char* payload, const uint8_t len)
{
  // don't care about the payload, there should be none
  if (this->log != nullptr)
  {
    this->log->println(F("[Magellan] got zeroed"));
  }

  // advance init state if waiting for zero
  if (this->init_state == WAIT_ZERO)
  {
    this->init_state = DONE;
  }

  return true;
}

bool MagellanParser::process_keypress(const char* payload, const uint8_t len)
{
  // expect 3 characters in the payload
  if (len != 3)
  {
    return false;
  }

  const uint8_t k0 = decode_nibble(payload[0]);
  const uint8_t k1 = decode_nibble(payload[1]);
  const uint8_t k2 = decode_nibble(payload[2]);

  this->buttons = k2 << 8 | k1 << 4 | k0;

  if (this->log != nullptr)
  {
    this->log->print(F("[Magellan] got keypress: "));
    this->log->println(this->buttons, BIN);
  }

  return true;
}

bool MagellanParser::process_position_rotation(const char* payload, const uint8_t len)
{
  // TODO: handle modes other than mode 3?

  // expect 24 characters in the payload 
  // (mode 3 = position and rotation)
  if (len != 24)
  {
    return false;
  }

  // get raw values
  const int16_t x = decode_signed_word(payload + 0);
  const int16_t y = decode_signed_word(payload + 8);
  const int16_t z = decode_signed_word(payload + 4);
  const int16_t u = decode_signed_word(payload + 20); // theta X = rX
  const int16_t v = decode_signed_word(payload + 12); // theta Y = rY
  const int16_t w = decode_signed_word(payload + 16); // theta Z = rZ

  // normalize values to be in the range [-1.0, 1.0]
  // the mouse seems to send values in a range of about [-2400, +2000] in 
  // highest sensitivity mode, so let's assume a effective range 
  // of [-2500 to +2500] and clamp the rest.
  // this may cause some weirdness, but it's way better than not working at all... 
  #define SCALE(n) (constrain((n / 2500.0f), -1.0, 1.0))

  // TODO: allow calibration of ranges depending on the sensitivity setting

  this->x = SCALE(x);
  this->y = SCALE(y);
  this->z = SCALE(z);
  this->u = SCALE(u);
  this->v = SCALE(v);
  this->w = SCALE(w);

  if (this->log != nullptr)
  {
    #define PRINT_VALUE(name, normalized, raw)  \
      this->log->print(F(name "="));       \
      this->log->print(normalized, 2);          \
      this->log->print(F(" ("));                \
      this->log->print(raw);                    \
      this->log->print(F(")"))

    this->log->print(F("[Magellan] got position/rotation:"));
    PRINT_VALUE("x", this->x, x);
    PRINT_VALUE(", y", this->y, y);
    PRINT_VALUE(", z", this->z, z);
    PRINT_VALUE(", u", this->u, u);
    PRINT_VALUE(", v", this->v, v);
    PRINT_VALUE(", w", this->w, w);
    this->log->println();
  }

  return true;
}
