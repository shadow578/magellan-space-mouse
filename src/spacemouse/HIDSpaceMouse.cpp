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

HIDSpaceMouse::HIDSpaceMouse(Print *log) : PluggableUSBModule(2, 1, endpointTypes)
{
  PluggableUSB().plug(this);

  // ensure state is cleared
  this->state.x = 0.0f;
  this->state.y = 0.0f;
  this->state.z = 0.0f;
  this->state.u = 0.0f;
  this->state.v = 0.0f;
  this->state.w = 0.0f;
  memset(this->state.buttons, false, sizeof(this->state.buttons));

  // ensure last state is cleared
  commit_state();

  // setup logging output
  this->log = log;
}

int HIDSpaceMouse::getInterface(uint8_t* interfaceNumber)
{
  #define SPACEMOUSE_D_HIDREPORT(length)                                     \
    {                                                                      \
        9, 0x21, 0x11, 0x01, 0, 1, 0x22, lowByte(length), highByte(length) \
    }

  interfaceNumber[0] += 1;
  const SpaceMouseHIDDescriptor descriptor = {
    D_INTERFACE(pluggedInterface, 2, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
    SPACEMOUSE_D_HIDREPORT(sizeof(SPACE_MOUSE_REPORT_DESCRIPTOR)),
    D_ENDPOINT(USB_ENDPOINT_IN(endpoint_tx()), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0),
    D_ENDPOINT(USB_ENDPOINT_OUT(endpoint_rx()), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0),
  };

  return USB_SendControl(0, &descriptor, sizeof(descriptor));
}

int HIDSpaceMouse::getDescriptor(USBSetup& setup)
{
  if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE)
  {
    return 0;
  }

  if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE)
  {
    return 0;
  }

  if (setup.wIndex != pluggedInterface)
  {
    return 0;
  }

  // protocol = HID_REPORT_PROTOCOL;

  return USB_SendControl(TRANSFER_PGM, SPACE_MOUSE_REPORT_DESCRIPTOR, sizeof(SPACE_MOUSE_REPORT_DESCRIPTOR));
}

bool HIDSpaceMouse::setup(USBSetup& setup)
{
  if (pluggedInterface != setup.wIndex)
  {
    return false;
  }

  if (setup.bmRequestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE)
  {
    if (setup.bRequest == HID_GET_REPORT)
    {
      return true;
    }
    if (setup.bRequest == HID_GET_PROTOCOL)
    {
      return true;
    }
  }

  if (setup.bmRequestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE)
  {
    if (setup.bRequest == HID_SET_PROTOCOL)
    {
      // protocol = setup.wValueL;
      return true;
    }
    if (setup.bRequest == HID_SET_IDLE)
    {
      // idle = setup.wValueL;
      return true;
    }
    if (setup.bRequest == HID_SET_REPORT)
    {
			// If you press "Calibrate" in the windows driver of a _SpaceNavigator_ the following setup request is sent:
			// wValue: 0x0307
			// wIndex: 0 (0x0000)
			// wLength: 2
			// Data Fragment: 0700
			// Unfortunately, we are simulating a _SpaceMouse Pro Wireless (cabled)_, because it has more than two buttons
			// With this SM pro, the windows driver is NOT sending this status report and their is no point in waiting for it...

      if (this->log != nullptr)
      {
        this->log->println(F("[SpaceMouse] got HID_SET_REPORT: "));
        this->log->print(F("  wValue="));
        this->log->println(setup.wValueL, HEX);
        this->log->print(F("  wIndex="));
        this->log->println(setup.wIndex, HEX);
        this->log->print(F("  wLength="));
        this->log->println(setup.wLength, HEX);
        this->log->print(F("  Value H/L="));
        this->log->print(setup.wValueH, HEX);
        this->log->print(F("/"));
        this->log->println(setup.wValueL, HEX);        
      }
			return true;
    }
  }

  return false;
}

int HIDSpaceMouse::send_report(const uint8_t id, const uint8_t *data, const size_t len)
{
  auto ret1 = USB_Send(endpoint_tx(), &id, 1);
  if (ret1 < 0)
  {
    return ret1;
  }

  auto ret2 = USB_Send(endpoint_tx() | TRANSFER_RELEASE, data, len);
  if (ret2 < 0)
  {
    return ret2;
  }

  return ret1 + ret2;
}

