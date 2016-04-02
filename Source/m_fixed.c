// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Fixed point implementation.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: m_fixed.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif

#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"

/* --------------------------------------------------------------------------------- */

int ABS (int a)
{
  int b = a >> 31;
  return ((a ^ b) - b);
}

int MAX (int a, int b)
{
  b = a - b;
  return (a - (b & (b >> 31)));
}

int MIN (int a, int b)
{
  a = a - b;
  return (b + (a & (a >> 31)));
}

int BETWEEN(int a, int b, int c)
{
    return MAX(a, MIN(b, c));
}

int SIGN (int a)
{
  return (1 | (a >> 31));
}

/* --------------------------------------------------------------------------------- */

fixed_t
FixedMul
( fixed_t	r0,
  fixed_t	r1)
{
  fixed_t r2,r3;

  r2 = r0 >> 16;
  r0 = r0 & ~(r2 << 16);
  r2 = r1 * r2;
  r3 = r1 >> 16;
  r1 = r1 & ~(r3 << 16);
  r2 = r2 + (r3 * r0);
  r0 = r1 * r0;
  r0 = r2 + (((unsigned int) r0) >> 16);

  return (r0);
}
#if 0
	MOV	R2, R0, ASR #16
	BIC	R0, R0, R2, LSL #16
	MUL	R2, R1, R2		; A.INT * B
	MOV	R3, R1, ASR #16
	BIC	R1, R1, R3, LSL #16
	MLA	R2, R3, R0, R2
	MUL	R0, R1, R0
	ADD	R0, R2, R0, LSR #16
	MOV	PC, LR
#endif

/* --------------------------------------------------------------------------------- */

fixed_t
FixedDiv
( fixed_t	a,
  fixed_t	b )
{
  fixed_t r2,r3;
  unsigned int r0,r1;

  r3 = a ^ b;
  if (a < 0)
  {
    r0 = -a;
  }
  else
  {
    r0 = a;
  }

  if (b < 0)
  {
    r1 = -b;
  }
  else
  {
    r1 = b;
  }

  if ((r0 >> 14) >= r1)
  {
    return (r3 < 0) ? MININT : MAXINT;
  }

  r2 = 0;

  if (r0 >= (r1 << 0))
  {
    if (r0 >= (r1 << 1))
    {
      if (r0 >= (r1 << 2))
      {
        if (r0 >= (r1 << 3))
        {
          if (r0 >= (r1 << 4))
          {
            if (r0 >= (r1 << 5))
            {
              if (r0 >= (r1 << 6))
              {
                if (r0 >= (r1 << 7))
                {
                  if (r0 >= (r1 << 8))
                  {
                    if (r0 >= (r1 << 9))
                    {
                      if (r0 >= (r1 << 10))
                      {
                        if (r0 >= (r1 << 11))
                        {
                          if (r0 >= (r1 << 12))
                          {
                            if (r0 >= (r1 << 13))
                            {
                              if (r0 >= (r1 << 14))
                              {
                                r0 -= (r1 << 15);
                                r2 |= (1U << (15+16));
                                if (r0 >= (r1 << 14))
                                {
                                  r0 -= (r1 << 14);
                                  r2 |= (1 << (14+16));
                                }
                              }
			      if (r0 >= (r1 << 13))
			      {
				r0 -= (r1 << 13);
				r2 |= (1 << (13+16));
			      }
                            }
			    if (r0 >= (r1 << 12))
			    {
			      r0 -= (r1 << 12);
			      r2 |= (1 << (12+16));
			    }
                          }
			  if (r0 >= (r1 << 11))
			  {
			    r0 -= (r1 << 11);
			    r2 |= (1 << (11+16));
			  }
                        }
			if (r0 >= (r1 << 10))
			{
			  r0 -= (r1 << 10);
			  r2 |= (1 << (10+16));
			}
                      }
		      if (r0 >= (r1 << 9))
		      {
			r0 -= (r1 << 9);
			r2 |= (1 << (9+16));
		      }
                    }
		    if (r0 >= (r1 << 8))
		    {
		      r0 -= (r1 << 8);
		      r2 |= (1 << (8+16));
		    }
                  }
		  if (r0 >= (r1 << 7))
		  {
		    r0 -= (r1 << 7);
		    r2 |= (1 << (7+16));
		  }
                }
		if (r0 >= (r1 << 6))
		{
		  r0 -= (r1 << 6);
		  r2 |= (1 << (6+16));
		}
              }
	      if (r0 >= (r1 << 5))
	      {
		r0 -= (r1 << 5);
		r2 |= (1 << (5+16));
	      }
            }
	    if (r0 >= (r1 << 4))
	    {
	      r0 -= (r1 << 4);
	      r2 |= (1 << (4+16));
	    }
          }
	  if (r0 >= (r1 << 3))
	  {
	    r0 -= (r1 << 3);
	    r2 |= (1 << (3+16));
	  }
        }
	if (r0 >= (r1 << 2))
	{
	  r0 -= (r1 << 2);
	  r2 |= (1 << (2+16));
	}
      }
      if (r0 >= (r1 << 1))
      {
	r0 -= (r1 << 1);
	r2 |= (1 << (1+16));
      }
    }
    if (r0 >= (r1 << 0))
    {
      r0 -= (r1 << 0);
      r2 |= (1 << (0+16));
    }
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 15);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 14);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 13);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 12);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 11);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 10);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 9);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 8);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 7);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 6);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 5);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 4);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 3);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 2);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 1);
  }

  r0 <<= 1;
  if (r0 >= r1)
  {
    r0 -= r1;
    r2 |= (1 << 0);
  }

  if (r3 < 0)
  {
    r2 = -r2;
  }

  return (r2);
}

