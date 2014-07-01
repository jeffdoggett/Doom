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
//	Movement, collision handling.
//	Shooting and aiming.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_map.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";
#endif


#include "includes.h"


static fixed_t	tmbbox[4];
static mobj_t*	tmthing;
static int	tmflags;
static fixed_t	tmx;
static fixed_t	tmy;
static fixed_t	tmz;


// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
boolean		floatok;

fixed_t		tmfloorz;
fixed_t		tmceilingz;
fixed_t		tmdropoffz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t*		ceilingline;
line_t*		blockline;     // killough 8/11/98: blocking linedef

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid

// 1/11/98 killough: removed limit on special lines crossed
line_t	**	spechit;
static int	spechit_max;
int		numspechit;

boolean		Monsters_Infight = false;
static mobj_t*	onmobj;

extern void P_ExplodeMissile (mobj_t* mo);
extern boolean	gamekeydown[NUMKEYS];

void P_Init_SpecHit (void)
{
  spechit_max = 32;
  spechit = (line_t **)malloc(sizeof(*spechit) * spechit_max);
  if (spechit == NULL)
    I_Error ("P_Init_SpecHit: No RAM");
}

//
// TELEPORT MOVE
//

//
// PIT_StompThing
//
boolean PIT_StompThing (mobj_t* thing)
{
    map_dests_t * mapd_ptr;
    fixed_t	blockdist;

    if (!(thing->flags & MF_SHOOTABLE) )
	return true;

    blockdist = thing->radius + tmthing->radius;

    if ( abs(thing->x - tmx) >= blockdist
	 || abs(thing->y - tmy) >= blockdist )
    {
	// didn't hit it
	return true;
    }

    // don't clip against self
    if (thing == tmthing)
	return true;

    // monsters don't stomp things except on boss level (30)
    if (!tmthing->player) // && gamemap != 30)
    {
      mapd_ptr = G_Access_MapInfoTab (gameepisode, gamemap);
      if (mapd_ptr -> allow_monster_telefrags == 0)
	return false;
    }

    if (tmz > thing->z + thing->height)
        return true; // overhead
    if (tmz + tmthing->height < thing->z)
        return true; // underneath

    P_DamageMobj (thing, tmthing, tmthing, (thing->health+thing->info->spawnhealth)*2);

    return true;
}


//
// P_TeleportMove
//
boolean P_TeleportMove (mobj_t* thing, fixed_t x, fixed_t y, fixed_t z)
{
    int		xl;
    int		xh;
    int		yl;
    int		yh;
    int		bx;
    int		by;

    subsector_t*	newsubsec;

    // kill anything occupying the position
    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;
    tmz = z;

    tmbbox[BOXTOP] = y + tmthing->radius;
    tmbbox[BOXBOTTOM] = y - tmthing->radius;
    tmbbox[BOXRIGHT] = x + tmthing->radius;
    tmbbox[BOXLEFT] = x - tmthing->radius;

    newsubsec = R_PointInSubsector (x,y);
    ceilingline = NULL;

    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    // stomp on any things contacted
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
		return false;

    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition (thing);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;
    thing->x = x;
    thing->y = y;

    P_SetThingPosition (thing);

    return true;
}


//
// MOVEMENT ITERATOR FUNCTIONS
//


//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//
boolean PIT_CheckLine (line_t* ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
	|| tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
	|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
	|| tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
	return true;

    if (P_BoxOnLineSide (tmbbox, ld) != -1)
	return true;

    // A line has been hit

    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.

    if (!ld->backsector)
    {
	blockline = ld;
	return false;		// one sided line
    }

    if (!(tmthing->flags & MF_MISSILE) )
    {
	if ( ld->flags & ML_BLOCKING )
	    return false;	// explicitly blocking everything

	if ( !tmthing->player && (ld->flags & ML_BLOCKMONSTERS))
	    return false;	// block monsters only
    }

    // set openrange, opentop, openbottom
    P_LineOpening (ld);

    // adjust floor / ceiling heights
    if (opentop < tmceilingz)
    {
	tmceilingz = opentop;
	ceilingline = ld;
	blockline = ld;
    }

    if (openbottom > tmfloorz)
    {
	tmfloorz = openbottom;
	blockline = ld;
    }

    if (lowfloor < tmdropoffz)
	tmdropoffz = lowfloor;

    // if contacted a special line, add it to the list
    if (ld->special)
    {
	// 1/11/98 killough: remove limit on lines hit
	if (numspechit >= spechit_max)
	{
	    spechit_max += 32;
	    spechit = (line_t **)realloc(spechit, sizeof(*spechit) * spechit_max);
	    if (spechit == NULL)
	      I_Error ("PIT_CheckLine: No RAM");
	}
	spechit[numspechit++] = ld;
    }

    return true;
}

//
// PIT_CheckThing
//
boolean PIT_CheckThing (mobj_t* thing)
{
    fixed_t	newdist;
    fixed_t	olddist;
    fixed_t	blockdist;
    boolean	solid;
    int		damage;
    int		tradius;

    if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_MISSILE|MF_SHOOTABLE) ))
	return true;

    // [BH] don't hit if either thing is a corpse, which may still be solid if
    // they are still going through their death sequence.
    if ((thing->flags & MF_CORPSE) || (tmthing->flags & MF_CORPSE))
	return true;

    /* We've changed all of the MF_SPECIAL stuff to their */
    /* actual radius rather than all being 20*FRACUNIT but */
    /* we want the original size back again when picking up. */

