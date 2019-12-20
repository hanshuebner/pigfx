#include "../uspi/include/uspios.h"
#include "irq.h"
#include "raspihwconfig.h"
#include "timer.h"
#include "uart.h"

#ifndef __unused
#define __unused __attribute__((unused))
#endif

void
MsDelay(unsigned nMilliSeconds)
{
  usleep(nMilliSeconds * 1000);
}

void
usDelay(unsigned nMicroSeconds)
{
  usleep(nMicroSeconds);
}

unsigned
StartKernelTimer(
  unsigned nHzDelay, // in HZ units (see "system configuration" above)
  TKernelTimerHandler* pHandler,
  void* pParam,
  void* pContext) // handed over to the timer handler
{
  return attach_timer_handler(nHzDelay * 1000 / HZ, pHandler, pParam, pContext);
}

void
CancelKernelTimer(unsigned hTimer)
{
  detach_timer_handler(hTimer);
}

void
ConnectInterrupt(unsigned nIRQ, TInterruptHandler* pHandler, void* pParam)
{
  irq_attach_handler(nIRQ, pHandler, pParam);
}

int
SetPowerStateOn(
  unsigned nDeviceId) // "set power state" to "on", wait until completed
{
  if (RHW_SUCCESS !=
      rhw_set_device_power((RHW_DEVICE)nDeviceId, RHW_POWER_ON)) {
    return 0;
  }
  usleep(500000); // Wait some more for wireless keyboards startup time
  return 1;
}

int
GetMACAddress(unsigned char Buffer[6]) // "get board MAC address"
{
  if (RHW_SUCCESS != rhw_get_mac_address(Buffer))
    return 0;

  return 1;
}

void
uspi_assertion_failed(__unused const char* pExpr, __unused const char* pFile, __unused unsigned nLine)
{
  while (1)
    usleep(1000000);
}

void
DebugHexdump(const void* pBuffer,
             unsigned nBufLen,
             __unused const char* pSource /* = 0 */)
{
  uart_dump_mem((unsigned char*)pBuffer, (unsigned char*)(pBuffer) + nBufLen);
}

int
raise(int signum)
{
  return signum;
}
