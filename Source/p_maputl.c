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
//	Movement/collision utility functions,
//	as used by function in p_map.c.
//	BLOCKMAP Iterator functions,
//	and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_maputl.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";
#endif

#include "includes.h"

extern void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y);

//-----------------------------------------------------------------------------
//
// P_ApproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t
P_ApproxDistance
( fixed_t	dx,
  fixed_t	dy )
{
    dx = ABS(dx);
    dy = ABS(dy);
    if (dx < dy)
	return dx+dy-(dx>>1);
    return dx+dy-(dy>>1);
}

//-----------------------------------------------------------------------------
//
// P_PointOnLineSide
// Returns 0 or 1
//
int
P_PointOnLineSide
( fixed_t	x,
  fixed_t	y,
  line_t*	line )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    if (!line->dx)
    {
	if (x <= line->v1->x)
	    return line->dy > 0;

	return line->dy < 0;
    }
    if (!line->dy)
    {
	if (y <= line->v1->y)
	    return line->dx < 0;

	return line->dx > 0;
    }

    dx = (x - line->v1->x);
    dy = (y - line->v1->y);

    left = FixedMul ( line->dy>>FRACBITS , dx );
    right = FixedMul ( dy , line->dx>>FRACBITS );

    if (right < left)
	return 0;		// front side
    return 1;			// back side
}

//-----------------------------------------------------------------------------
//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int
P_BoxOnLineSide
( fixed_t*	tmbox,
  line_t*	ld )
{
    int		p1;
    int		p2;

    switch (ld->slopetype)
    {
      case ST_HORIZONTAL:
	p1 = tmbox[BOXTOP] > ld->v1->y;
	p2 = tmbox[BOXBOTTOM] > ld->v1->y;
	if (ld->dx < 0)
	{
	    p1 ^= 1;
	    p2 ^= 1;
	}
	break;

      case ST_VERTICAL:
	p1 = tmbox[BOXRIGHT] < ld->v1->x;
	p2 = tmbox[BOXLEFT] < ld->v1->x;
	if (ld->dy < 0)
	{
	    p1 ^= 1;
	    p2 ^= 1;
	}
	break;

      case ST_POSITIVE:
	p1 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXTOP], ld);
	p2 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
	break;

      case ST_NEGATIVE:
	p1 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
	p2 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
	break;
    }

    if (p1 == p2)
	return p1;
    return -1;
}

//-----------------------------------------------------------------------------
//
// P_PointOnDivlineSide
// Returns 0 or 1.
//
int
P_PointOnDivlineSide
( fixed_t	x,
  fixed_t	y,
  divline_t*	line )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    if (!line->dx)
    {
	if (x <= line->x)
	    return line->dy > 0;

	return line->dy < 0;
    }
    if (!line->dy)
    {
	if (y <= line->y)
	    return line->dx < 0;

	return line->dx > 0;
    }

    dx = (x - line->x);
    dy = (y - line->y);

    // try to quickly decide by looking at sign bits
    if ( (line->dy ^ line->dx ^ dx ^ dy)&0x80000000 )
    {
	if ( (line->dy ^ dx) & 0x80000000 )
	    return 1;		// (left is negative)
	return 0;
    }

    left = FixedMul ( line->dy>>8, dx>>8 );
    right = FixedMul ( dy>>8 , line->dx>>8 );

    if (right < left)
	return 0;		// front side
    return 1;			// back side
}

//-----------------------------------------------------------------------------
//
// P_MakeDivline
//
void
P_MakeDivline
( line_t*	li,
  divline_t*	dl )
{
    dl->x = li->v1->x;
    dl->y = li->v1->y;
    dl->dx = li->dx;
    dl->dy = li->dy;
}

