// -*- C++ -*-

#pragma once

#include <circle_stdlib_app.h>

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/types.h>

#include "Logging.h"
#include "Terminal.h"

class PiVT
  : protected Logging
{
public:

  enum ShutdownMode {
    ShutdownNone,
    ShutdownHalt,
    ShutdownReboot
  };

  PiVT();

  void initialize();
  ShutdownMode run();

private:
  CActLED _act_led;
  CKernelOptions _options;
  CDeviceNameService _device_name_service;
  CNullDevice _null_device;
  CExceptionHandler _exception_handler;
  CInterruptSystem _interrupt;
  CSerialDevice _serial_device;
  CTimer _timer;
  CLogger _logger;
  CUSBHCIDevice _usb_hci;
  CEMMCDevice _emmc;
  CFATFileSystem _file_system;


  Terminal* _terminal;

  static PiVT* _this;
};
