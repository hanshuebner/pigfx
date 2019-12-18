#ifndef _PIGFX_TIMER_H_
#define _PIGFX_TIMER_H_

extern void usleep(unsigned int usec);
extern unsigned int time_microsec();

typedef void _TimerHandler(unsigned hTimer, void* pParam, void* pContext);

extern void timers_init();
extern unsigned attach_timer_handler(unsigned millis,
                                     _TimerHandler* handler,
                                     void* pParam,
                                     void* pContext);
extern void detach_timer_handler(unsigned hnd);
extern void timer_poll();

#endif
