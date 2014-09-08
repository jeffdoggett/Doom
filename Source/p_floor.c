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
//	Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_floor.c,v 1.4 1997/02/03 16:47:54 b1 Exp $";
#endif


#include "includes.h"


#if 0
boolean P_CheckSector(sector_t *sector, boolean crunch);
# define P_ChangeSectorFunc(s,c)	P_CheckSector(s,c)
#else
# define P_ChangeSectorFunc(s,c)	P_ChangeSector(s,c)
#endif

//-----------------------------------------------------------------------------
//
// FLOORS
//

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e
T_MovePlane
( sector_t*	sector,
  fixed_t	speed,
  fixed_t	dest,
  boolean	crush,
  int		floorOrCeiling,
  int		direction )
{
  boolean	flag;
  fixed_t	lastpos;

  switch(floorOrCeiling)
  {
    case 0:
      // FLOOR
      switch(direction)
      {
	case -1:
	  // DOWN
	  if (sector->floorheight - speed < dest)
	  {
	    lastpos = sector->floorheight;
	    sector->floorheight = dest;
	    flag = P_ChangeSectorFunc(sector,crush);
#if 0
	    /* Removed by JAD 13/10/2011 because a floor */
	    /* going downwards cannot possibly crush anything */
	    if (flag == true)
	    {
		sector->floorheight =lastpos;
		P_ChangeSectorFunc(sector,crush);
		//return crushed;
	    }
#endif
	    return pastdest;
	  }
	  else
	  {
	    lastpos = sector->floorheight;
	    sector->floorheight -= speed;
	    flag = P_ChangeSectorFunc(sector,crush);
#if 0
	    if (flag == true)
	    {
		sector->floorheight = lastpos;
		P_ChangeSectorFunc(sector,crush);
		return crushed;
	    }
#endif
	  }
	  break;

	case 1:
	  // UP
	  if (sector->floorheight + speed > dest)
	  {
	    lastpos = sector->floorheight;
	    sector->floorheight = dest;
	    flag = P_ChangeSectorFunc(sector,crush);
	    if (flag == true)
	    {
		sector->floorheight = lastpos;
		P_ChangeSectorFunc(sector,crush);
		//return crushed;
	    }
	    return pastdest;
	  }
	  else
	  {
	    // COULD GET CRUSHED
	    lastpos = sector->floorheight;
	    sector->floorheight += speed;
	    flag = P_ChangeSectorFunc(sector,crush);
	    if (flag == true)
	    {
	      if (crush == true)
		  return crushed;
	      sector->floorheight = lastpos;
	      P_ChangeSectorFunc(sector,crush);
	      return crushed;
	    }
	  }
	  break;
      }
      break;

    case 1:
      // CEILING
      switch(direction)
      {
	case -1:
	  // DOWN
	  if (sector->ceilingheight - speed < dest)
	  {
	    lastpos = sector->ceilingheight;
	    sector->ceilingheight = dest;
	    flag = P_ChangeSectorFunc(sector,crush);

	    if (flag == true)
	    {
		sector->ceilingheight = lastpos;
		P_ChangeSectorFunc(sector,crush);
		//return crushed;
	    }
	    return pastdest;
	  }
	  else
	  {
	    // COULD GET CRUSHED
	    lastpos = sector->ceilingheight;
	    sector->ceilingheight -= speed;
	    flag = P_ChangeSectorFunc(sector,crush);

	    if (flag == true)
	    {
	      if (crush == true)
		  return crushed;
	      sector->ceilingheight = lastpos;
	      P_ChangeSectorFunc(sector,crush);
	      return crushed;
	    }
	  }
	  break;

	case 1:
	  // UP
	  if (sector->ceilingheight + speed > dest)
	  {
	    lastpos = sector->ceilingheight;
	    sector->ceilingheight = dest;
	    flag = P_ChangeSectorFunc(sector,crush);
#if 0
	    /* Removed by JAD 13/10/2011 because a ceiling */
	    /* going upwards cannot possibly crush anything */
	    if (flag == true)
	    {
	      sector->ceilingheight = lastpos;
	      P_ChangeSectorFunc(sector,crush);
	      //return crushed;
	    }
#endif
	    return pastdest;
	  }
	  else
	  {
	    lastpos = sector->ceilingheight;
	    sector->ceilingheight += speed;
	    flag = P_ChangeSectorFunc(sector,crush);
// UNUSED
#if 0
	    if (flag == true)
	    {
	      sector->ceilingheight = lastpos;
	      P_ChangeSectorFunc(sector,crush);
	      return crushed;
	    }
#endif
	  }
	  break;
      }
      break;

  }
  return ok;
}


