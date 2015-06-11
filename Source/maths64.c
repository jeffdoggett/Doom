/* --------------------------------------------------------------------------------- */
/*
   When using StubsG, the 64 bit maths functions are not available
   so we link this file instead.
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef __riscos
#include <stdint.h>
#endif

/* --------------------------------------------------------------------------------- */

static uint64_t multiply_64 (uint32_t p1, uint32_t p2)
{
  uint32_t answer_1;
  uint32_t answer_2;
  uint32_t answer_3;
  uint32_t answer_4;
  uint64_t answer;

  answer_1 = (p1 & 0xFFFF) * (p2 & 0xFFFF);
  answer_2 = (p1 & 0xFFFF) * (p2 >> 16);
  answer_3 = (p1 >> 16) * (p2 & 0xFFFF);
  answer_4 = (p1 >> 16) * (p2 >> 16);

  answer = ((uint64_t)answer_4 << 32) + answer_1;
  answer += (uint64_t)answer_2 << 16;
  answer += (uint64_t)answer_3 << 16;

#ifdef DEBUG
  fprintf (stderr, "multiply_16 called with p1 = %d, p2 = %d\n", p1, p2);
  fprintf (stderr, "p1 = %X, p2 = %X, a1 = %X, a2 = %X, a3 = %X, a4 = %X, ah = %X, al = %X\n", p1, p2, answer_1, answer_2, answer_3, answer_4, answer[0], answer[1]);
#endif
  return (answer);
}

/* --------------------------------------------------------------------------------- */

int64_t _ll_mulss (int32_t a, int32_t b)
{
  unsigned int negatives;
  int64_t answer;

  // printf ("_ll_mulss %d %d", a,b);

  negatives = 0;
  if (a < 0)
  {
    a = -a;
    negatives++;
  }
  if (b < 0)
  {
    b = -b;
    negatives++;
  }

  answer = multiply_64 (a,b);
  if (negatives & 1)
    answer = -answer;

  // printf (" = %lld\n", answer);
  return (answer);
}

/* --------------------------------------------------------------------------------- */

static uint64_t div64_32 (uint64_t rem, uint32_t base)
{
  uint64_t b;
  uint64_t d;
  uint64_t res;
  uint32_t high;

  if (base == 0)
    return (~0);

  b = base;
  d = 1;
  high = (uint32_t) (rem >> 32);
  res = 0;

  /* Reduce the thing a bit first */
  if (high >= base)
  {
    high /= base;
    res = (uint64_t) high << 32;
    rem -= (uint64_t) (high*base) << 32;
  }

  while ((int64_t)b > 0 && b < rem)
  {
    b = b+b;
    d = d+d;
  }

  do
  {
    if (rem >= b)
    {
      rem -= b;
      res += d;
    }
    b >>= 1;
    d >>= 1;
  } while (d);

  return (res);
}

/* --------------------------------------------------------------------------------- */

int64_t _ll_sdiv (int64_t a, int64_t b)
{
  unsigned int negatives;
  int64_t answer;

  // printf ("_ll_sdiv %lld %d\n", a,b);

  negatives = 0;
  if (a < 0)
  {
    a = -a;
    negatives++;
  }
  if (b < 0)
  {
    b = -b;
    negatives++;
  }

  if ((uint64_t) b > 0xFFFFFFFF)
  {
    /* Bodge it to make it fit! */
    do
    {
      a = (uint64_t) a >> 1;
      b = (uint64_t) b >> 1;
    } while ((uint64_t) b > 0xFFFFFFFF);
  }

  answer = div64_32 (a, (uint32_t) b);

  if (negatives & 1)
    answer = -answer;

  return (answer);
}

/* --------------------------------------------------------------------------------- */

int64_t _ll_srdv (int64_t b, int64_t a)
{
  return (_ll_sdiv (a, b));
}

/* --------------------------------------------------------------------------------- */

uint64_t _ll_udiv (uint64_t a, uint64_t b)
{
  uint64_t answer;

  // printf ("_ll_udiv %lld %d\n", a,b);

  if (b > 0xFFFFFFFF)
  {
    /* Bodge it to make it fit! */
    do
    {
      a = a >> 1;
      b = b >> 1;
    } while (b > 0xFFFFFFFF);
  }

  answer = div64_32 (a, (uint32_t) b);
  return (answer);
}

/* --------------------------------------------------------------------------------- */

#ifdef MATHS_64_TESTING

/* --------------------------------------------------------------------------------- */

int main (int argc, char * argv [])
{
  int64_t ans;

  ans = _ll_sdiv (100000000, 8);
  printf ("ans = %lld\n", ans);
}

#endif

