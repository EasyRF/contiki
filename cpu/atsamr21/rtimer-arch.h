#ifndef __RTIMER_ARCH_H__
#define __RTIMER_ARCH_H__

#include "dev/watchdog.h"
#include "sys/rtimer.h"

#define RTIMER_ARCH_SECOND  32768

#define RTIMER_BUSYWAIT_UNTIL(cond, max_time)                           \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) {  \
      watchdog_periodic();                                              \
    }                                                                   \
  } while(0)

void rtimer_arch_set(rtimer_clock_t t);
rtimer_clock_t rtimer_arch_now(void);

#endif /* __RTIMER_ARCH_H__ */
