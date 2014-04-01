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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif


#ifdef __GNUG__
#pragma implementation "m_swap.h"
#endif
#include "m_swap.h"


long SwapLONG (long ln)
{
  unsigned long l;
  unsigned long rc;

  l = ln;
  rc = (l & 0xFF);
  rc <<= 8;
  l >>= 8;
  rc |= (l & 0xFF);
  rc <<= 8;
  l >>= 8;
  rc |= (l & 0xFF);
  rc <<= 8;
  l >>= 8;
  rc |= (l & 0xFF);
  return ((long)rc);
}

/* --------------------------------------------------------------------------------- */

short SwapSHORT (short ls)
{
  unsigned int l;
  unsigned int rc;

  l = ls;
  rc = (l & 0xFF);
  rc <<= 8;
  l >>= 8;
  rc |= (l & 0xFF);
  return ((short)rc);
}

/* --------------------------------------------------------------------------------- */
