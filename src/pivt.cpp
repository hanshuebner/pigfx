
#include <cstring>

#include <iostream>

#include <circle/startup.h>

#include "pivt.h"

using namespace std;

PiVT* PiVT::_this = nullptr;

PiVT::PiVT()
  : Logging("PiVT"),
    _timer(&_interrupt),
    _logger(LogDebug, &_timer),
    _usb_hci(&_interrupt, &_timer)
{
  _interrupt.Initialize();
  _serial_device.Initialize(38400);
  _logger.Initialize(&_serial_device);
  _timer.Initialize();
  _usb_hci.Initialize();

  _terminal = new Terminal(&_serial_device);

  log(LogNotice, "PiVT starting");

  _this = this;
}

PiVT::ShutdownMode
PiVT::run()
{
  log(LogDebug, "PiVT initialized, running");
  while (1) {
    _terminal->process();
  }
}

int
main(void)
{
  PiVT pivt;

  pivt.run();

  return EXIT_HALT;
}