#if 0
|FixedDiv|:
	movs	r2, r0			; first to the range check (easier on positive data)
	rsblt	r2, r2, #0
	movs	r3, r1
	rsblt	r3, r3, #0
	cmp	r3, r2, lsr #14		; do the divide if b > (a>>14)
	bhi	|FixedDiv2|
	eors	r2, r0, r1
	mvnge	r0, #0x80000000
	movlt	r0, #0x80000000
	mov	pc, lr

|FixedDiv2|:
	eor	r3, r0, r1
	cmp	r0, #0
	rsblt	r0, r0, #0
	cmp	r1, #0
	rsblt	r1, r1, #0
	mov	r2, #0
	cmp	r0, r1, lsl #0x0
	bcc	|DivMark0|
	cmp	r0, r1, lsl #0x1
	bcc	|DivMark1|
	cmp	r0, r1, lsl #0x2
	bcc	|DivMark2|
	cmp	r0, r1, lsl #0x3
	bcc	|DivMark3|
	cmp	r0, r1, lsl #0x4
	bcc	|DivMark4|
	cmp	r0, r1, lsl #0x5
	bcc	|DivMark5|
	cmp	r0, r1, lsl #0x6
	bcc	|DivMark6|
	cmp	r0, r1, lsl #0x7
	bcc	|DivMark7|
	cmp	r0, r1, lsl #0x8
	bcc	|DivMark8|
	cmp	r0, r1, lsl #0x9
	bcc	|DivMark9|
	cmp	r0, r1, lsl #0xA
	bcc	|DivMark10|
	cmp	r0, r1, lsl #0xB
	bcc	|DivMark11|
	cmp	r0, r1, lsl #0xC
	bcc	|DivMark12|
	cmp	r0, r1, lsl #0xD
	bcc	|DivMark13|
	cmp	r0, r1, lsl #0xE
	bcc	|DivMark14|
	subcs	r0, r0, r1, lsl #15
	orrcs	r2, r2, #0x80000000
	cmp	r0, r1, lsl #0xE
	subcs	r0, r0, r1, lsl #0xE
	orrcs	r2, r2, #0x40000000
|DivMark14|:
	cmp	r0, r1, lsl #0xD
	subcs	r0, r0, r1, lsl #0xD
	orrcs	r2, r2, #0x20000000
|DivMark13|:
	cmp	r0, r1, lsl #0xC
	subcs	r0, r0, r1, lsl #0xC
	orrcs	r2, r2, #0x10000000
|DivMark12|:
	cmp	r0, r1, lsl #0xB
	subcs	r0, r0, r1, lsl #0xB
	orrcs	r2, r2, #0x8000000
|DivMark11|:
	cmp	r0, r1, lsl #0xA
	subcs	r0, r0, r1, lsl #0xA
	orrcs	r2, r2, #0x4000000