//  if (thing->flags & MF_SPECIAL)
      tradius = thing->info->pickupradius;
//  else
//    tradius = thing->radius;

    blockdist = tradius + tmthing->radius;

    if ( abs(thing->x - tmx) >= blockdist
	 || abs(thing->y - tmy) >= blockdist )
    {
	// didn't hit it
	return true;
    }

    // don't clip against self
    if (thing == tmthing)
	return true;

    if (thing->flags & MF_MISSILE)
    {
      if (!tmthing->player)
	P_ExplodeMissile (thing);

      if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE) ))
	return (true);
    }

    // check if a mobj passed over/under another object

    if (tmthing->flags2 & MF2_PASSMOBJ)
    {
	if (tmthing->z >= thing->z + thing->height)
	    return true;	// over thing
	else if (tmthing->z + tmthing->height <= thing->z)
	    return true;	// under thing
    }

    // check for skulls slamming into things
    if (tmthing->flags & MF_SKULLFLY)
    {
	damage = ((P_Random()%8)+1)*tmthing->info->damage;

	P_DamageMobj (thing, tmthing, tmthing, damage);

	tmthing->flags &= ~MF_SKULLFLY;
	tmthing->momx = tmthing->momy = tmthing->momz = 0;

	P_SetMobjState (tmthing, (statenum_t) tmthing->info->spawnstate);

	return false;		// stop moving
    }


    // missiles can hit other things
    if (tmthing->flags & MF_MISSILE)
    {
	// see if it went over / under
	if (tmthing->z > thing->z + thing->height)
	    return true;		// overhead
	if (tmthing->z+tmthing->height < thing->z)
	    return true;		// underneath

	if (tmthing->target && (
	    tmthing->target->type == thing->type ||
	    (tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
	    (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
	{
	    // Don't hit same species as originator.
	    if (thing == tmthing->target)
		return true;

	    if ((thing->type != MT_PLAYER) && (Monsters_Infight == false))
	    {
		// Explode, but do no damage.
		// Let players missile other players.
		return false;
	    }
	}

	if (! (thing->flags & MF_SHOOTABLE) )
	{
	    // didn't do any damage
	    return (boolean)!(thing->flags & MF_SOLID);
	}

	// damage / explode
	damage = ((P_Random()%8)+1)*tmthing->info->damage;
	P_DamageMobj (thing, tmthing, tmthing->target, damage);

	// don't traverse any more
	return false;
    }

    // check for special pickup
    if (thing->flags & MF_SPECIAL)
    {
	solid = (boolean) (thing->flags&MF_SOLID);
	if (tmflags&MF_PICKUP)
	{
	    // can remove thing
	    P_TouchSpecialThing (thing, tmthing);
	}
	return (boolean)!solid;
    }

    // check if things are stuck and allow move if it makes them further apart

    if ((tmx == tmthing->x) && (tmy == tmthing->y))
	return true;

    newdist = P_ApproxDistance(thing->x - tmx, thing->y - tmy);
    olddist = P_ApproxDistance(thing->x - tmthing->x, thing->y - tmthing->y);

    if ((newdist > olddist)
     && (!((tmthing->z >= thing->z + thing->height)
      || (tmthing->z + tmthing->height <= thing->z))))
	return true;

    // killough 3/16/98: Allow non-solid moving objects to move through solid
    // ones, by allowing the moving thing (tmthing) to move if it's non-solid,
    // despite another solid thing being in the way.
    // killough 4/11/98: Treat no-clipping things as not blocking
    return (boolean)!((thing->flags & MF_SOLID) && !(thing->flags & MF_NOCLIP)
		&& (tmthing->flags & MF_SOLID));
}


//
// PIT_CheckOnmobjZ
//
boolean PIT_CheckOnmobjZ (mobj_t * thing)
{
    fixed_t     blockdist;

    if (!(thing->flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE)))
	return true;		// Can't hit thing

    blockdist = thing->radius + tmthing->radius;

    if (ABS(thing->x - tmx) >= blockdist || ABS(thing->y - tmy) >= blockdist)
	return true;		// Didn't hit thing

    if (thing == tmthing)
	return true;		// Don't clip against self

    if (tmthing->z > thing->z + thing->height)
	return true;

    if (tmthing->z + tmthing->height < thing->z)
	return true;		// Under thing

    if (thing->flags & MF_SOLID)
	onmobj = thing;

    return (boolean)!(thing->flags & MF_SOLID);
}

//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x,y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
boolean
P_CheckPosition
( mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
    int		xl;
    int		xh;
    int		yl;
    int		yh;
    int		bx;
    int		by;
    int		tmradius;
    subsector_t*newsubsec;

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    tmradius = tmthing->radius;
    tmbbox[BOXTOP] = y + tmradius;
    tmbbox[BOXBOTTOM] = y - tmradius;
    tmbbox[BOXRIGHT] = x + tmradius;
    tmbbox[BOXLEFT] = x - tmradius;

    newsubsec = R_PointInSubsector (x,y);
    ceilingline = NULL;
    blockline = NULL;

    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    if ( tmflags & MF_NOCLIP )
	return true;

    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
		return false;

    // check lines
    xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

    for (bx=xl ; bx<=xh ; bx++)
	for (by=yl ; by<=yh ; by++)
	    if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
		return false;

    return true;
}


//
// P_CheckOnmobj
// Checks if the new Z position is legal
//
mobj_t *P_CheckOnmobj (mobj_t * thing)
{
    int		xl, xh, yl, yh, bx, by;
    subsector_t	*newsubsec;
    fixed_t	x;
    fixed_t	y;
    mobj_t	oldmo;

    x = thing->x;
    y = thing->y;
    tmthing = thing;
    tmflags = thing->flags;
    oldmo = *thing;		// save the old mobj before the fake zmovement
    P_FakeZMovement(tmthing);

    tmx = x;
    tmy = y;

    tmbbox[BOXTOP] = y + tmthing->radius;
    tmbbox[BOXBOTTOM] = y - tmthing->radius;
    tmbbox[BOXRIGHT] = x + tmthing->radius;
    tmbbox[BOXLEFT] = x - tmthing->radius;

    newsubsec = R_PointInSubsector(x, y);
    ceilingline = NULL;

    // the base floor / ceiling is from the subsector that contains the
    // point.  Any contacted lines the step closer together will adjust them
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    validcount++;
    numspechit = 0;

    if (tmflags & MF_NOCLIP)
	return NULL;

    // check things first, possibly picking things up
    // the bounding box is extended by MAXRADIUS because mobj_ts are grouped
    // into mapblocks based on their origin point, and can overlap into adjacent
    // blocks by up to MAXRADIUS units
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; bx++)
	for (by = yl; by <= yh; by++)
	    if (!P_BlockThingsIterator(bx, by, PIT_CheckOnmobjZ))
	    {
		*tmthing = oldmo;
		return onmobj;
	    }
    *tmthing = oldmo;

    return NULL;
}

//
// P_FakeZMovement
//
void P_FakeZMovement (mobj_t *mo)
{
    // adjust height
    mo->z += mo->momz;

    if ((mo->flags & MF_FLOAT) && mo->target)
    {
	// float down towards target if too close
	if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
	{
	    fixed_t delta = (mo->target->z + (mo->height >> 1) - mo->z) * 3;

	    if (P_ApproxDistance(mo->x - mo->target->x, mo->y - mo->target->y) < ABS(delta))
		mo->z += (delta < 0 ? -FLOATSPEED : FLOATSPEED);
        }
    }

    // clip movement
    if (mo->z <= mo->floorz)
    {
	// hit the floor
	if (mo->flags & MF_SKULLFLY)
            mo->momz = -mo->momz; // the skull slammed into something

	if (mo->momz < 0)
            mo->momz = 0;
	mo->z = mo->floorz;
    }
    else if (!(mo->flags & MF_NOGRAVITY))
    {
	if (!mo->momz)
            mo->momz = -GRAVITY;
	mo->momz -= GRAVITY;
    }

    if (mo->z + mo->height > mo->ceilingz)
    {
	// hit the ceiling
	if (mo->momz > 0)
            mo->momz = 0;

	if (mo->flags & MF_SKULLFLY)
            mo->momz = -mo->momz; // the skull slammed into something

	mo->z = mo->ceilingz - mo->height;
    }
}

//
//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
boolean
P_TryMove
( mobj_t*	thing,
  fixed_t	x,
  fixed_t	y )
{
  fixed_t	oldx;
  fixed_t	oldy;
  int		side;
  int		oldside;
  line_t*	ld;

  floatok = false;
  if (!P_CheckPosition (thing, x, y))
    return false;		// solid wall or thing

  if ( !(thing->flags & MF_NOCLIP) )
  {
    if (tmceilingz - tmfloorz < thing->height)
      return false;		// doesn't fit

    floatok = true;

    if ( !(thing->flags&MF_TELEPORT)
     &&tmceilingz - thing->z < thing->height)
      return false;		// mobj must lower itself to fit

    if ( !(thing->flags&MF_TELEPORT)
     && tmfloorz - thing->z > 24*FRACUNIT )
      return false;		// too big a step up

    if ( !(thing->flags&(MF_DROPOFF|MF_FLOAT|MF_SLIDE))
     && tmfloorz - tmdropoffz > 24*FRACUNIT )
      return false;		// don't stand over a dropoff
  }

  // the move is ok,
  // so link the thing into its new position
  P_UnsetThingPosition (thing);

  oldx = thing->x;
  oldy = thing->y;
  thing->floorz = tmfloorz;
  thing->ceilingz = tmceilingz;
  thing->x = x;
  thing->y = y;

  P_SetThingPosition (thing);

  // if any special lines were hit, do the effect
  if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
  {
    while (numspechit)
    {
      numspechit--;
      // see if the line was crossed
      ld = spechit[numspechit];
      if (ld->special)			// Quick double check!
      {
	side = P_PointOnLineSide (thing->x, thing->y, ld);
	oldside = P_PointOnLineSide (oldx, oldy, ld);
	if (side != oldside)
	  P_CrossSpecialLine (ld, oldside, thing);
      }
    }
  }

  return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
boolean P_ThingHeightClip (mobj_t* thing)
{
    boolean		onfloor;

    onfloor = (boolean)(thing->z == thing->floorz);

    P_CheckPosition (thing, thing->x, thing->y);
    // what about stranding a monster partially off an edge?

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    if (onfloor)
    {
	// walking monsters rise and fall with the floor
	thing->z = thing->floorz;
    }
    else
    {
	// don't adjust a floating monster unless forced to
	if (thing->z+thing->height > thing->ceilingz)
	    thing->z = thing->ceilingz - thing->height;
    }

    if (thing->ceilingz - thing->floorz < thing->height)
	return false;

    return true;
}



//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//
fixed_t		bestslidefrac;
fixed_t		secondslidefrac;

line_t*		bestslideline;
line_t*		secondslideline;

mobj_t*		slidemo;

fixed_t		tmxmove;
fixed_t		tmymove;



//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine (line_t* ld)
{
    int		side;

    angle_t	lineangle;
    angle_t	moveangle;
    angle_t	deltaangle;

    fixed_t	movelen;
    fixed_t	newlen;


    if (ld->slopetype == ST_HORIZONTAL)
    {
	tmymove = 0;
	return;
    }

    if (ld->slopetype == ST_VERTICAL)
    {
	tmxmove = 0;
	return;
    }

    side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);

    lineangle = R_PointToAngle2 (0,0, ld->dx, ld->dy);

    if (side == 1)
	lineangle += ANG180;

    moveangle = R_PointToAngle2 (0,0, tmxmove, tmymove);
    moveangle += 10;		// Prevents sudden path reversal due to rounding error
    deltaangle = moveangle-lineangle;

    if (deltaangle > ANG180)
	deltaangle += ANG180;
    //	I_Error ("SlideLine: ang>ANG180");

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;

    movelen = P_ApproxDistance (tmxmove, tmymove);
    newlen = FixedMul (movelen, finecosine[deltaangle]);

    tmxmove = FixedMul (newlen, finecosine[lineangle]);
    tmymove = FixedMul (newlen, finesine[lineangle]);
}


//
// PTR_SlideTraverse
//
boolean PTR_SlideTraverse (intercept_t* in)
{
    line_t*	li;

    if (!in->isaline)
	I_Error ("PTR_SlideTraverse: not a line?");

    li = in->d.line;

    if ( ! (li->flags & ML_TWOSIDED) )
    {
	if (P_PointOnLineSide (slidemo->x, slidemo->y, li))
	{
	    // don't hit the back side
	    return true;
	}
	goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening (li);

    if (openrange < slidemo->height)
	goto isblocking;		// doesn't fit

    if (opentop - slidemo->z < slidemo->height)
	goto isblocking;		// mobj is too high

    if (openbottom - slidemo->z > 24*FRACUNIT )
	goto isblocking;		// too big a step up

    // this line doesn't block movement
    return true;

    // the line does block movement,
    // see if it is closer than best so far
  isblocking:
    if (in->frac < bestslidefrac)
    {
	secondslidefrac = bestslidefrac;
	secondslideline = bestslideline;
	bestslidefrac = in->frac;
	bestslideline = li;
    }

    return false;	// stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (mobj_t* mo)
{
    fixed_t	leadx;
    fixed_t	leady;
    fixed_t	trailx;
    fixed_t	traily;
    fixed_t	newx;
    fixed_t	newy;
    int		hitcount;

    slidemo = mo;
    hitcount = 0;

  retry:
    if (++hitcount == 3)
	goto stairstep;		// don't loop forever


    // trace along the three leading corners
    if (mo->momx > 0)
    {
	leadx = mo->x + mo->radius;
	trailx = mo->x - mo->radius;
    }
    else
    {
	leadx = mo->x - mo->radius;
	trailx = mo->x + mo->radius;
    }

    if (mo->momy > 0)
    {
	leady = mo->y + mo->radius;
	traily = mo->y - mo->radius;
    }
    else
    {
	leady = mo->y - mo->radius;
	traily = mo->y + mo->radius;
    }

    bestslidefrac = FRACUNIT+1;

    P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );
    P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
		     PT_ADDLINES, PTR_SlideTraverse );

    // move up to the wall
    if (bestslidefrac == FRACUNIT+1)
    {
	// the move most have hit the middle, so stairstep
      stairstep:
	if (!P_TryMove (mo, mo->x, mo->y + mo->momy))
	    P_TryMove (mo, mo->x + mo->momx, mo->y);
	return;
    }

    // fudge a bit to make sure it doesn't hit
    bestslidefrac -= 0x800;
    if (bestslidefrac > 0)
    {
	newx = FixedMul (mo->momx, bestslidefrac);
	newy = FixedMul (mo->momy, bestslidefrac);

	if (!P_TryMove (mo, mo->x+newx, mo->y+newy))
	    goto stairstep;
    }

    // Now continue along the wall.
    // First calculate remainder.
    bestslidefrac = FRACUNIT-(bestslidefrac+0x800);

    if (bestslidefrac > FRACUNIT)
	bestslidefrac = FRACUNIT;

    if (bestslidefrac <= 0)
	return;

    tmxmove = FixedMul (mo->momx, bestslidefrac);
    tmymove = FixedMul (mo->momy, bestslidefrac);

    P_HitSlideLine (bestslideline);	// clip the moves

    mo->momx = tmxmove;
    mo->momy = tmymove;

    if (!P_TryMove (mo, mo->x+tmxmove, mo->y+tmymove))
    {
	goto retry;
    }
}


//
// P_LineAttack
//
mobj_t*		linetarget;	// who got hit (or NULL)
mobj_t*		shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t		shootz;

int		la_damage;
fixed_t		attackrange;

fixed_t		aimslope;

// slopes to top and bottom of target
extern fixed_t	topslope;
extern fixed_t	bottomslope;


//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
boolean
PTR_AimTraverse (intercept_t* in)
{
    line_t*	li;
    mobj_t*	th;
    fixed_t	slope;
    fixed_t	thingtopslope;
    fixed_t	thingbottomslope;
    fixed_t	dist;

    if (in->isaline)
    {
	li = in->d.line;

	if ( !(li->flags & ML_TWOSIDED) )
	    return false;		// stop

	// Crosses a two sided line.
	// A two sided line will restrict
	// the possible target ranges.
	P_LineOpening (li);

	if (openbottom >= opentop)
	    return false;		// stop

	dist = FixedMul (attackrange, in->frac);

	if (li->frontsector->floorheight != li->backsector->floorheight)
	{
	    slope = FixedDiv (openbottom - shootz , dist);
	    if (slope > bottomslope)
		bottomslope = slope;
	}

	if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	    slope = FixedDiv (opentop - shootz , dist);
	    if (slope < topslope)
		topslope = slope;
	}

	if (topslope <= bottomslope)
	    return false;		// stop

	return true;			// shot continues
    }

    // shoot a thing
    th = in->d.thing;
    if (th == shootthing)
	return true;			// can't shoot self

    if (!(th->flags&MF_SHOOTABLE))
	return true;			// corpse or something

    // check angles to see if the thing can be aimed at
    dist = FixedMul (attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

    if (thingtopslope < bottomslope)
	return true;			// shot over the thing

    thingbottomslope = FixedDiv (th->z - shootz, dist);

    if (thingbottomslope > topslope)
	return true;			// shot under the thing

    // this thing can be hit!
    if (thingtopslope > topslope)
	thingtopslope = topslope;

    if (thingbottomslope < bottomslope)
	thingbottomslope = bottomslope;

    aimslope = (thingtopslope+thingbottomslope)/2;
    linetarget = th;

    return false;			// don't go any farther
}


//
// PTR_ShootTraverse
//
boolean PTR_ShootTraverse (intercept_t* in)
{
    fixed_t	x;
    fixed_t	y;
    fixed_t	z;
    fixed_t	frac;

    line_t*	li;

    mobj_t*	th;

    fixed_t	slope;
    fixed_t	dist;
    fixed_t	thingtopslope;
    fixed_t	thingbottomslope;

    if (in->isaline)
    {
	li = in->d.line;

	if (li->special)
	    P_ShootSpecialLine (shootthing, li);

	if ( !(li->flags & ML_TWOSIDED) )
	    goto hitline;

	// crosses a two sided line
	P_LineOpening (li);

	dist = FixedMul (attackrange, in->frac);

	if (li->frontsector->floorheight != li->backsector->floorheight)
	{
	    slope = FixedDiv (openbottom - shootz , dist);
	    if (slope > aimslope)
		goto hitline;
	}

	if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
	{
	    slope = FixedDiv (opentop - shootz , dist);
	    if (slope < aimslope)
		goto hitline;
	}

	// shot continues
	return true;


	// hit line
      hitline:
	// position a bit closer
	frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
	x = trace.x + FixedMul (trace.dx, frac);
	y = trace.y + FixedMul (trace.dy, frac);
	z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

	if (li->frontsector->ceilingpic == skyflatnum)
	{
	    // don't shoot the sky!
	    if (z > li->frontsector->ceilingheight)
		return false;

	    // it's a sky hack wall
	    if	(li->backsector && li->backsector->ceilingpic == skyflatnum
		 && li->backsector->ceilingheight < z)
		return false;
	}

	// Spawn bullet puffs.
	P_SpawnPuff (x,y,z);

	// don't go any farther
	return false;
    }

    // shoot a thing
    th = in->d.thing;
    if (th == shootthing)
	return true;		// can't shoot self

    if (!(th->flags&MF_SHOOTABLE))
	return true;		// corpse or something

    // check angles to see if the thing can be aimed at
    dist = FixedMul (attackrange, in->frac);
    thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

    if (thingtopslope < aimslope)
	return true;		// shot over the thing

    thingbottomslope = FixedDiv (th->z - shootz, dist);

    if (thingbottomslope > aimslope)
	return true;		// shot under the thing


    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

    x = trace.x + FixedMul (trace.dx, frac);
    y = trace.y + FixedMul (trace.dy, frac);
    z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

    // Spawn bullet puffs or blod spots,
    // depending on target type.
    if (in->d.thing->flags & MF_NOBLOOD)
	P_SpawnPuff (x,y,z);
    else
	P_SpawnBlood (x,y,z, la_damage);

    if (la_damage)
	P_DamageMobj (th, shootthing, shootthing, la_damage);

    // don't go any farther
    return false;

}


//
// P_AimLineAttack
//
fixed_t
P_AimLineAttack
( mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance )
{
    fixed_t	x2;
    fixed_t	y2;

    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;

    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

    // can't shoot outside view angles
    topslope = 100*FRACUNIT/160;
    bottomslope = -100*FRACUNIT/160;

    attackrange = distance;
    linetarget = NULL;

    P_PathTraverse ( t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_AimTraverse );

    if (linetarget)
	return aimslope;

    return 0;
}


//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
void
P_LineAttack
( mobj_t*	t1,
  angle_t	angle,
  fixed_t	distance,
  fixed_t	slope,
  int		damage )
{
    fixed_t	x2;
    fixed_t	y2;

    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;
    la_damage = damage;
    x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
    y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
    shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
    attackrange = distance;
    aimslope = slope;

    P_PathTraverse ( t1->x, t1->y,
		     x2, y2,
		     PT_ADDLINES|PT_ADDTHINGS,
		     PTR_ShootTraverse );
}

//
// USE LINES
//
mobj_t*		usething;

static boolean PTR_UseAnyway (line_t * line)
{
  int i;
  int side;
  int linenum;
  boolean rc;
  sector_t * sec;

  if ((usething->player)
   && (!netgame)
   && (gamekeydown['`'])				// Cheat
   && (line->flags & ML_TWOSIDED)
   && (M_CheckParm ("-operateanyway")))
  {
    side = P_PointOnLineSide (usething->x, usething->y, line);
    sec = sides[ line->sidenum[side^1]] .sector;
    // printf ("side = %d, sec = %d, tag = %d\n", side, sec-sectors, sec -> tag);
    if (sec -> tag)
    {
      linenum = -1;
      do
      {
	linenum = P_FindLineFromTag (sec->tag, linenum);
	if (linenum == -1)
	  break;

	line = lines+linenum;
	// printf ("Line %d operates sector, tag = %d, special = %d\n", linenum, line->tag, line->special);
	// printf ("Calling P_ShootSpecialLine\n");
	rc = P_ShootSpecialLine (usething, line);
	if (rc)
	  return (rc);
	// printf ("Calling P_CrossSpecialLine\n");
	rc = P_CrossSpecialLine (line, 0, usething);
	if (rc)
	  return (rc);
	// printf ("Calling P_UseSpecialLine\n");
	rc = P_UseSpecialLine (usething, line, 0);
	if (rc)
	  return (rc);
	// printf ("Nothing doing\n");
      } while (1);
    }

    /* Nothing doing via tags, try to see whether it is a door */
    /* that is operated from the other side with tag 0. */
    for (i = 0;i < sec->linecount;i++)
    {
      line = sec->lines[i];
      if ((line->special) && (line->tag == 0))
      {
	rc = P_ShootSpecialLine (usething, line);
	if (rc)
	  return (rc);
	rc = P_CrossSpecialLine (line, 0, usething);
	if (rc)
	  return (rc);
	rc = P_UseSpecialLine (usething, line, 0);
	if (rc)
	  return (rc);
      }
    }
  }

  return (false);
}


boolean	PTR_UseTraverse (intercept_t* in)
{
  int		side;
  line_t *	line;

  line = in->d.line;

  if (!line->special)
  {
      P_LineOpening (line);
      if (openrange <= 0)
      {
	  if (PTR_UseAnyway (line) == false)
	    S_StartSound (usething, sfx_noway);

	  // can't use through a wall
	  return (false);
      }
      // not a special line, but keep checking
      // PTR_UseAnyway (line);
      return (true);
  }

  side = P_PointOnLineSide (usething->x, usething->y, line);

  // can't use for more than one special line
  // in a row unless ML_PASSUSE is set.
  if ((P_UseSpecialLine (usething, line, side))
   && ((line -> flags & ML_PASSUSE) == 0))
    return (false);

  PTR_UseAnyway (line);
  return (true);
}


//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines (player_t*	player)
{
    int		angle;
    fixed_t	x1;
    fixed_t	y1;
    fixed_t	x2;
    fixed_t	y2;

    usething = player->mo;

    angle = player->mo->angle >> ANGLETOFINESHIFT;

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
    y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];

    P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
}


