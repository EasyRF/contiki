#include "contiki.h"
#include "samr21-rf.h"

static uint16_t lfsr;

/*---------------------------------------------------------------------------*/
uint16_t
fibonacci(void)
{
  unsigned bit;

  /* taps: 16 14 13 11; feedback polynomial: x^16 + x^14 + x^13 + x^11 + 1 */
  bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
  lfsr =  (lfsr >> 1) | (bit << 15);

  return lfsr;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Generates a new random number using the cc2538 RNG.
 * \return     The random number.
 */
unsigned short
rand(void)
{
  return fibonacci();
}
/*---------------------------------------------------------------------------*/
void
srand(unsigned short seed)
{
  if (seed == 0) {
    lfsr = samr21_random_rand();
  } else {
    /* Use supplied seed */
    lfsr = seed;
  }
}