|DivMark10|:
	cmp	r0, r1, lsl #0x9
	subcs	r0, r0, r1, lsl #0x9
	orrcs	r2, r2, #0x2000000
|DivMark9|:
	cmp	r0, r1, lsl #0x8
	subcs	r0, r0, r1, lsl #0x8
	orrcs	r2, r2, #0x1000000
|DivMark8|:
	cmp	r0, r1, lsl #0x7
	subcs	r0, r0, r1, lsl #0x7
	orrcs	r2, r2, #0x800000
|DivMark7|:
	cmp	r0, r1, lsl #0x6
	subcs	r0, r0, r1, lsl #0x6
	orrcs	r2, r2, #0x400000
|DivMark6|:
	cmp	r0, r1, lsl #0x5
	subcs	r0, r0, r1, lsl #0x5
	orrcs	r2, r2, #0x200000
|DivMark5|:
	cmp	r0, r1, lsl #0x4
	subcs	r0, r0, r1, lsl #0x4
	orrcs	r2, r2, #0x100000
|DivMark4|:
	cmp	r0, r1, lsl #0x3
	subcs	r0, r0, r1, lsl #0x3
	orrcs	r2, r2, #0x80000
|DivMark3|:
	cmp	r0, r1, lsl #0x2
	subcs	r0, r0, r1, lsl #0x2
	orrcs	r2, r2, #0x40000
|DivMark2|:
	cmp	r0, r1, lsl #0x1
	subcs	r0, r0, r1, lsl #0x1
	orrcs	r2, r2, #0x20000
|DivMark1|:
	cmp	r0, r1, lsl #0x0
	subcs	r0, r0, r1, lsl #0x0
	orrcs	r2, r2, #0x10000
|DivMark0|:
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x8000
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x4000
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x2000
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x1000
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x800
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x400
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x200
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x100
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x80
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x40
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x20
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x10
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x8
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x4
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x2
	mov	r0, r0, lsl #1
	cmp	r0, r1
	subcs	r0, r0, r1
	orrcs	r2, r2, #0x1

	tst	r3, #0x80000000
	rsbne	r2, r2, #0
	mov	r0, r2
	mov	pc, lr

/* --------------------------------------------------------------------------------- */

/* Performs a / b * FRACUNIT */

unsigned int FixedDiv3 (unsigned int a, unsigned int b)
{
  unsigned int c,d;

  if ((a == 0) || (b == 0))  return (0);

  c = 16;

  if (a < 0x10000)
  {
    a <<= 16;
    c -= 16;
  }

  if ((a & 0xFF000000) == 0)
  {
    a <<= 8;
    c -= 8;
  }

  if ((a & 0xF0000000) == 0)
  {
    a <<= 4;
    c -= 4;
  }

  if ((a & 0xC0000000) == 0)
  {
    a <<= 2;
    c -= 2;
  }

  if ((a & 0x80000000) == 0)
  {
    a <<= 1;
    c -= 1;
  }

  if (b < 0x10000)
  {
    b <<= 16;
    c += 16;
  }

  if ((b & 0xFF000000) == 0)
  {
    b <<= 8;
    c += 8;
  }

  if ((b & 0xF0000000) == 0)
  {
    b <<= 4;
    c += 4;
  }

  if ((b & 0xC0000000) == 0)
  {
    b <<= 2;
    c += 2;
  }

  if ((b & 0x80000000) == 0)
  {
    b <<= 1;
    c += 1;
  }

  if (c >= 32)  return (0);

  d = 1 << c;

  c = a;
  a = 0;
  b &= ~1;

  do
  {
    if (c >= b)
    {
      c = c - b;
      a = a | d;
    }
    if (c >= (b >> 1))
    {
      c = c - (b >> 1);
      a = a | (d >> 1);
    }
    c <<= 1;
    d >>= 1;
  } while (d);
  return (a);
}

#endif

#ifdef FIXED_D_TESTING

/* --------------------------------------------------------------------------------- */

int main (int argc, char * argv [])
{
  fixed_t ans;

  ans = FixedDiv (0xFFFF0000, 0x00080000);
  printf ("ans = %08X\n", ans);
}

#endif