//-----------------------------------------------------------------------------
//
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* floor)
{
  result_e	res;
  sector_t *	sec;

  sec = floor->sector;

  res = T_MovePlane(sec,
		    floor->speed,
		    floor->floordestheight,
		    floor->crush,0,floor->direction);


  if (res != pastdest)
  {
    if (!(leveltime&7))
      S_StartSound((mobj_t *)&sec->soundorg, sfx_stnmov);
  }
  else
  {
    sec->floordata = NULL;
#ifdef SHOW_FLOOR_TEX_CHANGE
    printf ("Floor spec %d -> %d, pic %d -> %d\n",
	      sec->special, floor->newspecial,
	      sec->floorpic, floor->newtexture);
#endif
    sec->special = floor->newspecial;			// We have aready set these to the
    sec->floorpic = floor->newtexture;			// correct special/texture so we can
							// just write them always.
    P_RemoveThinker(&floor->thinker);
    S_StartSound((mobj_t *)&sec->soundorg, sfx_pstop);
  }
}

//-----------------------------------------------------------------------------

static unsigned int find_shortest_lower_texture (sector_t* sec, int secnum)
{
  unsigned int	i;
  unsigned int	offset;
  side_t*	side;

  offset = ~0;
  for (i = 0; i < sec->linecount; i++)
  {
    if (twoSided (secnum, i) )
    {
      side = getSide(secnum,i,0);
      if (side->bottomtexture >= 0)
	if (textureheight[side->bottomtexture] < offset)
	    offset = textureheight[side->bottomtexture];
      side = getSide(secnum,i,1);
      if (side->bottomtexture >= 0)
	if (textureheight[side->bottomtexture] < offset)
	  offset = textureheight[side->bottomtexture];
    }
  }

  if (offset == ~0)
    offset = 0;
  return (offset);
}

//-----------------------------------------------------------------------------

static void queue_do_change_texture_or_special (floormove_t* floor, sector_t* sec, unsigned int mode)
{
  switch (mode)
  {
    case FChgZero:	// Zero
      floor->newspecial = 0;
      floor->newtexture = sec->floorpic;
      break;

    case FChgTxt:	// Texture only
      floor->newtexture = sec->floorpic;
      break;

    case FChgTyp:	// both
      floor->newspecial = sec->special;
      floor->newtexture = sec->floorpic;
      break;
  }
}

//-----------------------------------------------------------------------------

static void queue_change_texture_or_special (floormove_t* floor, int secnum, unsigned int mode)
{
  sector_t* sec;

  sec = P_FindModelFloorSector (floor->floordestheight, secnum);
  if (sec)
    queue_do_change_texture_or_special (floor, sec, mode);
}

//-----------------------------------------------------------------------------
/*
  Init a floor movement structure to some safe defaults.
*/

static floormove_t* get_floor_block (sector_t* sec)
{
  floormove_t* floor;

  floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
  P_AddThinker (&floor->thinker, (actionf_p1) T_MoveFloor);

  sec->floordata = floor;
  floor->sector = sec;
  floor->newtexture = sec->floorpic;	// Copy current texture/special so that we
  floor->newspecial = sec->special;	// can just copy back later without checking.
  floor->direction = 1;
  floor->speed = FLOORSPEED;
  floor->floordestheight = sec->floorheight;
  floor->crush = false;
//floor->type = buildStair;
  return (floor);
}

