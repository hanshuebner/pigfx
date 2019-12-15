#include "timer.h"

typedef struct
{
  _TimerHandler* handler;
  void* pParam;
  void* pContext;
  unsigned int microsec_interval;
  unsigned int last_tick;

} TimerUnit;

#define N_TIMERS 20
static TimerUnit timers[N_TIMERS];

void
timers_init()
{
  int i;
  for (i = 0; i < N_TIMERS; ++i) {
    timers[i].handler = 0;
  }
}

unsigned
attach_timer_handler(unsigned int millis,
                     _TimerHandler* handler,
                     void* pParam,
                     void* pContext)
{
  unsigned hnd;
  for (hnd = 0; hnd < N_TIMERS; ++hnd) {
    if (timers[hnd].handler == 0) {
      timers[hnd].handler = handler;
      timers[hnd].pParam = pParam;
      timers[hnd].pContext = pContext;
      timers[hnd].microsec_interval = millis * 1000;
      timers[hnd].last_tick = time_microsec();
      return hnd + 1;
    }
  }

  return 0;
}

void
detach_timer_handler(unsigned hnd)
{
  if (hnd - 1 < N_TIMERS)
    timers[hnd - 1].handler = 0;
}

void
timer_poll()
{
  unsigned hnd;

  for (hnd = 0; hnd < N_TIMERS; ++hnd) {
    volatile unsigned int tnow = time_microsec();

    if (timers[hnd].handler != 0 &&
        (tnow - timers[hnd].last_tick) > timers[hnd].microsec_interval &&
        (tnow > timers[hnd].last_tick ||
         (tnow < timers[hnd].last_tick &&
          (timers[hnd].last_tick - tnow) > 1000000))) {
      _TimerHandler* handler = timers[hnd].handler;
      timers[hnd].handler = 0;
      handler(hnd + 1, timers[hnd].pParam, timers[hnd].pContext);
    }
  }
}