//-----------------------------------------------------------------------------
//
// P_InterceptVector
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings
// and addlines traversers.
//
fixed_t
P_InterceptVector
( divline_t*	v2,
  divline_t*	v1 )
{
#if 1
    fixed_t	frac;
    fixed_t	num;
    fixed_t	den;

    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

    if (den == 0)
	return 0;
    //	I_Error ("P_InterceptVector: parallel");

    num =
	FixedMul ( (v1->x - v2->x)>>8 ,v1->dy )
	+FixedMul ( (v2->y - v1->y)>>8, v1->dx );

    frac = FixedDiv (num , den);

    return frac;
#else	// UNUSED, float debug.
    float	frac;
    float	num;
    float	den;
    float	v1x;
    float	v1y;
    float	v1dx;
    float	v1dy;
    float	v2x;
    float	v2y;
    float	v2dx;
    float	v2dy;

    v1x = (float)v1->x/FRACUNIT;
    v1y = (float)v1->y/FRACUNIT;
    v1dx = (float)v1->dx/FRACUNIT;
    v1dy = (float)v1->dy/FRACUNIT;
    v2x = (float)v2->x/FRACUNIT;
    v2y = (float)v2->y/FRACUNIT;
    v2dx = (float)v2->dx/FRACUNIT;
    v2dy = (float)v2->dy/FRACUNIT;

    den = v1dy*v2dx - v1dx*v2dy;

    if (den == 0)
	return 0;	// parallel

    num = (v1x - v2x)*v1dy + (v2y - v1y)*v1dx;
    frac = num / den;

    return frac*FRACUNIT;
#endif
}

//-----------------------------------------------------------------------------
//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t	lowfloor;


void P_LineOpening (line_t* linedef)
{
    sector_t*	front;
    sector_t*	back;

    if (linedef->sidenum[1] == (dushort_t) -1)
    {
	// single sided line
	openrange = 0;
	return;
    }

    front = linedef->frontsector;
    back = linedef->backsector;

    if (front->ceilingheight < back->ceilingheight)
	opentop = front->ceilingheight;
    else
	opentop = back->ceilingheight;

    if (front->floorheight > back->floorheight)
    {
	openbottom = front->floorheight;
	lowfloor = back->floorheight;
    }
    else
    {
	openbottom = back->floorheight;
	lowfloor = front->floorheight;
    }

    openrange = opentop - openbottom;
}

//-----------------------------------------------------------------------------
//
// THING POSITION SETTING
//


//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists ot things inside
// these structures need to be updated.
//
void P_UnsetThingPosition (mobj_t* thing)
{
    int		blockx;
    int		blocky;

    if ( ! (thing->flags & MF_NOSECTOR) )
    {
	// inert things don't need to be in blockmap?
	// unlink from subsector
	if (thing->snext)
	    thing->snext->sprev = thing->sprev;

	if (thing->sprev)
	    thing->sprev->snext = thing->snext;
	else
	    thing->subsector->sector->thinglist = thing->snext;

	// phares 3/14/98
	//
	// Save the sector list pointed to by touching_sectorlist.
	// In P_SetThingPosition, we'll keep any nodes that represent
	// sectors the Thing still touches. We'll add new ones then, and
	// delete any nodes for sectors the Thing has vacated. Then we'll
	// put it back into touching_sectorlist. It's done this way to
	// avoid a lot of deleting/creating for nodes, when most of the
	// time you just get back what you deleted anyway.
	//
	// If this Thing is being removed entirely, then the calling
	// routine will clear out the nodes in sector_list.
	sector_list = thing->touching_sectorlist;
	thing->touching_sectorlist = NULL; // to be restored by P_SetThingPosition
    }

    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
	// inert things don't need to be in blockmap
	// unlink from block map
	if (thing->bnext)
	    thing->bnext->bprev = thing->bprev;

	if (thing->bprev)
	    thing->bprev->bnext = thing->bnext;
	else
	{
	    blockx = P_GetSafeBlockX(thing->x - bmaporgx);
	    blocky = P_GetSafeBlockY(thing->y - bmaporgy);

	    if (blockx>=0 && blockx < bmapwidth
		&& blocky>=0 && blocky <bmapheight)
	    {
		blocklinks[blocky*bmapwidth+blockx] = thing->bnext;
	    }
	}
    }
}

