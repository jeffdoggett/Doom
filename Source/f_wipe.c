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
//	Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: f_wipe.c,v 1.2 1997/02/03 22:45:09 b1 Exp $";
#endif

#include "includes.h"

//-----------------------------------------------------------------------------
//
//		       SCREEN WIPE PACKAGE
//

static unsigned int * ptrs = NULL;

#define wipe_scr_start	screens[2]
#define wipe_scr_end	screens[3]

//-----------------------------------------------------------------------------

static void wipe_Init (void)
{
  if (ptrs == NULL)
  {
    ptrs = Z_Malloc (SCREENWIDTH*sizeof(int), PU_STATIC, NULL);
    memset (ptrs, 0, SCREENWIDTH*sizeof(int));
  }
}

//-----------------------------------------------------------------------------

static void wipe_Finish (void)
{
  if (ptrs)
  {
    Z_Free (ptrs);
    ptrs = NULL;
  }
}

//-----------------------------------------------------------------------------

int wipe_StartScreen (void)
{
  I_ReadScreen (wipe_scr_start);
  return 0;
}

//-----------------------------------------------------------------------------

int wipe_EndScreen (void)
{
  I_ReadScreen (wipe_scr_end);
  V_DrawBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_start); // restore start scr.
  wipe_Init ();
  return 0;
}

//-----------------------------------------------------------------------------

static void wipe_CopyColumn (unsigned int x, unsigned int y)
{
  byte * dest;
  byte * source;
  unsigned int qty;

  dest = screens[0] + x;

  if (y)
  {
    qty = y;
    source = wipe_scr_end + x;
    do
    {
      *dest = *source;
      dest += SCREENWIDTH;
      source += SCREENWIDTH;
    } while (--qty);
  }

  qty = SCREENHEIGHT - y;
  if (qty)
  {
    source = wipe_scr_start + x;
    do
    {
      *dest = *source;
      dest += SCREENWIDTH;
      source += SCREENWIDTH;
    } while (--qty);
  }
}

//-----------------------------------------------------------------------------

boolean wipe_ScreenWipe (int ticks)
{
  unsigned int q;
  unsigned int r;
  unsigned int x;
  unsigned int y;
  unsigned int qty;
  unsigned int * p;

  p = ptrs;
  qty = 0;
  x = 0;
  q = 10;
  do
  {
    y = *p;
    if (y < SCREENHEIGHT)
    {
      if ((x & 3) == 0)
      {
	if (rand() & 1)
	{
	  if (q < 14)
	    q += 1;
	}
	else
	{
	  if (q > 6)
	    q -= 1;
	}
      }
      r = q * ticks;
      if ((y + r) >= SCREENHEIGHT)
	r = SCREENHEIGHT - y;
      wipe_CopyColumn (x, *p=y+r);
      qty++;
    }
    p++;
  } while (++x < SCREENWIDTH);

  if (qty == 0)
  {
    wipe_Finish ();
    return (true);
  }

  return (false);
}

//-----------------------------------------------------------------------------
