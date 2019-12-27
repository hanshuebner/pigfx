
#include <cstring>


#include <iostream>

#include <circle/startup.h>

#include "pivt.h"

using namespace std;

CLogger* logger = nullptr;

extern "C" void
LogWrite(const char* source, unsigned severity, const char* fmt, ...)
{
  if (logger) {
    va_list vl;
    va_start(vl, fmt);
    logger->WriteV(source, (TLogSeverity) severity, fmt, vl);
    va_end(vl);
  }
}

void
log(TLogSeverity severity, const char* fmt, ...)
{
  if (logger) {
    va_list vl;
    va_start(vl, fmt);
    logger->WriteV("PiVT", (TLogSeverity) severity, fmt, vl);
    va_end(vl);
  }
}

PiVT* PiVT::_this = nullptr;

PiVT::PiVT()
  : _timer(&_interrupt),
    _logger(LogDebug, &_timer)
{
  _act_led.Blink(1);

  _interrupt.Initialize();
  _serial_device.Initialize(38400);
  _logger.Initialize(&_serial_device);
  _timer.Initialize();

  //  _terminal = new Terminal();

  _act_led.Blink(1);

  logger = &_logger;

  log(LogNotice, "PiVT starting");

  _this = this;
}

PiVT::ShutdownMode
PiVT::run()
{
  Framebuffer framebuffer;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      VTermColor bg;
      vterm_color_indexed(&bg, 0);
      VTermColor fg;
      vterm_color_indexed(&fg, i * 16 + j);
      framebuffer.putc(i, j, 'H', fg, bg, VTermScreenCellAttrs());
    }
  }
  while (1) {
    framebuffer.handle_cursor();
  }
}

int
main(void)
{
  PiVT pivt;

  pivt.run();

  return EXIT_HALT;
}
