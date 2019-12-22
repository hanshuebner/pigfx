// -*- C++ -*-

#pragma once

#include <set>
#include <map>
#include <string>

#include "Terminal.h"

using namespace std;

class Keyboard
{
public:
  Keyboard(Terminal& terminal);

  void handle_report(unsigned char modifiers,
                     const unsigned char keys[6]);

private:
  struct KeyDefinition;
  class KeypressHandler;

  Terminal& _terminal;
  set<unsigned char> _keys_pressed;
  bool _error;
  map<unsigned char, KeyDefinition*> _map;

  void key_pressed(unsigned char modifiers,
                   unsigned char key_code);

  struct KeyDefinition
  {
    KeyDefinition(const char* const name, KeypressHandler* solo, KeypressHandler* shift, KeypressHandler* control)
      : _name(name), _solo(solo), _shift(shift), _control(control)
    {}

    const string _name;
    KeypressHandler* _solo;
    KeypressHandler* _shift;
    KeypressHandler* _control;
  };

  class KeypressHandler
  {
  public:
    virtual ~KeypressHandler() {};
    virtual const string operator()() const = 0;
  };

  class String
    : public KeypressHandler
  {
  public:
    String(const char* s) : _s(s) {}
    virtual const string operator()() const { return _s; }

    const string _s;
  };

  class DeadKey
    : public KeypressHandler
  {
  public:
    virtual const string operator()() const { return ""; }
  };

  static DeadKey dead_key;
};
