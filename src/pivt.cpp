
#include <cstring>

#include <iostream>

#include <circle/startup.h>

#include "pivt.h"

using namespace std;

PiVT* PiVT::_this = nullptr;

PiVT::PiVT()
  : Logging("PiVT"),
    _serial_device(&_interrupt),
    _timer(&_interrupt),
    _logger(LogDebug, &_timer),
    _usb_hci(&_interrupt, &_timer)
{
  _interrupt.Initialize();
  _serial_device.Initialize(38400);
  _serial_device.SetOptions(0);

  CDevice* logTarget = _device_name_service.GetDevice(_options.GetLogDevice(), false);
  if (logTarget == nullptr) {
    logTarget = &_null_device;
  }
  _logger.Initialize(logTarget);

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