//-----------------------------------------------------------------------------
//
// HANDLE FLOOR TYPES
//
int
EV_DoFloor
( line_t*	line,
  floor_e	floortype )
{
  int		secnum;
  int		rtn;
  int		itype;
  unsigned int	mode;
  unsigned int	offset;
  fixed_t	newheight;
  sector_t*	sec;
  floormove_t*	floor;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

#if 0
    if (P_SectorActive(floor_special, sec))
	continue;
#else
    // ALREADY MOVING?  IF SO, KEEP GOING...
    if (sec->floordata)
	continue;
#endif
    // new floor thinker
    rtn = 1;
    floor = get_floor_block (sec);
//  floor->type = floortype;

    switch(floortype)
    {
      case lowerFloor:
	floor->direction = -1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindHighestFloorSurrounding(sec);
	break;

      case lowerFloorToLowest:
	floor->direction = -1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindLowestFloorSurrounding(sec);
	break;

      case turboLower:
	floor->direction = -1;
	floor->speed = FLOORSPEED * 4;
	floor->floordestheight = P_FindHighestFloorSurrounding(sec);
	if (floor->floordestheight != sec->floorheight)
	    floor->floordestheight += 8*FRACUNIT;
	break;

      case raiseFloorCrush:
	floor->crush = true;
      case raiseFloor:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindLowestCeilingSurrounding(sec);
	if (floor->floordestheight > sec->ceilingheight)
	    floor->floordestheight = sec->ceilingheight;
	floor->floordestheight -= (8*FRACUNIT)*
	    (floortype == raiseFloorCrush);
	break;

      case raiseFloorTurbo:
//	floor->direction = 1;
	floor->speed = FLOORSPEED*4;
	floor->floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
	break;

      case raiseFloorToNearest:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
	break;

      case raiseFloorToHighest:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindHighestFloorSurrounding(sec);
	break;

      case raiseFloor24:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = floor->sector->floorheight + (24 * FRACUNIT);
	break;

      case raiseFloor512:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = floor->sector->floorheight + (512 * FRACUNIT);
	break;

      case changeAndRaiseFloor24:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = floor->sector->floorheight + (24 * FRACUNIT);
	/* The change is done *first* before the floor is moved. */
	/* I guess that if the change is required afterwards then */
	/* the Boom Fby24 would be used. */
	floor->newtexture = sec->floorpic = line->frontsector->floorpic;
	floor->newspecial = sec->special = line->frontsector->special;
	break;

      case raiseToTexture:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = floor->sector->floorheight + find_shortest_lower_texture (sec, secnum);
      break;

      case raiseToCeiling:
//	floor->direction = 1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = floor->sector->ceilingheight;
	break;

      case lowerAndChange:
	floor->direction = -1;
//	floor->speed = FLOORSPEED;
	floor->floordestheight = P_FindLowestFloorSurrounding(sec);
	queue_change_texture_or_special (floor, secnum, 3);
	break;

      case lowerFloor24:
//	    if (!demo_compatibility)
	{
	  floor->direction = -1;
//	  floor->speed = FLOORSPEED;
	  floor->floordestheight = floor->sector->floorheight - (24 * FRACUNIT);
	}
	break;

      case lowerFloor32Turbo:
//	    if (!demo_compatibility)
	{
	  floor->direction = -1;
	  floor->speed = FLOORSPEED*4;
	  floor->floordestheight = floor->sector->floorheight - (32*FRACUNIT);
	}
	break;

      case lowerFloorToNearest:
//	    if (!demo_compatibility)
	{
	  floor->direction = -1;
//	  floor->speed = FLOORSPEED;
	  floor->floordestheight = P_FindNextLowestFloor(sec, sec->floorheight);
	}
	break;

      case raiseFloor32Turbo:
//	    if (!demo_compatibility)
	{
//	  floor->direction = 1;
	  floor->speed = FLOORSPEED*4;
	  floor->floordestheight = floor->sector->floorheight + (32*FRACUNIT);
	}
	break;

      default:
	if ((floortype >= GenFloorBase)
	 && (floortype < 0x8000))
	{
	  itype = (int) floortype;
#if 0
	  printf ("Boom %X (%u %X %X %d)\n", itype,
				(itype >> FloorTargetShift) & 7,
				(itype & FloorDirection),
				(itype & FloorCrush),
				(itype >> FloorChangeShift) & 3);
#endif
	  offset = 0;
	  switch ((itype >> FloorTargetShift) & 7)
	  {
	    case FtoHnF:						// HNF
	      newheight = P_FindHighestFloorSurrounding(sec);
	      break;
	    case FtoLnF:						// LNF
	      newheight = P_FindLowestFloorSurrounding(sec);
	      break;
	    case FtoNnF:						// NNF
	      if (itype & FloorDirection)
		newheight = P_FindNextHighestFloor (sec,sec->floorheight);
	      else
		newheight = P_FindNextLowestFloor (sec,sec->floorheight);
	      break;
	    case FtoLnC:						// LnC
	      newheight = P_FindLowestCeilingSurrounding(sec);
	      break;
	    case FtoC:
	      newheight = sec->ceilingheight;
	      break;
	    case FbyST:
	      offset = find_shortest_lower_texture (sec, secnum);
	      newheight = floor->sector->floorheight;
	      break;
	    case Fby24:						// 24
	      offset = (24*FRACUNIT);
	      newheight = floor->sector->floorheight;
	      break;
	    default:/* Fby32: */				// 32
	      offset = (32*FRACUNIT);
	      newheight = floor->sector->floorheight;
	  }

	  floor->speed = (FLOORSPEED) << ((itype >> FloorSpeedShift) & 3);

	  if (itype & FloorDirection)
	  {
//	    floor->direction = 1;
	    floor->floordestheight = (newheight + offset);
	  }
	  else
	  {
	    floor->direction = -1;
	    floor->floordestheight = (newheight - offset);
	  }

	  if (itype & FloorCrush)
	    floor->crush = true;

	  mode = (itype >> FloorChangeShift) & 3;
	  if (mode)
	  {
	    if ((itype & FloorModel) == 0)
	    {
	      queue_do_change_texture_or_special (floor, line->frontsector, mode);
	    }
	    else
	    {
	      switch ((itype >> FloorTargetShift) & 7)
	      {
		case FtoLnC:
		case FtoC:
		  sec = P_FindModelCeilingSector (floor->floordestheight, secnum);
		  if (sec)
		    queue_do_change_texture_or_special (floor, sec, mode);
		  break;
		default:
		  queue_change_texture_or_special (floor, secnum, mode);
	      }
	    }
	  }
	}
	break;
    }
  }
  return (rtn);
}

