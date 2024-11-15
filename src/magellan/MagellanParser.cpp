#include "MagellanParser.hpp"

using namespace magellan_internal;

/**
 * decode a character into a nibble
 */
static uint8_t decode_nibble(const char c)
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
    default: return 0;
  }
}

/**
 * decode a signed 16-bit word from a buffer of 4 characters
 */
static int16_t decode_signed_word(const char* buffer)
{
  uint8_t n0 = decode_nibble(buffer[0]);
  const uint8_t n1 = decode_nibble(buffer[1]);
  const uint8_t n2 = decode_nibble(buffer[2]);
  const uint8_t n3 = decode_nibble(buffer[3]);

  // extract sign and clear it from the nibble
  const bool sign = (n0 & 0x08) != 0;
  n0 &= 0x07;

  // combine nibbles
  uint16_t value = n3 << 12 | n2 << 8 | n1 << 4 | n0;

  // apply sign
  if (sign)
  {
    value = -value;
  }

  return value;
}

void MagellanParser::update()
{
  const uint32_t now = millis();
  if (now < this->wait_until)
  {
    // have to wait
    return;
  }

  if (this->serial->available() == 0)
  {
    // no data available
    return;
  }

  const char c = this->serial->read();

  switch(this->state)
  {
//#region initialization sequence
    case INIT_RESET:
    {
      this->send_command(COMMAND_RESET);
      this->wait_until = now + 100; // 100 ms
      break;
    }
    case INIT_GET_VERSION:
    {
      this->send_command(COMMAND_GET_VERSION);
      this->wait_until = now + 100; // 100 ms
      break;
    }
    case INIT_ENABLE_BUTTON_REPORTING:
    {
      this->send_command(COMMAND_ENABLE_BUTTON_REPORTING);
      this->wait_until = now + 100; // 100 ms
      break;
    }
    case INIT_SET_MODE:
    {
      this->send_command(COMMAND_SET_MODE3);
      this->wait_until = now + 100; // 100 ms
      break;
    }
    case INIT_ZERO:
    {
      this->send_command(COMMAND_ZERO);
      this->wait_until = now + 100; // 100 ms
      break;
    }
//#endregion
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
        default:
        {
          // unknown message
          this->message_type = UNKNOWN;
          break;
        }
        
        this->state = READ_MESSAGE;
      }
      break;
    }
    case READ_MESSAGE:
    {
      // is this the end of the message?
      if (c == MESSAGE_END)
      {
        rx_buffer[rx_len] = '\0'; // ensure null-termination

        process_message(this->message_type, rx_buffer, rx_len);
        break;
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
        this->state = WAIT_MESSAGE_END;
      }
      break;
    }
    case WAIT_MESSAGE_END:
    {
      if (c == MESSAGE_END)
      {
        this->state = IDLE;
      }
      break;
    }
    default:
    {
      // unknown state, reset to IDLE
      this->state = IDLE;
      break;
    }
  }
}

void MagellanParser::send_command(const char* command)
{
  this->serial->print(command);
  this->serial->flush();
}

void MagellanParser::process_message(const message_type_t type, const char* payload, const uint8_t len)
{
  switch (type)
  {
    case VERSION:
    {
      // validate version includes 'MAGELLAN'
      if (strstr(payload, VERSION_MAGIC) == nullptr)
      {
        break;
      }

      this->ready = true;
      break;
    }
    case KEYPRESS:
    {
      // expect 3 characters in the payload
      if (len != 3)
      {
        break;
      }

      const uint8_t k0 = decode_nibble(payload[0]);
      const uint8_t k1 = decode_nibble(payload[1]);
      const uint8_t k2 = decode_nibble(payload[2]);

      const uint16_t keys = k2 << 8 | k1 << 4 | k0;

      // TODO parse key presses
      break;
    }
    case POSITION_ROTATION:
    {
      // expect 24 characters in the payload 
      // (mode 3 = position and rotation)
      if (len != 24)
      {
        break;
      }

      // get raw values
      const int16_t x = decode_signed_word(payload + 0);
      const int16_t y = decode_signed_word(payload + 8);
      const int16_t z = decode_signed_word(payload + 4);
      const int16_t u = decode_signed_word(payload + 20); // theta X = rX
      const int16_t v = decode_signed_word(payload + 12); // theta Y = rY
      const int16_t w = decode_signed_word(payload + 16); // theta Z = rZ

      // normalize values to be in the range [-1.0, 1.0]
      constexpr float D = 32767.0f;
      this->x = x / D;
      this->y = y / D;
      this->z = z / D;
      this->u = u / D;
      this->v = v / D;
      this->w = w / D;
      break;
    }
    default:
    {
      // unknown message type
      break;
    }
  }
}