//
// RADIUS ATTACK
//
mobj_t*		bombsource;
mobj_t*		bombspot;
int		bombdamage;


//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
boolean PIT_RadiusAttack (mobj_t* thing)
{
    fixed_t     dx;
    fixed_t     dy;
    fixed_t     dz;
    fixed_t     dist;

    if (!(thing->flags & MF_SHOOTABLE))
	return (true);

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG
     || thing->type == MT_SPIDER)
	return (true);

    dx = ABS(thing->x - bombspot->x);
    dy = ABS(thing->y - bombspot->y);

    dist = MAX(dx, dy);
    dist -= thing->radius;

    if (thing->type == MT_BOSSBRAIN)
    {
	dist = MAX (0, dist >> FRACBITS);

	if (dist >= bombdamage)
	    return (true);	// out of range
    }
    else
    {
	dz = ABS (thing->z + (thing->height >> 1) - bombspot->z);
	dist = (dist > dz ? dist : dz);

	dist = MAX (0, dist >> FRACBITS);

	if (dist >= bombdamage)
	    return (true);	// out of range

	if (thing->floorz > bombspot->z && bombspot->ceilingz < thing->z)
	    return (true);

	if (thing->ceilingz < bombspot->z && bombspot->floorz > thing->z)
	    return (true);
    }

    if (P_CheckSight (thing, bombspot))
    {
	// must be in direct path
	P_DamageMobj (thing, bombspot, bombsource, bombdamage - dist);
    }

    return (true);
}


