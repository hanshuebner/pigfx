#pragma once

#include <memory>

#include <vterm.h>

#include "Framebuffer.h"
#include "Keyboard.h"

using namespace std;

class Terminal
{
 public:
  Terminal(shared_ptr<Framebuffer> framebuffer,
           shared_ptr<Keyboard> keyboard);

  void output(const char* const string, unsigned int length);

  int damage(VTermRect rect);
  int movecursor(VTermPos position, __unused VTermPos oldPosition, int visible);
  int moverect(VTermRect dest, VTermRect src);

 private:
  shared_ptr<Framebuffer> _framebuffer;
  shared_ptr<Keyboard> _keyboard;

  VTerm* _term;
  VTermScreen* _screen;
  VTermScreenCallbacks _callbacks;
};

