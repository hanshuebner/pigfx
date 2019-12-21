#pragma once

#include <memory>

#include <vterm.h>

#include "gfx.h"

using namespace std;

class Terminal
{
 public:
  Terminal(shared_ptr<Framebuffer> framebuffer);

  void output(const char* const string, unsigned int length);

  int damage(VTermRect rect);
  int movecursor(VTermPos position, __unused VTermPos oldPosition, int visible);
  int moverect(VTermRect dest, VTermRect src);

 private:
  shared_ptr<Framebuffer> _framebuffer;

  VTerm* _term;
  VTermScreen* _screen;
  VTermScreenCallbacks _callbacks;
};

