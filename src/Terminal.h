#pragma once

#include <memory>
#include <map>

#include <vterm.h>

#include <circle/timer.h>

#include "Framebuffer.h"
#include "Keyboard.h"

using namespace std;

class Terminal
{
 public:
  Terminal();

  void output(const char* const string, unsigned int length = 0);

  void debug();

  int damage(VTermRect rect);
  int movecursor(VTermPos position, __unused VTermPos oldPosition, int visible);
  int moverect(VTermRect dest, VTermRect src);

  void uart_write(const string& s);

 private:
  shared_ptr<Framebuffer> _framebuffer;
  shared_ptr<Keyboard> _keyboard;

  VTerm* _term;
  VTermScreen* _screen;
  VTermScreenCallbacks _callbacks;

  class UnicodeMap {
  public:
    UnicodeMap();

    map<uint32_t, uint8_t> _map;

    uint8_t to_dec_char(uint32_t code) { return _map.count(code) ? _map[code] : code; }
  };

  UnicodeMap _unicode_map;
};