//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void
P_RadiusAttack
( mobj_t*	spot,
  mobj_t*	source,
  int		damage )
{
    int		x;
    int		y;

    int		xl;
    int		xh;
    int		yl;
    int		yh;

    fixed_t	dist;

    dist = (damage+MAXRADIUS)<<FRACBITS;
    yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
    yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
    xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
    xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
    bombspot = spot;
    bombsource = source;
    bombdamage = damage;

    for (y=yl ; y<=yh ; y++)
	for (x=xl ; x<=xh ; x++)
	    P_BlockThingsIterator (x, y, PIT_RadiusAttack );
}



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//
boolean		crushchange;
boolean		nofit;


//
// PIT_ChangeSector
//
boolean PIT_ChangeSector (mobj_t*	thing)
{
    mobj_t*	mo;

    if (P_ThingHeightClip (thing))
    {
	// keep checking
	return true;
    }


    // crunch bodies to giblets
    if ((thing->health <= 0)
	// [BH] since they explode, don't gib barrels,
	//  pain elementals or lost souls
	&& (thing->type != MT_BARREL)
	&& (thing->type != MT_SKULL)
	&& (thing->type != MT_PAIN))
    {
	P_SetMobjState (thing, S_GIBS);

	thing->flags &= ~MF_SOLID;
	thing->height = 0;
	thing->radius = 0;

	// keep checking
	return true;
    }

    // crunch dropped items
    if (thing->flags & MF_DROPPED)
    {
	/* In the wolfendoom conversions the baddies drop a key */
	/* when killed. We don't want these crushing in doors. */
	/* (E.g. Original/wad Map09) */
	if (thing->info)
	switch (thing -> sprite)  // These are identified by sprite number in p_inter.c
	{
	  case SPR_BKEY:
	  case SPR_YKEY:
	  case SPR_RKEY:
	  case SPR_BSKU:
	  case SPR_YSKU:
	  case SPR_RSKU:
	    nofit = true;
	    return true;
	}

	P_RemoveMobj (thing);

	// keep checking
	return true;
    }

    if (! (thing->flags & MF_SHOOTABLE) )
    {
	// assume it is bloody gibs or something
	return true;
    }

    nofit = true;

    if (crushchange && !(leveltime&3) )
    {
	int t;

	P_DamageMobj(thing,NULL,NULL,10);

	// spray blood in a random direction
	mo = P_SpawnMobj (thing->x,
			  thing->y,
			  thing->z + thing->height/2, MT_BLOOD);

	t = P_Random();	/* remove dependence on order of evaluation */
	mo->momx = (t - P_Random ())<<12;
	t = P_Random();	/* remove dependence on order of evaluation */
	mo->momy = (t - P_Random ())<<12;
    }

    // keep checking (crush other things)
    return true;
}