//-----------------------------------------------------------------------------
/*
  We need to check first all of the sectors that comprise the staircase
  to ensure that none of them are already moving, especially in the case of
  retriggerable stairs.
*/

static boolean make_stairs (sector_t * sec, stair_e buildtype, boolean doit)
{
  int		i;
  int		ok;
  int		height;
  int		texture;
  int		direction;
  int		stairlock;
  line_t*	sline;
  sector_t*	tsec;
  fixed_t	speed;
  fixed_t	stairsize;
  floormove_t*	floor;

  stairlock = sec -> stairlock + 1;	// Magic number

  if (doit == false)
  {
    if (sec -> floordata)
      return (false);
  }
  else
  {
    switch ((((int)buildtype) >> StairStepShift) & 3)
    {
      case 0:  stairsize=FRACUNIT*4; break;
      case 1:  stairsize=FRACUNIT*8; break;
      case 2:  stairsize=FRACUNIT*16; break;
      default: stairsize=FRACUNIT*24; break;
    }

    switch ((((int) buildtype) >> StairSpeedShift) & 3)
    {
      case 0:  speed = FLOORSPEED/8; break;
      case 1:  speed = FLOORSPEED/4; break;
      case 2:  speed = FLOORSPEED;   break;
      default: speed = FLOORSPEED*4; break;
    }

    if ((((int) buildtype) & StairDirection) == 0)	// Down?
    {
      stairsize = -stairsize;
      direction = -1;
    }
    else
    {
      direction = 1;
    }

    height = sec->floorheight + stairsize;

    // new floor thinker
    if (sec -> floordata == NULL)
    {
      floor = get_floor_block (sec);
      floor->direction = direction;
      floor->speed = speed;
      floor->floordestheight = height;
      if (floor->floordestheight > sec->ceilingheight)
	floor->floordestheight = sec->ceilingheight;
    }
    height += stairsize;
  }

  sec -> stairlock = stairlock;
  texture = sec->floorpic;

  // Find next sector to raise
  // 1.	Find 2-sided line with same sector side[0]
  // 2.	Other side is the next sector to raise
  do
  {
    ok = 0;
    for (i = 0;i < sec->linecount;i++)
    {
      sline = sec->lines[i];
      if ( !(sline->flags & ML_TWOSIDED) )
	continue;

      if (sec != sline->frontsector)
	continue;

      tsec = sline->backsector;

      if ((((int)buildtype & StairIgnore) == 0)
       && (tsec->floorpic != texture))
	continue;

      /* All this complexity is to cope with TNT map 30. */
      /* We need to recreate the buggy behavior of 1.9 */
      /* while preventing re-triggerable stairs from */
      /* being actioned again whilst they're still moving. */

      if (doit == false)
      {
	if (tsec -> stairlock == stairlock)
	  continue;

	if (tsec->floordata)
	  return (false);

	sec = tsec;
      }
      else
      {
	if (tsec -> stairlock == stairlock)	// floordata will be non zero here as
	{					// well if the stairlock matches.
	  height += stairsize;
	  continue;
	}

	sec = tsec;

	// new floor thinker
	floor = get_floor_block (sec);
	floor->direction = direction;
	floor->speed = speed;
	floor->floordestheight = height;
	if (floor->floordestheight > sec->ceilingheight)
	  floor->floordestheight = sec->ceilingheight;
	height += stairsize;
      }

      sec -> stairlock = stairlock;
      ok = 1;
      break;
    }
  } while (ok);

  return (true);
}

