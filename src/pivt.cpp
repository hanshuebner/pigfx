
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
  Terminal terminal(&_serial_device);

  log(LogDebug, "PiVT initialized, running");
  while (1) {
    terminal.process();
  }
}

int
main(void)
{
  PiVT pivt;

  pivt.run();

  return EXIT_HALT;
}