#ifndef USE_BOOM_P_ChangeSector
//
// P_ChangeSector
//

boolean
P_ChangeSector
( sector_t*	sector,
  boolean	crunch )
{
    int	x;
    int	y;

    nofit = false;
    crushchange = crunch;

    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
	for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
	    P_BlockThingsIterator (x, y, PIT_ChangeSector);


    crushchange = false;
    return nofit;
}

#else
// P_ChangeSector
// jff 3/19/98 added to just check monsters on the periphery
// of a moving sector instead of all in bounding box of the
// sector. Both more accurate and faster.
// [BH] renamed from P_CheckSector to P_ChangeSector to replace old one entirely
//
boolean P_ChangeSector (sector_t *sector, boolean crunch)
{
    mobj_t* mobj;
    msecnode_t *n;

    nofit = false;
    crushchange = crunch;

    // killough 4/4/98: scan list front-to-back until empty or exhausted,
    // restarting from beginning after each thing is processed. Avoids
    // crashes, and is sure to examine all things in the sector, and only
    // the things which are in the sector, until a steady-state is reached.
    // Things can arbitrarily be inserted and removed and it won't mess up.
    //
    // killough 4/7/98: simplified to avoid using complicated counter

    // Mark all things invalid
    for (n = sector->touching_thinglist; n; n = n->m_snext)
	n->visited = false;

    do
	for (n = sector->touching_thinglist; n; n = n->m_snext)	// go through list
	    if (!n->visited)					// unprocessed thing found
	    {
		n->visited = true;				// mark thing as processed
		mobj = n->m_thing;
		if ((mobj)
		 && (!(mobj->flags & MF_NOBLOCKMAP)))		// jff 4/7/98 don't do these
		{
		  PIT_ChangeSector(mobj);			// process it
		  break;					// exit and start over
		}
	    }
    while (n);		// repeat from scratch until all things left are marked valid

    crushchange = false;
    return nofit;
}