//-----------------------------------------------------------------------------
//
// BUILD A STAIRCASE!
//

int
EV_BuildStairs
( line_t*	line,
  stair_e	buildtype)
{
  int		rtn;
  int		secnum;
  sector_t*	sec;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    sec = &sectors [secnum];
    if (make_stairs (sec, buildtype, false) == true)	// Can we do it?
    {
      (void) make_stairs (sec, buildtype, true);	// Yes.
      rtn = 1;
    }
  }

  return (rtn);
}

//-----------------------------------------------------------------------------
//
// Collapse a donut
//
int EV_DoDonut(line_t*	line)
{
  sector_t*	s1;
  sector_t*	s2;
  sector_t*	s3;
  line_t*	line2;
  int		secnum;
  int		rtn;
  int		i;
  floormove_t*	floor;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    s1 = &sectors[secnum];

#if 0
    if (P_SectorActive (floor_special, s1))
	continue;
#else
    // ALREADY MOVING?  IF SO, KEEP GOING...
    if (s1->floordata)
	continue;
#endif
    s2 = getNextSector (s1->lines[0], s1);
    if (s2 == NULL)					// JAD 01/03/14
      continue;

    for (i = 0;i < s2->linecount;i++)
    {
      line2 = s2->lines[i];
      if (((line2->flags & ML_TWOSIDED) == 0)		// Fix here JAD 01/03/14
       || ((s3 = line2->backsector) == NULL)
       || (s3 == s1))
	continue;

      //	Spawn rising slime
      floor = get_floor_block (s2);
//    floor->type = donutRaise;
      floor->speed = FLOORSPEED / 2;
      floor->newtexture = s3->floorpic;
      floor->newspecial = 0;
      floor->floordestheight = s3->floorheight;

      //	Spawn lowering donut-hole
      floor = get_floor_block (s1);
//    floor->type = lowerFloor;
      floor->direction = -1;
      floor->speed = FLOORSPEED / 2;
      floor->floordestheight = s3->floorheight;

      rtn = 1;
      break;
    }
  }
  return rtn;
}

/* ---------------------------------------------------------------------------- */

int EV_DoChange
( line_t*	line,
  change_e	changetype )
{
  int		secnum;
  int		rtn;
  sector_t*	sec;
  sector_t*	secm;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    /* change all sectors with the same tag as the linedef */
    sec = &sectors[secnum];

    rtn = 1;

    /* handle trigger or numeric change type */
    switch(changetype)
    {
      case trigChangeOnly:
	sec->floorpic = line->frontsector->floorpic;
	sec->special = line->frontsector->special;
	sec->oldspecial = line->frontsector->oldspecial;
	break;
      case numChangeOnly:
	secm = P_FindModelFloorSector(sec->floorheight,secnum);
	if (secm) /* if no model, no change */
	{
	  sec->floorpic = secm->floorpic;
	  sec->special = secm->special;
	  sec->oldspecial = secm->oldspecial;
	}
	break;
      default:
	break;
    }
  }
  return rtn;
}

