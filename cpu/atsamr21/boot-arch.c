#include "compiler.h"
#include "core_cmFunc.h"

void
boot_firmware(unsigned long address)
{
  /* Disable interrupts */
  __disable_irq();

  /* Set stack pointer */
  __set_MSP(((unsigned long *)address)[0]);

  /* We then call the reset vector from the vector table, which we
     happen to know is located at place [1] in the table. */
  ((void (**)(void))address)[1]();
}
