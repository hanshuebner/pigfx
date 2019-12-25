// -*- C++ -*-

#pragma once

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/types.h>

#include "Terminal.h"

class PiVT
{
public:
  PiVT();

  void initialize();
  void run();

private:
  Terminal* _terminal;

  CMemorySystem _memory;
  CActLED _act_led;
  CKernelOptions _options;
  CDeviceNameService _device_name_service;
  CSerialDevice _serial;
  CExceptionHandler _exception_handler;
  CInterruptSystem _interrupt;
  CTimer _timer;
  CLogger _logger;
  CUSBHCIDevice _usb_hci;

  static PiVT* _this;
};