//-----------------------------------------------------------------------------

int EV_DoElevator
( line_t* line,
  elevator_e    elevtype )
{
  int		secnum;
  int		rtn;
  sector_t*	sec;
  elevator_t*	elevator;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    /* act on all sectors with the same tag as the triggering linedef */
    sec = &sectors[secnum];

    /* If either floor or ceiling is already activated, skip it */
    if (sec->floordata || sec->ceilingdata) /*jff 2/22/98 */
      continue;

    /* create and initialize new elevator thinker */
    rtn = 1;
    elevator = Z_Malloc (sizeof(*elevator), PU_LEVSPEC, 0);
    P_AddThinker (&elevator->thinker, (actionf_p1) T_MoveElevator);
    sec->floordata = elevator; /*jff 2/22/98 */
    sec->ceilingdata = elevator; /*jff 2/22/98 */
    elevator->type = elevtype;
    elevator->sector = sec;
    elevator->direction = 1;
    elevator->speed = ELEVATORSPEED;

    /* set up the fields according to the type of elevator action */
    switch(elevtype)
    {
	/* elevator down to next floor */
      case elevateDown:
	elevator->direction = -1;
//	elevator->speed = ELEVATORSPEED;
	elevator->floordestheight = P_FindNextLowestFloor(sec,sec->floorheight);
	break;

	/* elevator up to next floor */
      case elevateUp:
//	elevator->direction = 1;
//	elevator->speed = ELEVATORSPEED;
	elevator->floordestheight = P_FindNextHighestFloor(sec,sec->floorheight);
	break;

	/* elevator to floor height of activating switch's front sector */
      case elevateCurrent:
//	elevator->speed = ELEVATORSPEED;
	elevator->floordestheight = line->frontsector->floorheight;
	elevator->direction = elevator->floordestheight>sec->floorheight?  1 : -1;
	break;

      default:
	break;
    }

    elevator->ceilingdestheight = elevator->floordestheight + sec->ceilingheight - sec->floorheight;
  }
  return rtn;
}

//-----------------------------------------------------------------------------

void T_MoveElevator (elevator_t* elevator)
{
  result_e      res;

  //BOOMTRACEOUT("T_MoveElevator")

  if (elevator->direction<0)      /* moving down */
  {
    res = T_MovePlane	     /*jff 4/7/98 reverse order of ceiling/floor */
    (
      elevator->sector,
      elevator->speed,
      elevator->ceilingdestheight,
      false,
      1,			  /* move floor */
      elevator->direction
    );
    if (res==ok || res==pastdest) /* jff 4/7/98 don't move ceil if blocked */
      T_MovePlane
      (
	elevator->sector,
	elevator->speed,
	elevator->floordestheight,
	false,
	0,			/* move ceiling */
	elevator->direction
      );
  }
  else /* up */
  {
    res = T_MovePlane	     /*jff 4/7/98 reverse order of ceiling/floor */
    (
      elevator->sector,
      elevator->speed,
      elevator->floordestheight,
      false,
      0,			  /* move ceiling */
      elevator->direction
    );
    if (res==ok || res==pastdest) /* jff 4/7/98 don't move floor if blocked */
      T_MovePlane
      (
	elevator->sector,
	elevator->speed,
	elevator->ceilingdestheight,
	false,
	1,			/* move floor */
	elevator->direction
      );
  }


  if (res != pastdest)	    /* if destination height acheived */
  {
    /* make floor move sound */
    if (!(leveltime&7))
      S_StartSound((mobj_t *)&elevator->sector->soundorg, sfx_stnmov);
  }
  else
  {
    elevator->sector->floordata = NULL;     /*jff 2/22/98 */
    elevator->sector->ceilingdata = NULL;   /*jff 2/22/98 */
    /*if (demo_compatibility) elevator->sector->lightingdata = NULL; TEST */
    P_RemoveThinker(&elevator->thinker);    /* remove elevator from actives */

    /* make floor stop sound */
    S_StartSound((mobj_t *)&elevator->sector->soundorg, sfx_pstop);
  }
}

//-----------------------------------------------------------------------------