void HIDSpaceMouse::update()
{
  get_led_state();

  switch(this->hid_state)
  {
    case IDLE:
    {
      if (state_dirty())
      {
        commit_state();
        this->hid_state = SEND_TRANSLATION;

        if (this->log != nullptr)
        {
          this->log->println(F("[SpaceMouse] mouse state updated, entering SEND_TRANSLATION"));
        }
      }

      break;
    }
    case SEND_TRANSLATION:
    {
      if (can_send_next_report())
      {
        submit_translation(
          map_normal_float(this->submit_state.x, POSITION_RANGE),
          map_normal_float(this->submit_state.y, POSITION_RANGE),
          map_normal_float(this->submit_state.z, POSITION_RANGE)
        );
        this->hid_state = SEND_ROTATION;
      }
      break;
    }
    case SEND_ROTATION:
    {
      if (can_send_next_report())
      {
        submit_rotation(
          map_normal_float(this->submit_state.u, ROTATION_RANGE),
          map_normal_float(this->submit_state.v, ROTATION_RANGE),
          map_normal_float(this->submit_state.w, ROTATION_RANGE)
        );
        this->hid_state = SEND_BUTTONS;
      }
      break;
    }
    case SEND_BUTTONS:
    {
      if (can_send_next_report())
      {
        submit_buttons(this->submit_state.buttons);
        this->hid_state = IDLE;
      }
      break;
    }
    default:
    {
      // got into a invalid state, reset to IDLE
      this->hid_state = IDLE;
    }
  }
}

void HIDSpaceMouse::get_led_state()
{
  if (USB_Available(endpoint_rx()) >= 2)
  {
    uint8_t data[2];
    USB_Recv(endpoint_rx(), data, 2);
    
    // is LED report?
    if (data[0] == LED_REPORT_ID)
    {
      ledState = data[1] == 1;

      if (this->log != nullptr)
      {
        this->log->print(F("[SpaceMouse] got LED state: "));
        this->log->println(ledState ? F("on") : F("off"));
      }
    }
  }
  
}

void HIDSpaceMouse::submit_translation(const int16_t x, const int16_t y, const int16_t z)
{
  if (this->log != nullptr)
  {
    this->log->print(F("[SpaceMouse] submit_translation("));
    this->log->print(x);
    this->log->print(F(", "));
    this->log->print(y);
    this->log->print(F(", "));
    this->log->print(z);
    this->log->println(F(")"));
  }

  const uint8_t translation[6] = {
    static_cast<uint8_t>(x & 0xFF), static_cast<uint8_t>(x >> 8),
    static_cast<uint8_t>(y & 0xFF), static_cast<uint8_t>(y >> 8),
    static_cast<uint8_t>(z & 0xFF), static_cast<uint8_t>(z >> 8)
  };

  send_report(TRANSLATION_REPORT_ID, translation, 6);
}

void HIDSpaceMouse::submit_rotation(const int16_t u, const int16_t v, const int16_t w)
{
  if (this->log != nullptr)
  {
    this->log->print(F("[SpaceMouse] submit_rotation("));
    this->log->print(u);
    this->log->print(F(", "));
    this->log->print(v);
    this->log->print(F(", "));
    this->log->print(w);
    this->log->println(F(")"));
  }

  const uint8_t rotation[6] = {
    static_cast<uint8_t>(u & 0xFF), static_cast<uint8_t>(u >> 8),
    static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>(v >> 8),
    static_cast<uint8_t>(w & 0xFF), static_cast<uint8_t>(w >> 8)
  };

  send_report(ROTATION_REPORT_ID, rotation, 6);
}

void HIDSpaceMouse::submit_buttons(const bool buttons[BUTTON_COUNT])
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

  if (this->log != nullptr)
  {
    this->log->print(F("[SpaceMouse] submit_buttons(): "));
    for (size_t i = 0; i < len; i++)
    {
      this->log->print(data[i], BIN);
      this->log->print(F(" "));
    }
    this->log->println();
  }

  send_report(BUTTON_REPORT_ID, data, len);
}
