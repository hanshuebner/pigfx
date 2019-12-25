
#include <cstring>
#include <memory>
#include <string>

#include <vterm.h>

#include "Terminal.h"
#include "Framebuffer.h"
#include "Keyboard.h"

using namespace std;

extern "C" void
LogWrite(__unused const char* pSource, __unused unsigned Severity, __unused const char* fmt, ...)
{
}

extern "C" void
entry_point()
{
  auto framebuffer = make_shared<Framebuffer>();
  auto keyboard = make_shared<Keyboard>();
  auto terminal = make_shared<Terminal>(framebuffer, keyboard);

  while (1) {
  }
}
