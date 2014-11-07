#include <asf.h>
#include "contiki.h"
#include "sys/clock.h"
#include "sys/cc.h"
#include "sys/etimer.h"
#include "sys/energest.h"


static volatile clock_time_t current_clock = 0;
static volatile unsigned long current_seconds = 0;

#ifndef CONTIKI_NO_CORE
static clock_time_t second_countdown = CLOCK_SECOND;
#endif


/*---------------------------------------------------------------------------*/
#ifndef CONTIKI_NO_CORE
void
SysTick_Handler(void)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  current_clock++;

  if(--second_countdown == 0) {
    current_seconds++;
    second_countdown = CLOCK_SECOND;
  }

  if(etimer_next_expiration_time() <= current_clock) {
    etimer_request_poll();
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
#endif
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  irq_initialize_vectors();
  cpu_irq_enable();
  sleepmgr_init();
  system_init();
  delay_init();
  SysTick_Config(system_cpu_clock_get_hz() / CLOCK_SECOND - 1UL);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return current_clock;
}
/*---------------------------------------------------------------------------*/
/**
 * Delay the CPU for dt microseconds
 */
void
clock_delay_usec(uint16_t dt)
{
  delay_us(dt);
}
/*---------------------------------------------------------------------------*/
/**
 * Wait for a multiple of 1 ms.
 */
void
clock_wait(clock_time_t i)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < (clock_time_t) i);
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  return current_seconds;
}
/*--------------------------------------------------------------------------*/
void
clock_set_seconds(unsigned long sec)
{
  current_seconds = sec;
}
/*--------------------------------------------------------------------------*/


