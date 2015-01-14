#include "compiler.h"
#include "stack_test.h"
#include "log.h"


extern uint32_t _sstack;

/*---------------------------------------------------------------------------*/
void
write_aa_to_stack()
{
  uint8_t * start, * end;

  start = (uint8_t *)&_sstack;
  end = (uint8_t *)(__get_MSP() - 1000);

  for (; start < end; start++) {
    *start = 0xAA;
  }
}
/*---------------------------------------------------------------------------*/
void
print_stack_info(void)
{
  uint8_t * start, * end, *curr;

  start = (uint8_t *)&_sstack;
  end = (uint8_t *)__get_MSP();

  for (curr = start; curr < end; curr++) {
    if (*curr != 0xAA) {
      break;
    }
  }

  INFO("STACK INFO: start = %lX, end = %lX, curr = %lX, left = %lX",
       (long)start, (long)end, (long)curr, (long)(curr - start));
}
