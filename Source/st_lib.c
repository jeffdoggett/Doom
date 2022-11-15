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
//	The status bar widget code.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: st_lib.c,v 1.4 1997/02/03 16:47:56 b1 Exp $";
#endif

#include "includes.h"

#include "doomdef.h"

// in AM_map.c
extern boolean		automapactive;


void STlib_drawPatch
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch )
{
  y -= ((SHORT(patch->topoffset) * sbarscale) >> FRACBITS);
  x -= ((SHORT(patch->leftoffset) * sbarscale) >> FRACBITS);
  V_DrawPatchScaleFlip (x, y, scrn, patch, sbarscale, sbarscale, 0);
}


//
// Hack display negative frags.
//  Loads and store the stminus lump.
//
patch_t*		sttminus;

void STlib_init(void)
{
  sttminus = (patch_t *) W_CacheLumpName("STTMINUS", PU_STATIC);
}


// ?
void
STlib_initNum
( st_number_t*		n,
  int			x,
  int			y,
  patch_t**		pl,
  int*			num,
  boolean*		on,
  int			width )
{
    n->x	= x;
    n->y	= y;
    n->oldnum	= 0;
    n->width	= width;
    n->num	= num;
    n->on	= on;
    n->p	= pl;
}


//
// A fairly efficient way to draw a number
//  based on differences from the old number.
// Note: worth the trouble?
//
void
STlib_drawNum
( st_number_t*	n,
  boolean	refresh )
{
    int		numdigits = n->width;
    int		num = *n->num;

    int		w;
    int		h;
    int		x = n->x;
    int		y;

    int		neg;

    w = ((SHORT(n->p[0]->width)) * sbarscale) >> FRACBITS;
    h = ((SHORT(n->p[0]->height)) * sbarscale) >> FRACBITS;

    n->oldnum = *n->num;

    neg = num < 0;

    if (neg)
    {
	if (numdigits == 2 && num < -9)
	    num = -9;
	else if (numdigits == 3 && num < -99)
	    num = -99;

	num = -num;
    }

    // clear the area
    x = n->x - numdigits*w;
    y = n->y - ST_Y;
    if ((y < 0) || (y > SCREENHEIGHT))
	I_Error("drawNum: y < 0 (%d < %d (%d %d))\n", n->y, ST_Y, SCREENHEIGHT, ST_HEIGHT);
    if ((x < 0) || (y > SCREENWIDTH))
	I_Error("drawNum: x < 0 (%d (%d %d))\n", n->x, numdigits, w);

    V_CopyRect(x, y, ST_BG, w*numdigits, h, x, n->y, ST_FG);

    // if non-number, do not draw it
    if (num == 1994)
	return;

    x = n->x;

    // draw the new number
    do
    {
	x -= w;
	STlib_drawPatch (x, n->y, ST_FG, n->p [num % 10]);
	num /= 10;
    } while (num && --numdigits);

    // draw a minus sign if necessary
    if (neg)
      STlib_drawPatch((x - 8), n->y, ST_FG, sttminus);
}


//
void
STlib_updateNum
( st_number_t*		n,
  boolean		refresh )
{
    if (*n->on) STlib_drawNum(n, refresh);
}


//
void
STlib_initPercent
( st_percent_t*		p,
  int			x,
  int			y,
  patch_t**		pl,
  int*			num,
  boolean*		on,
  patch_t*		percent )
{
    STlib_initNum(&p->n, x, y, pl, num, on, 3);
    p->p = percent;
}




void
STlib_updatePercent
( st_percent_t*		per,
  boolean		refresh )
{
    if (refresh && *per->n.on)
	STlib_drawPatch(per->n.x, per->n.y, ST_FG, per->p);

    STlib_updateNum(&per->n, refresh);
}



void
STlib_initMultIcon
( st_multicon_t*	i,
  int			x,
  int			y,
  patch_t**		il,
  int*			inum,
  boolean*		on )
{
    i->x	= x;
    i->y	= y;
    i->oldinum 	= -1;
    i->inum	= inum;
    i->on	= on;
    i->p	= il;
}



void
STlib_updateMultIcon
( st_multicon_t*	mi,
  boolean		refresh )
{
    int		w;
    int		h;
    int		x;
    int		y;
    int		y1;

    if (*mi->on
	&& (mi->oldinum != *mi->inum || refresh)
	&& (*mi->inum!=-1))
    {
	if (mi->oldinum != -1)
	{
	    x = mi->x - ((SHORT(mi->p[mi->oldinum]->leftoffset) * sbarscale) >> FRACBITS);
	    y = mi->y - ((SHORT(mi->p[mi->oldinum]->topoffset) * sbarscale) >> FRACBITS);
	    w = ((SHORT(mi->p[mi->oldinum]->width) * sbarscale) >> FRACBITS);
	    h = ((SHORT(mi->p[mi->oldinum]->height) * sbarscale) >> FRACBITS);

	    y1 = y - ST_Y;
	    if (y1 < 0)
		I_Error("updateMultIcon: y1 - ST_Y < 0");

	    V_CopyRect(x, y1, ST_BG, w, h, x, y, ST_FG);
	}
	STlib_drawPatch(mi->x, mi->y, ST_FG, mi->p[*mi->inum]);
	mi->oldinum = *mi->inum;
    }
}



void
STlib_initBinIcon
( st_binicon_t*		b,
  int			x,
  int			y,
  patch_t*		i,
  boolean*		val,
  boolean*		on )
{
    b->x	= x;
    b->y	= y;
    b->oldval	= 0;
    b->val	= val;
    b->on	= on;
    b->p	= i;
}



void
STlib_updateBinIcon
( st_binicon_t*		bi,
  boolean		refresh )
{
    int		x;
    int		y;
    int		y1;
    int		w;
    int		h;
    patch_t*	patch;

    if (*bi->on
	&& (bi->oldval != *bi->val || refresh))
    {
	patch = bi->p;
	x = bi->x - ((SHORT(patch->leftoffset) * sbarscale) >> FRACBITS);
	y = bi->y - ((SHORT(patch->topoffset) * sbarscale) >> FRACBITS);
	w = ((SHORT(patch->width) * sbarscale) >> FRACBITS);
	h = ((SHORT(patch->height) * sbarscale) >> FRACBITS);

	y1 = y - ST_Y;
	if (y1 < 0)
	    I_Error("updateBinIcon: y1 - ST_Y < 0");

	if (*bi->val)
	  STlib_drawPatch(bi->x, bi->y, ST_FG, patch);
	else
	  V_CopyRect(x, y1, ST_BG, w, h, x, y, ST_FG);

	bi->oldval = *bi->val;
    }
}