//-----------------------------------------------------------------------------
//
// P_SetThingPosition
// Links a thing into both a block and a subsector
// based on it's x y.
// Sets thing->subsector properly
//
void
P_SetThingPosition (mobj_t* thing)
{
    subsector_t*	ss;
    sector_t*		sec;
    int			blockx;
    int			blocky;
    mobj_t**		link;


    // link into subsector
    ss = R_PointInSubsector (thing->x,thing->y);
    thing->subsector = ss;

    if ( ! (thing->flags & MF_NOSECTOR) )
    {
	// invisible things don't go into the sector links
	sec = ss->sector;

	thing->sprev = NULL;
	thing->snext = sec->thinglist;

	if (sec->thinglist)
	    sec->thinglist->sprev = thing;

	sec->thinglist = thing;

	// phares 3/16/98
	//
	// If sector_list isn't NULL, it has a collection of sector
	// nodes that were just removed from this Thing.

	// Collect the sectors the object will live in by looking at
	// the existing sector_list and adding new nodes and deleting
	// obsolete ones.

	// When a node is deleted, its sector links (the links starting
	// at sector_t->touching_thinglist) are broken. When a node is
	// added, new sector links are created.
	P_CreateSecNodeList(thing, thing->x, thing->y);
	thing->touching_sectorlist = sector_list; // Attach to Thing's mobj_t
	sector_list = NULL; // clear for next time
    }


    // link into blockmap
    if ( ! (thing->flags & MF_NOBLOCKMAP) )
    {
	// inert things don't need to be in blockmap
	blockx = (thing->x - bmaporgx)>>MAPBLOCKSHIFT;
	blocky = (thing->y - bmaporgy)>>MAPBLOCKSHIFT;

	if (blockx>=0
	    && blockx < bmapwidth
	    && blocky>=0
	    && blocky < bmapheight)
	{
	    link = &blocklinks[blocky*bmapwidth+blockx];
	    thing->bprev = NULL;
	    thing->bnext = *link;
	    if (*link)
		(*link)->bprev = thing;

	    *link = thing;
	}
	else
	{
	    // thing is off the map
	    thing->bnext = thing->bprev = NULL;
	}
    }
}


//-----------------------------------------------------------------------------
//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
boolean P_BlockLinesIterator (int x, int y, boolean(*func)(line_t*))
{
  if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
    return true;
  else
  {
    uint32_t * blockmapindex = &blockmaphead[4];
    int offset = blockmapindex[y * bmapwidth + x];
    const uint32_t *list;

    for (list = &blockmaphead[offset]; *list != (uint32_t)(-1); list++)
    {
      line_t *ld = &lines[*list];

      if (ld->validcount == validcount)
	continue; // line has already been checked

      ld->validcount = validcount;

      if (!func(ld))
	return false;
    }
    return true; // everything was checked
  }
}

