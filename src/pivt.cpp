
#include <cstring>

#include <circle/startup.h>

#include "pivt.h"

using namespace std;

extern "C" void
LogWrite(__unused const char* pSource, __unused unsigned Severity, __unused const char* fmt, ...)
{
}

PiVT* PiVT::_this = nullptr;

PiVT::PiVT()
  : _terminal(new Terminal()),
    _timer(&_interrupt),
    _logger(_options.GetLogLevel(), &_timer),
    _usb_hci(&_interrupt, &_timer)
{
  _this = this;
}

void
PiVT::initialize()
{
  _act_led.Blink(2);
  _serial.Initialize(38400);

  CDevice *target = _device_name_service.GetDevice(_options.GetLogDevice(), FALSE);
  target = &_serial;
  _logger.Initialize(target);

  _interrupt.Initialize();
  _timer.Initialize();
  _usb_hci.Initialize();
  _act_led.Blink(2);
}

void
PiVT::run()
{
  while (1) {
  }
}

extern "C" int
main()
{
#if 0
  PiVT pivt;
  pivt.initialize();
  pivt.run();
#else
  CMemorySystem memory;
  CActLED act_led;
  CSerialDevice serial;
  CExceptionHandler exception_handler;
  CInterruptSystem interrupt;
  CTimer timer(&interrupt);
  CLogger logger(LogDebug);

  act_led.Blink(5);
  serial.Initialize(38400);
  // logger.Initialize(&serial);
  // interrupt.Initialize();
  act_led.Blink(5);
  const char* message = "Hello world!\r\n";
  serial.Write(message, strlen(message));
#endif
  
  return EXIT_HALT;
}
