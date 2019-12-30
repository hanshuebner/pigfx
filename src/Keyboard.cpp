
#include "Keyboard.h"
#include "Terminal.h"

#include <cstring>

#include <circle/devicenameservice.h>
#include <circle/usb/usbkeyboard.h>

using namespace std;

unsigned char Keyboard::_modifiers;
unsigned char Keyboard::_keys[6];
Keyboard* Keyboard::_this;

void
handle_report_stub(unsigned char modifiers,
                   const unsigned char keys[6])
{
  Keyboard::_modifiers = modifiers;
  memcpy(Keyboard::_keys, keys, 6);
}

Keyboard::DeadKey Keyboard::dead_key;

void
Keyboard::initialize_keymap()
{
#include "keymap.inc"
}

Keyboard::Keyboard(Terminal* terminal)
  : Logging("Keyboard"),
    _error(false),
    _terminal(terminal)
{
  initialize_keymap();

  _usb_keyboard = (CUSBKeyboardDevice *) CDeviceNameService::Get()->GetDevice("ukbd1", FALSE);

  if (_usb_keyboard == 0) {
    log(LogError, "No keyboard found");
  } else {
    _this = this;

    _usb_keyboard->RegisterKeyStatusHandlerRaw(handle_report_stub);
  }
}

void
Keyboard::key_pressed(unsigned char modifiers,
                      unsigned char key_code)
{
  if (_map.count(key_code)) {
    auto definition = _map[key_code];
    auto handler = definition->_solo;

    if (modifiers & (Modifiers::LeftControl | Modifiers::RightControl)) {
      handler = definition->_control;
    } else if (modifiers & (Modifiers::LeftShift | Modifiers::RightShift)) {
      handler = definition->_shift;
    }

    auto str = (*handler)(this);
    if (str.length()) {
      _terminal->uart_write(str);
    }
  }
}

void
Keyboard::handle_report(unsigned char modifiers,
                        const unsigned char keys[6])
{
  static const unsigned char error[6] = { 1, 1, 1, 1, 1, 1 };
  static const unsigned char none[6] = { 0, 0, 0, 0, 0, 0 };

  if (_error) {
    // When the keyboard has reported an error, wait until it sends an
    // empty report.
    if (memcmp(keys, none, 6)) {
      return;
    }
    _error = false;
  }
  if (memcmp(keys, error, 6) == 0) {
    _error = true;
    _keys_pressed.clear();
    return;
  }
  set<unsigned char> keys_pressed_now;
  for (int i = 0; i < 6; i++) {
    if (keys[i]) {
      if (!_keys_pressed.count(keys[i])) {
        key_pressed(modifiers, keys[i]);
      }
      keys_pressed_now.insert(keys[i]);
    }
  }
  _keys_pressed = keys_pressed_now;
}

void
Keyboard::process()
{
  unsigned char modifiers;
  unsigned char keys[6];

  // Making this into a critical section is pretty heavy-handed.  It
  // would be better to use an interrupt safe queue instead, but for
  // now, this seems to work.

  EnterCritical();
  modifiers = _modifiers;
  memcpy(keys, _keys, 6);
  LeaveCritical();

  handle_report(modifiers, keys);
}

const string
Keyboard::CycleSerialSpeed::operator()(Keyboard* keyboard) const
{
  keyboard->terminal()->cycle_serial_speed();
  return "";
}

const string
Keyboard::ToggleScreenSize::operator()(Keyboard* keyboard) const
{
  keyboard->terminal()->toggle_screen_size();
  return "";
}