// phares 3/21/98
//
// Maintain a freelist of msecnode_t's to reduce memory allocs and frees.
msecnode_t *headsecnode;

// Temporary holder for thing_sectorlist threads
msecnode_t *sector_list;			// phares 3/16/98


// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
//

static msecnode_t *P_GetSecnode(void)
{
  msecnode_t *node;

  node = headsecnode;
  if (node == NULL)
    node = Z_Malloc (sizeof *node, PU_LEVMAP, NULL);
  else
    headsecnode = node->m_snext;

  return (node);
}

// P_PutSecnode() returns a node to the freelist.
static void P_PutSecnode(msecnode_t *node)
{
    node->m_snext = headsecnode;
    headsecnode = node;
}

// phares 3/16/98
//
// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
//
// killough 11/98: reformatted
static msecnode_t *P_AddSecnode(sector_t *s, mobj_t *thing, msecnode_t *nextnode)
{
    msecnode_t *node;

    for (node = nextnode; node; node = node->m_tnext)
	if (node->m_sector == s)	// Already have a node for this sector?
	{
	    node->m_thing = thing;	// Yes. Setting m_thing says 'keep it'.
	    return nextnode;
	}

    // Couldn't find an existing node for this sector. Add one at the head
    // of the list.
    node = P_GetSecnode();

    node->visited = false;	// killough 4/4/98, 4/7/98: mark new nodes unvisited.

    node->m_sector = s;		// sector
    node->m_thing = thing;	// mobj
    node->m_tprev = NULL;	// prev node on Thing thread
    node->m_tnext = nextnode;	// next node on Thing thread

    if (nextnode)
	nextnode->m_tprev = node;	// set back link on Thing

    // Add new node at head of sector thread starting at s->touching_thinglist
    node->m_sprev = NULL;			// prev node on sector thread
    node->m_snext = s->touching_thinglist;	// next node on sector thread
    if (s->touching_thinglist)
	node->m_snext->m_sprev = node;
    return s->touching_thinglist = node;
}

// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
//
// killough 11/98: reformatted
static msecnode_t *P_DelSecnode (msecnode_t *node)
{
  if (node)
  {
    msecnode_t *tp = node->m_tprev;	// prev node on thing thread
    msecnode_t *tn = node->m_tnext;	// next node on thing thread
    msecnode_t *sp;			// prev node on sector thread
    msecnode_t *sn;			// next node on sector thread

    // Unlink from the Thing thread. The Thing thread begins at
    // sector_list and not from mobj_t->touching_sectorlist.
    if (tp)
	tp->m_tnext = tn;

    if (tn)
	tn->m_tprev = tp;

    // Unlink from the sector thread. This thread begins at
    // sector_t->touching_thinglist.
    sp = node->m_sprev;
    sn = node->m_snext;

    if (sp)
	sp->m_snext = sn;
    else
	node->m_sector->touching_thinglist = sn;

    if (sn)
	sn->m_sprev = sp;

    // Return this node to the freelist
    P_PutSecnode(node);

    node = tn;
  }
  return node;
}

// Delete an entire sector list
void P_DelSeclist(msecnode_t *node)
{
    while (node)
	node = P_DelSecnode(node);
}

// phares 3/14/98
//
// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.
static boolean PIT_GetSectors(line_t *ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT] ||
	tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT] ||
	tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM] ||
	tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
	return true;

    if (P_BoxOnLineSide(tmbbox, ld) != -1)
	return true;

    // This line crosses through the object.

    // Collect the sector(s) from the line and add to the
    // sector_list you're examining. If the Thing ends up being
    // allowed to move to this position, then the sector_list
    // will be attached to the Thing's mobj_t at touching_sectorlist.
    sector_list = P_AddSecnode(ld->frontsector, tmthing, sector_list);

    // Don't assume all lines are 2-sided, since some Things
    // like MT_TFOG are allowed regardless of whether their radius takes
    // them beyond an impassable linedef.

    // killough 3/27/98, 4/4/98:
    // Use sidedefs instead of 2s flag to determine two-sidedness.
    // killough 8/1/98: avoid duplicate if same sector on both sides
    if (ld->backsector && ld->backsector != ld->frontsector)
	sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

    return true;
}

