
#include <cstring>

#include "uart.h"

#include "Keyboard.h"

using namespace std;

Keyboard::DeadKey Keyboard::dead_key;

Keyboard::Keyboard(Terminal& terminal)
  : _terminal(terminal),
    _error(false)
{
#include "keymap.inc"
}

void
Keyboard::key_pressed(unsigned char modifiers,
                      unsigned char key_code)
{
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
  if (memcmp(keys, error, 6)) {
    _error = true;
    _keys_pressed.clear();
    return;
  }
  set<unsigned char> keys_pressed_now;
  for (int i = 0; i < 6; i++) {
    if (keys[i]) {
      if (_keys_pressed.find(keys[i]) == _keys_pressed.end()) {
        key_pressed(modifiers, keys[i]);
        keys_pressed_now.insert(keys[i]);
      }
    }
  }
  _keys_pressed = keys_pressed_now;
}
