// -*- C++ -*-

#pragma once

#include <memory>
#include <map>

#include <vterm.h>

#include <circle/timer.h>

#include "Logging.h"
#include "Framebuffer.h"
#include "Keyboard.h"

using namespace std;

class CSerialDevice;

class Terminal
  : protected Logging
{
 public:
  Terminal(CSerialDevice* serial_port);

  int damage(VTermRect rect);
  int movecursor(VTermPos position, __unused VTermPos oldPosition, int visible);
  int moverect(VTermRect dest, VTermRect src);

  void uart_write(const string& s);

  void process();

 private:
  shared_ptr<Framebuffer> _framebuffer;
  shared_ptr<Keyboard> _keyboard;

  CSerialDevice* _serial_port;

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