//-----------------------------------------------------------------------------
//
// P_BlockThingsIterator
//
boolean
P_BlockThingsIterator
( int			x,
  int			y,
  boolean(*func)(mobj_t*) )
{
    if (x < 0 || y < 0 || x >= bmapwidth || y >= bmapheight)
        return true;

    for (mobj_t *mobj = blocklinks[y * bmapwidth + x]; mobj; mobj = mobj->bnext)
        if (!func(mobj))
            return false;

    if (func == PIT_RadiusAttack)	// Don't do for explosions
        return true;

    // Blockmap bug fix by Terry Hearst

    // (-1, -1)
    if (x > 0 && y > 0)
        for (mobj_t *mobj = blocklinks[(y - 1) * bmapwidth + x - 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x + mobj->radius - bmaporgx) >> MAPBLOCKSHIFT && y == (mobj->y + mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (0, -1)
    if (y > 0)
        for (mobj_t *mobj = blocklinks[(y - 1) * bmapwidth + x]; mobj; mobj = mobj->bnext)
            if (y == (mobj->y + mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (1, -1)
    if (x < bmapwidth - 1 && y > 0)
        for (mobj_t *mobj = blocklinks[(y - 1) * bmapwidth + x + 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x - mobj->radius - bmaporgx) >> MAPBLOCKSHIFT && y == (mobj->y + mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (1, 0)
    if (x < bmapwidth - 1)
        for (mobj_t *mobj = blocklinks[y * bmapwidth + x + 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x - mobj->radius - bmaporgx) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (1, 1)
    if (x < bmapwidth - 1 && y < bmapheight - 1)
        for (mobj_t *mobj = blocklinks[(y + 1) * bmapwidth + x + 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x - mobj->radius - bmaporgx) >> MAPBLOCKSHIFT && y == (mobj->y - mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (0, 1)
    if (y < bmapheight - 1)
        for (mobj_t *mobj = blocklinks[(y + 1) * bmapwidth + x]; mobj; mobj = mobj->bnext)
            if (y == (mobj->y - mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (-1, 1)
    if (x > 0 && y < bmapheight - 1)
        for (mobj_t *mobj = blocklinks[(y + 1) * bmapwidth + x - 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x + mobj->radius - bmaporgx) >> MAPBLOCKSHIFT && y == (mobj->y - mobj->radius - bmaporgy) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    // (-1, 0)
    if (x > 0)
        for (mobj_t *mobj = blocklinks[y * bmapwidth + x - 1]; mobj; mobj = mobj->bnext)
            if (x == (mobj->x + mobj->radius - bmaporgx) >> MAPBLOCKSHIFT)
                if (!func(mobj))
                    return false;

    return true;
}

//-----------------------------------------------------------------------------
//
// INTERCEPT ROUTINES
//

// 1/11/98 killough: Intercept limit removed
static intercept_t *intercepts;
static intercept_t *intercept_p;
static size_t num_intercepts;
divline_t trace;
static boolean earlyout;
static boolean showintercepts;


void P_Init_Intercepts (void)
{
  num_intercepts = 128;
  intercept_p = intercepts = (intercept_t *) malloc (sizeof(*intercepts) * num_intercepts);
  if (intercepts == NULL)
    I_Error ("Out of memory\n");
  showintercepts = (boolean) M_CheckParm ("-showintercepts");
}

//-----------------------------------------------------------------------------
// Check for limit and increase if necessary -- killough/jad

static int check_intercept (void)
{
  size_t offset;
  intercept_t *newintercepts;

  offset = intercept_p - intercepts;

  if (offset >= num_intercepts)
  {
    newintercepts = (intercept_t *) realloc (intercepts, sizeof(*intercepts) * (num_intercepts+128));
    if (newintercepts == NULL)
      return (0);

    num_intercepts += 128;
    intercepts = newintercepts;
    intercept_p = newintercepts + offset;
  }

  return (1);
}

//-----------------------------------------------------------------------------
//
// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
boolean PIT_AddLineIntercepts (line_t *ld)
{
  int s1;
  int s2;
  fixed_t frac;
  divline_t dl;

  // avoid precision problems with two routines
  if (trace.dx > FRACUNIT * 16 || trace.dy > FRACUNIT * 16
      || trace.dx < -FRACUNIT * 16 || trace.dy < -FRACUNIT * 16)
  {
    s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
    s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
  }
  else
  {
    s1 = P_PointOnLineSide (trace.x, trace.y, ld);
    s2 = P_PointOnLineSide (trace.x + trace.dx, trace.y + trace.dy, ld);
  }

  if (s1 == s2)
    return true;	// line isn't crossed

  // hit the line
  P_MakeDivline (ld, &dl);
  frac = P_InterceptVector (&trace, &dl);

  if (frac < 0)
    return true;	// behind source

  // try to early out the check
  if (earlyout && frac < FRACUNIT && !ld->backsector)
    return false;	// stop checking

  if (check_intercept())
  {
    intercept_p->frac = frac;
    intercept_p->thing = NULL;
    intercept_p->line = ld;
    intercept_p++;
  }
  return true;		// continue
}

//-----------------------------------------------------------------------------
#ifndef USE_OLD_MODE
//
// PIT_AddThingIntercepts
//
boolean PIT_AddThingIntercepts (mobj_t *thing)
{
  // Taken from ZDoom:
  // [RH] Don't check a corner to corner crossection for hit.
  // Instead, check against the actual bounding box.

  // There's probably a smarter way to determine which two sides
  // of the thing face the trace than by trying all four sides...
  int numfronts = 0;
  divline_t line;
  int i;

  for (i = 0; i < 4; ++i)
  {
    switch (i)
    {
      case 0: // Top edge
	line.x = thing->x + thing->radius;
	line.y = thing->y + thing->radius;
	line.dx = -thing->radius * 2;
	line.dy = 0;
	break;

      case 1: // Right edge
	line.x = thing->x + thing->radius;
	line.y = thing->y - thing->radius;
	line.dx = 0;
	line.dy = thing->radius * 2;
	break;

      case 2: // Bottom edge
	line.x = thing->x - thing->radius;
	line.y = thing->y - thing->radius;
	line.dx = thing->radius * 2;
	line.dy = 0;
	break;

      case 3: // Left edge
	line.x = thing->x - thing->radius;
	line.y = thing->y + thing->radius;
	line.dx = 0;
	line.dy = thing->radius * -2;
	break;
    }

    // Check if this side is facing the trace origin
    if (P_PointOnDivlineSide (trace.x, trace.y, &line) == 0)
    {
      numfronts++;

      // If it is, see if the trace crosses it
      if (P_PointOnDivlineSide (line.x, line.y, &trace) !=
	  P_PointOnDivlineSide (line.x + line.dx, line.y + line.dy, &trace))
      {
	// It's a hit
	fixed_t frac = P_InterceptVector (&trace, &line);

	if (frac < 0)
	  return true;		// behind source

	if (check_intercept ())
	{
	  intercept_p->frac = frac;
	  intercept_p->line = NULL;
	  intercept_p->thing = thing;
	  intercept_p++;
	}
	return true;
      }
    }
  }

  // If none of the sides were facing the trace, then the trace
  // must have started inside the box, so add it as an intercept.
  if (numfronts == 0)
  {
    if (check_intercept ())
    {
      intercept_p->frac = 0;
      intercept_p->line = NULL;
      intercept_p->thing = thing;
      intercept_p++;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
boolean P_TraverseIntercepts (traverser_t func, fixed_t maxfrac)
{
  int count = intercept_p - intercepts;
  intercept_t *in = NULL;

  if (showintercepts)
    printf ("intercepts used = %u/%u\n", count, num_intercepts);

  while (count--)
  {
    fixed_t dist = MAXINT;
    intercept_t *scan;

    for (scan = intercepts; scan < intercept_p; scan++)
    {
      if (scan->frac < dist)
      {
	dist = scan->frac;
	in = scan;
      }
    }

    if (dist > maxfrac)
      return true;	// checked everything in range

    if (!func (in))
      return false;	// don't bother going farther

    in->frac = MAXINT;
  }

  return true;		// everything was traversed
}

//-----------------------------------------------------------------------------
//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
boolean P_PathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
			int flags, boolean (*trav)(intercept_t *))
{
  fixed_t xt1, yt1;
  fixed_t xt2, yt2;
  fixed_t xstep, ystep;
  fixed_t partial;
  fixed_t xintercept, yintercept;
  int mapx, mapy;
  int mapxstep, mapystep;
  int count;

  earlyout = (boolean) (flags & PT_EARLYOUT);

  validcount++;
  intercept_p = intercepts;

  if (!((x1 - bmaporgx) & (MAPBLOCKSIZE - 1)))
    x1 += FRACUNIT;		// don't side exactly on a line

  if (!((y1 - bmaporgy) & (MAPBLOCKSIZE - 1)))
    y1 += FRACUNIT;		// don't side exactly on a line

  trace.x = x1;
  trace.y = y1;
  trace.dx = x2 - x1;
  trace.dy = y2 - y1;

  x1 -= bmaporgx;
  y1 -= bmaporgy;
  xt1 = x1 >> MAPBLOCKSHIFT;
  yt1 = y1 >> MAPBLOCKSHIFT;

  x2 -= bmaporgx;
  y2 -= bmaporgy;
  xt2 = x2 >> MAPBLOCKSHIFT;
  yt2 = y2 >> MAPBLOCKSHIFT;

  if (xt2 > xt1)
  {
    mapxstep = 1;
    partial = FRACUNIT - ((x1 >> MAPBTOFRAC) & (FRACUNIT - 1));
    ystep = FixedDiv(y2 - y1, ABS(x2 - x1));
  }
  else if (xt2 < xt1)
  {
    mapxstep = -1;
    partial = (x1 >> MAPBTOFRAC) & (FRACUNIT - 1);
    ystep = FixedDiv(y2 - y1, ABS(x2 - x1));
  }
  else
  {
    mapxstep = 0;
    partial = FRACUNIT;
    ystep = 256 * FRACUNIT;
  }

  yintercept = (y1 >> MAPBTOFRAC) + FixedMul(partial, ystep);

  if (yt2 > yt1)
  {
    mapystep = 1;
    partial = FRACUNIT - ((y1 >> MAPBTOFRAC) & (FRACUNIT - 1));
    xstep = FixedDiv(x2 - x1, ABS(y2 - y1));
  }
  else if (yt2 < yt1)
  {
    mapystep = -1;
    partial = (y1 >> MAPBTOFRAC) & (FRACUNIT - 1);
    xstep = FixedDiv(x2 - x1, ABS(y2 - y1));
  }
  else
  {
    mapystep = 0;
    partial = FRACUNIT;
    xstep = 256 * FRACUNIT;
  }

  xintercept = (x1 >> MAPBTOFRAC) + FixedMul (partial, xstep);

  // Step through map blocks.
  // Count is present to prevent a round off error
  // from skipping the break.
  mapx = xt1;
  mapy = yt1;

  for (count = 0; count < 100; count++)
  {
    if (flags & PT_ADDLINES)
    {
      if (!P_BlockLinesIterator (mapx, mapy, PIT_AddLineIntercepts))
	return false;		// early out
    }

    if (flags & PT_ADDTHINGS)
    {
      if (!P_BlockThingsIterator (mapx, mapy, PIT_AddThingIntercepts))
	return false;		// early out
    }

    if (mapx == xt2 && mapy == yt2)
      break;

    switch ((((yintercept >> FRACBITS) == mapy) << 1) | ((xintercept >> FRACBITS) == mapx))
    {
      case 0:
	count = 100;
	break;

      case 1:
	xintercept += xstep;
	mapy += mapystep;
	break;

      case 2:
	yintercept += ystep;
	mapx += mapxstep;
	break;

      case 3:
	if (flags & PT_ADDLINES)
	{
	  if (!P_BlockLinesIterator (mapx + mapxstep, mapy, PIT_AddLineIntercepts))
	    return false;
	  if (!P_BlockLinesIterator (mapx, mapy + mapystep, PIT_AddLineIntercepts))
	    return false;
	}

	if (flags & PT_ADDTHINGS)
	{
	  if (!P_BlockThingsIterator (mapx + mapxstep, mapy, PIT_AddThingIntercepts))
	    return false;
	  if (!P_BlockThingsIterator (mapx, mapy + mapystep, PIT_AddThingIntercepts))
	    return false;
	}
	xintercept += xstep;
	yintercept += ystep;
	mapx += mapxstep;
	mapy += mapystep;
	break;
    }
  }

  // go through the sorted list
  return P_TraverseIntercepts (trav, FRACUNIT);
}

//-----------------------------------------------------------------------------
#else
//-----------------------------------------------------------------------------
//
// PIT_AddThingIntercepts
//
boolean PIT_AddThingIntercepts (mobj_t* thing)
{
    fixed_t		x1;
    fixed_t		y1;
    fixed_t		x2;
    fixed_t		y2;

    int			s1;
    int			s2;

    boolean		tracepositive;

    divline_t		dl;

    fixed_t		frac;

    tracepositive = (boolean) ((trace.dx ^ trace.dy)>0);

    // check a corner to corner crossection for hit
    if (tracepositive)
    {
	x1 = thing->x - thing->radius;
	y1 = thing->y + thing->radius;

	x2 = thing->x + thing->radius;
	y2 = thing->y - thing->radius;
    }
    else
    {
	x1 = thing->x - thing->radius;
	y1 = thing->y - thing->radius;

	x2 = thing->x + thing->radius;
	y2 = thing->y + thing->radius;
    }

    s1 = P_PointOnDivlineSide (x1, y1, &trace);
    s2 = P_PointOnDivlineSide (x2, y2, &trace);

    if (s1 == s2)
	return true;		// line isn't crossed

    dl.x = x1;
    dl.y = y1;
    dl.dx = x2-x1;
    dl.dy = y2-y1;

    frac = P_InterceptVector (&trace, &dl);

    if (frac < 0)
	return true;		// behind source

    if (check_intercept ())
    {
      intercept_p->frac = frac;
      intercept_p->line = NULL;
      intercept_p->thing = thing;
      intercept_p++;
    }

    return true;		// keep going
}

//-----------------------------------------------------------------------------
//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
boolean
P_TraverseIntercepts
( traverser_t	func,
  fixed_t	maxfrac )
{
    int			count;
    fixed_t		dist;
    intercept_t*	scan;
    intercept_t*	in;

    count = intercept_p - intercepts;

    in = 0;			// shut up compiler warning

    while (count--)
    {
	dist = MAXINT;
	for (scan = intercepts ; scan<intercept_p ; scan++)
	{
	    if (scan->frac < dist)
	    {
		dist = scan->frac;
		in = scan;
	    }
	}

	if (dist > maxfrac)
	    return true;	// checked everything in range

#if 0  // UNUSED
    {
	// don't check these yet, there may be others inserted
	in = scan = intercepts;
	for ( scan = intercepts ; scan<intercept_p ; scan++)
	    if (scan->frac > maxfrac)
		*in++ = *scan;
	intercept_p = in;
	return false;
    }
#endif

	if ( !func (in) )
	    return false;	// don't bother going farther

	in->frac = MAXINT;
    }

    return true;		// everything was traversed
}

//-----------------------------------------------------------------------------
//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
boolean
P_PathTraverse
( fixed_t		x1,
  fixed_t		y1,
  fixed_t		x2,
  fixed_t		y2,
  int			flags,
  boolean (*trav) (intercept_t *))
{
    fixed_t	xt1;
    fixed_t	yt1;
    fixed_t	xt2;
    fixed_t	yt2;

    fixed_t	xstep;
    fixed_t	ystep;

    fixed_t	partial;

    fixed_t	xintercept;
    fixed_t	yintercept;

    int		mapx;
    int		mapy;

    int		mapxstep;
    int		mapystep;

    int		count;

    earlyout = (boolean) (flags & PT_EARLYOUT);

    validcount++;
    intercept_p = intercepts;

    if ( ((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
	x1 += FRACUNIT;	// don't side exactly on a line

    if ( ((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
	y1 += FRACUNIT;	// don't side exactly on a line

    trace.x = x1;
    trace.y = y1;
    trace.dx = x2 - x1;
    trace.dy = y2 - y1;

    x1 -= bmaporgx;
    y1 -= bmaporgy;
    xt1 = x1>>MAPBLOCKSHIFT;
    yt1 = y1>>MAPBLOCKSHIFT;

    x2 -= bmaporgx;
    y2 -= bmaporgy;
    xt2 = x2>>MAPBLOCKSHIFT;
    yt2 = y2>>MAPBLOCKSHIFT;

    if (xt2 > xt1)
    {
	mapxstep = 1;
	partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
	ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else if (xt2 < xt1)
    {
	mapxstep = -1;
	partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
	ystep = FixedDiv (y2-y1,abs(x2-x1));
    }
    else
    {
	mapxstep = 0;
	partial = FRACUNIT;
	ystep = 256*FRACUNIT;
    }

    yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


    if (yt2 > yt1)
    {
	mapystep = 1;
	partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
	xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else if (yt2 < yt1)
    {
	mapystep = -1;
	partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
	xstep = FixedDiv (x2-x1,abs(y2-y1));
    }
    else
    {
	mapystep = 0;
	partial = FRACUNIT;
	xstep = 256*FRACUNIT;
    }
    xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);

    // Step through map blocks.
    // Count is present to prevent a round off error
    // from skipping the break.
    mapx = xt1;
    mapy = yt1;

    for (count = 0 ; count < 64 ; count++)
    {
	if (flags & PT_ADDLINES)
	{
	    if (!P_BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
		return false;	// early out
	}

	if (flags & PT_ADDTHINGS)
	{
	    if (!P_BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
		return false;	// early out
	}

	if (mapx == xt2
	    && mapy == yt2)
	{
	    break;
	}

	if ( (yintercept >> FRACBITS) == mapy)
	{
	    yintercept += ystep;
	    mapx += mapxstep;
	}
	else if ( (xintercept >> FRACBITS) == mapx)
	{
	    xintercept += xstep;
	    mapy += mapystep;
	}

    }
    // go through the sorted list
    return P_TraverseIntercepts ( trav, FRACUNIT );
}

//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
// MAES: support 512x512 blockmaps.
extern int blockmapxneg;
extern int blockmapyneg;

int P_GetSafeBlockX (int coord)
{
    // If x is LE than those special values, interpret as positive.
    // Otherwise, leave it as it is.
    if ((coord >>= MAPBLOCKSHIFT) <= blockmapxneg)
        return (coord & 0x01FF);    // Broke width boundary

    return coord;
}

//-----------------------------------------------------------------------------
// MAES: support 512x512 blockmaps.
int P_GetSafeBlockY (int coord)
{
    // If y is LE than those special values, interpret as positive.
    // Otherwise, leave it as it is.
    if ((coord >>= MAPBLOCKSHIFT) <= blockmapyneg)
        return (coord & 0x01FF);    // Broke height boundary

    return coord;
}

//-----------------------------------------------------------------------------
//
// MBF21: RoughBlockCheck
// [XA] adapted from Hexen -- used by P_RoughTargetSearch
//
static mobj_t *RoughBlockCheck(mobj_t *mo, int index, angle_t fov)
{
    mobj_t  *link = blocklinks[index];

    while (link)
    {
        // skip non-shootable actors
        if (!(link->flags & MF_SHOOTABLE))
        {
            link = link->bnext;
            continue;
        }

        // skip the projectile's owner
        if (link == mo->target)
        {
            link = link->bnext;
            continue;
        }

        // skip actors on the same "team", unless infighting
        if (mo->target && !((link->flags ^ mo->target->flags) & MF_FRIEND)
            && mo->target->target != link && !(link->player && mo->target->player))
        {
            link = link->bnext;
            continue;
        }

        // skip actors outside of specified FOV
        if (fov > 0 && !P_CheckFOV(mo, link, fov))
        {
            link = link->bnext;
            continue;
        }

        // skip actors not in line of sight
        if (!P_CheckSight(mo, link))
        {
            link = link->bnext;
            continue;
        }

        // all good! return it.
        return link;
    }

    // couldn't find a valid target
    return NULL;
}

//-----------------------------------------------------------------------------
//
// MBF21: P_RoughTargetSearch
// Searches though the surrounding mapblocks for monsters/players
// based on Hexen's P_RoughMonsterSearch
//
// distance is in MAPBLOCKUNITS
mobj_t *P_RoughTargetSearch(mobj_t *mo, angle_t fov, int distance)
{
    const int   startx = (mo->x - bmaporgx) >> MAPBLOCKSHIFT;
    const int   starty = (mo->y - bmaporgy) >> MAPBLOCKSHIFT;
    mobj_t      *target;

    if (startx >= 0 && startx < bmapwidth && starty >= 0 && starty < bmapheight
        && ((target = RoughBlockCheck(mo, starty * bmapwidth + startx, fov))!=0))
        return target;  // found a target right away

    for (int count = 1; count <= distance; count++)
    {
        int blockx, blocky;
        int blockindex;
        int firststop = startx + count;
        int secondstop;
        int thirdstop;
        int finalstop;

        if (firststop < 0)
            continue;

        if ((secondstop = starty + count) < 0)
            continue;

        if (firststop >= bmapwidth)
            firststop = bmapwidth - 1;

        if (secondstop >= bmapheight)
            secondstop = bmapheight - 1;

	if (count > startx)
	{
	  blockx = 0;
	}
	else
	{
	  blockx = startx - count;
	  if (blockx > (bmapwidth - 1))
	  {
	    blockx = (bmapwidth - 1);
	  }
	}

	if (count > starty)
	{
	  blocky = 0;
	}
	else
	{
	  blocky = starty - count;
	  if (blocky > (bmapheight - 1))
	  {
	    blocky = (bmapheight - 1);
	  }
	}

        blockindex = blocky * bmapwidth + blockx;

        thirdstop = secondstop * bmapwidth + blockx;
        secondstop = secondstop * bmapwidth + firststop;
        firststop += blocky * bmapwidth;
        finalstop = blockindex;

        // Trace the first block section (along the top)
        for (; blockindex <= firststop; blockindex++)
            if (((target = RoughBlockCheck(mo, blockindex, fov))!=0))
                return target;

        // Trace the second block section (right edge)
        for (blockindex--; blockindex <= secondstop; blockindex += bmapwidth)
            if (((target = RoughBlockCheck(mo, blockindex, fov))!=0))
                return target;

        // Trace the third block section (bottom edge)
        for (blockindex -= bmapwidth; blockindex >= thirdstop; blockindex--)
            if (((target = RoughBlockCheck(mo, blockindex, fov))!=0))
                return target;

        // Trace the final block section (left edge)
        for (blockindex++; blockindex > finalstop; blockindex -= bmapwidth)
            if (((target = RoughBlockCheck(mo, blockindex, fov))!=0))
                return target;
    }

    return NULL;
}

//-----------------------------------------------------------------------------