// phares 3/14/98
//
// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.
//
// killough 11/98: reformatted
void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y)
{
    int xl, xh, yl, yh, bx, by;
    msecnode_t*	node;
    mobj_t*	saved_tmthing;
    fixed_t	saved_tmx;
    fixed_t	saved_tmy;

    // First, clear out the existing m_thing fields. As each node is
    // added or verified as needed, m_thing will be set properly. When
    // finished, delete all nodes where m_thing is still NULL. These
    // represent the sectors the Thing has vacated.
    for (node = sector_list; node; node = node->m_tnext)
	node->m_thing = NULL;

    saved_tmthing = tmthing;	// Save the current tmthing before
    saved_tmx = tmx;		// we clobber it.
    saved_tmy = tmy;

    tmthing = thing;
    tmflags = thing->flags;

    tmx = x;
    tmy = y;

    tmbbox[BOXTOP] = y + tmthing->radius;
    tmbbox[BOXBOTTOM] = y - tmthing->radius;
    tmbbox[BOXRIGHT] = x + tmthing->radius;
    tmbbox[BOXLEFT] = x - tmthing->radius;

    validcount++;	// used to make sure we only process a line once

    xl = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; bx++)
	for (by = yl; by <= yh; by++)
	    P_BlockLinesIterator(bx, by, PIT_GetSectors);

    // Add the sector of the (x,y) point to sector_list.
    sector_list = P_AddSecnode(thing->subsector->sector, thing, sector_list);

    // Now delete any nodes that won't be used. These are the ones where
    // m_thing is still NULL.
    for (node = sector_list; node;)
	if (node->m_thing == NULL)
	{
	    if (node == sector_list)
		sector_list = node->m_tnext;
	    node = P_DelSecnode(node);
	}
	else
	    node = node->m_tnext;

    // Restore tmthing....

    tmx = saved_tmx;
    tmy = saved_tmy;
    if ((tmthing = saved_tmthing) != NULL)
    {
      tmflags = tmthing->flags;
      tmbbox[BOXTOP] = tmy + tmthing->radius;
      tmbbox[BOXBOTTOM] = tmy - tmthing->radius;
      tmbbox[BOXRIGHT] = tmx + tmthing->radius;
      tmbbox[BOXLEFT] = tmx - tmthing->radius;
    }
}

#endif
