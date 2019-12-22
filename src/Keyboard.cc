
#include <cstring>

#include "uart.h"

#include "Keyboard.h"

using namespace std;

Keyboard::DeadKey Keyboard::dead_key;

Keyboard::Keyboard()
  : _error(false)
{
#include "keymap.inc"
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

    auto str = (*handler)();
    if (str.length()) {
      uart_write(str.c_str(), str.length());
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
