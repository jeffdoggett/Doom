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
// DESCRIPTION:  Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_ceilng.c,v 1.4 1997/02/03 16:47:53 b1 Exp $";
#endif


#include "includes.h"

static int P_ActivateInStasisCeiling (line_t* line);

//-----------------------------------------------------------------------------
//
// CEILINGS
//


ceiling_t* activeceilingshead;


//-----------------------------------------------------------------------------

static void do_change_texture_or_special (sector_t* ceilingsector, sector_t* sec, unsigned int mode)
{
  switch (mode)
  {
    case CChgZero:	// Zero
      ceilingsector->special    = 0;
      ceilingsector->ceilingpic = sec->ceilingpic;
      break;

    case CChgTxt:	// Texture only
      ceilingsector->ceilingpic = sec->ceilingpic;
      break;

    case CChgTyp:	// both
      ceilingsector->special	= sec->special;
      ceilingsector->ceilingpic = sec->ceilingpic;
      break;
  }
}

//-----------------------------------------------------------------------------

static void change_texture_or_special (ceiling_t* ceiling)
{
  unsigned int mode;

  mode = ((int)ceiling->type >> CeilingChangeShift) & 3;
  do_change_texture_or_special (ceiling->sector, ceiling->secspecial, mode);
}

//-----------------------------------------------------------------------------
//
// T_MoveCeiling
//

void T_MoveCeiling (ceiling_t* ceiling)
{
  result_e	res;

  switch(ceiling->direction)
  {
//  case 0:	// IN STASIS
//    break;

    case 1:	// UP
      res = T_MoveCeilingPlane (ceiling->sector, ceiling->speed, ceiling->topheight, false, ceiling->direction);

      if (res != pastdest)
      {
	if (!(leveltime&7))
	{
	  switch(ceiling->type)
	  {
	    case silentCrushAndRaise:
	      break;
	    default:
	      S_StartSound((mobj_t *)&ceiling->sector->soundorg,sfx_stnmov);
	      // ?
	      break;
	  }
	}
      }
      else
      {
	switch(ceiling->type)
	{
	  case raiseToHighest:
	    P_RemoveActiveCeiling(ceiling);
	    break;

	  case silentCrushAndRaise:
	    S_StartSound((mobj_t *)&ceiling->sector->soundorg, sfx_pstop);

	  case fastCrushAndRaise:
	  case crushAndRaise:
	    if (netgame || (!gamekeydown['`'])) 	// Stay open cheat
	      ceiling->direction = -1;
	    else
	      P_RemoveActiveCeiling(ceiling);
	    break;

	  default:
	    if ((int)ceiling->type >= GenCeilingBase)
	    {
	      change_texture_or_special (ceiling);
	      P_RemoveActiveCeiling(ceiling);
	    }
	    break;
	}
      }
      break;

    case -1:	// DOWN
      res = T_MoveCeilingPlane (ceiling->sector, ceiling->speed, ceiling->bottomheight, ceiling->crush,ceiling->direction);
      if (res != pastdest)
      {
	if (!(leveltime&7))
	{
	  switch(ceiling->type)
	  {
	    case silentCrushAndRaise: break;
	    default:
	      S_StartSound((mobj_t *)&ceiling->sector->soundorg,sfx_stnmov);
	  }
	}

	if (res == crushed)
	{
	  if ((ceiling->type == lowerAndCrush) || (ceiling -> crush))
	  {
	    ceiling->speed = CEILSPEED / 8;
	  }
	}
      }
      else
      {
	switch(ceiling->type)
	{
	  case silentCrushAndRaise:
	    S_StartSound((mobj_t *)&ceiling->sector->soundorg,
			 sfx_pstop);
	  case crushAndRaise:
	    ceiling->speed = CEILSPEED;
	  case fastCrushAndRaise:
	    ceiling->direction = 1;
	    break;

	  case lowerAndCrush:
	  case lowerToFloor:
	    P_RemoveActiveCeiling(ceiling);
	    break;

	  default:
	    if ((int)ceiling->type >= GenCeilingBase)
	    {
	      change_texture_or_special (ceiling);
	      P_RemoveActiveCeiling(ceiling);
	    }
	    break;
	}
      }
      break;
  }
}

//-----------------------------------------------------------------------------

ceiling_t* P_NewCeilingAction (sector_t* sec, ceiling_e type)
{
  ceiling_t*	ceiling;

  ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
  P_AddThinker (&ceiling->thinker, (actionf_p1)T_MoveCeiling);
  sec->ceilingdata = ceiling;
  ceiling->sector = sec;
  ceiling->crush = false;
  ceiling->secspecial = sec;
  ceiling->direction = 1;
  ceiling->speed = CEILSPEED;
  ceiling->topheight = sec->ceilingheight;
  ceiling->bottomheight = sec->floorheight;
  ceiling->tag = sec->tag;
  ceiling->type = type;
  P_AddActiveCeiling (ceiling);
  return (ceiling);
}

//-----------------------------------------------------------------------------
//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
int
EV_DoCeiling
( line_t*	line,
  ceiling_e	type )
{
  unsigned int	i;
  unsigned int	mode;
  unsigned int	offset;
  int		secnum;
  int		rtn;
  int		itype;
  fixed_t	newheight;
  side_t*	side;
  sector_t*	sec;
  ceiling_t*	ceiling;


  //	Reactivate in-stasis ceilings...for certain types.
#if 1
  rtn = P_ActivateInStasisCeiling (line);
#else
  rtn = 0;
  switch(type)
  {
    case fastCrushAndRaise:
    case silentCrushAndRaise:
    case crushAndRaise:
      rtn = P_ActivateInStasisCeiling (line);
    default:
      break;
  }
#endif

  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
#if 0
    if (P_SectorActive(ceiling_special, sec))
	continue;
#else
    if (sec->ceilingdata)
	continue;
#endif
    // new ceiling thinker
    rtn = 1;
    ceiling = P_NewCeilingAction (sec, type);

    switch(type)
    {
      case fastCrushAndRaise:
	ceiling->crush = true;
//	ceiling->topheight = sec->ceilingheight;
	ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);
	ceiling->direction = -1;
	ceiling->speed = CEILSPEED * 2;
	break;

      case silentCrushAndRaise:
      case crushAndRaise:
	ceiling->crush = true;
//	ceiling->topheight = sec->ceilingheight;
      case lowerAndCrush:
	ceiling->bottomheight += 8*FRACUNIT;
      case lowerToFloor:
//	ceiling->bottomheight = sec->floorheight;
	ceiling->direction = -1;
//	ceiling->speed = CEILSPEED;
	break;

      case raiseToHighest:
	ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
//	ceiling->direction = 1;
//	ceiling->speed = CEILSPEED;
	break;

      default:			// Assume Boom line
	itype = (int) type;
#if 0
	printf ("Boom %X (%u %X %X %d)\n", itype,
			      (itype >> CeilingTargetShift) & 7,
			      (itype & CeilingDirection),
			      (itype & CeilingCrush),
			      (itype >> CeilingChangeShift) & 3);
#endif
	offset = 0;
	switch ((itype >> CeilingTargetShift) & 7)
	{
	  case CtoHnC:						// HNC
	    newheight = P_FindHighestCeilingSurrounding(sec);
	    break;
	  case CtoLnC:						// LNC
	    newheight = P_FindLowestCeilingSurrounding(sec);
	    break;
	  case CtoNnC:						// NNC
	    if (itype & CeilingDirection)
	      newheight = P_FindNextHighestCeiling (sec,sec->ceilingheight);
	    else
	      newheight = P_FindNextLowestCeiling (sec,sec->ceilingheight);
	    break;
	  case CtoHnF:						// HNF
	    newheight = P_FindHighestFloorSurrounding(sec);
	    break;
	  case CtoF:						// FLR
	    newheight = sec->floorheight;
	    break;
	  case CbyST:						// SUT (Shortest upper texture)
	    offset = ~0;
	    for (i = 0; i < sec->linecount; i++)
	    {
	      if (twoSided (secnum, i) )
	      {
		side = getSide(secnum,i,0);
		if (side->toptexture >= 0)
		  if (textures[side->toptexture].height < offset)
		    offset = textures[side->toptexture].height;
		side = getSide(secnum,i,1);
		if (side->toptexture >= 0)
		  if (textures[side->toptexture].height < offset)
		    offset = textures[side->toptexture].height;
	      }
	    }
	    if (offset == ~0)
	      offset = 0;
	    newheight = sec->ceilingheight;
	    break;
	  case Cby24:						// 24
	    offset = (24*FRACUNIT);
	    newheight = sec->ceilingheight;
	    break;
	  default: /* Cby32: */					// 32
	    offset = (32*FRACUNIT);
	    newheight = sec->ceilingheight;
	}

	ceiling->speed = (CEILSPEED) << ((itype >> CeilingSpeedShift) & 3);

	if (itype & CeilingDirection)
	{
	  ceiling->topheight =
	  ceiling->bottomheight = (newheight + offset);
//	  ceiling->direction = 1;
	}
	else
	{
	  ceiling->topheight =
	  ceiling->bottomheight = (newheight - offset);
	  ceiling->direction = -1;
	}

	if (itype & CeilingCrush)
	  ceiling->crush = true;

//	printf ("direction = %d, current = %d, dest = %d, speed = %d\n",ceiling->direction, sec->ceilingheight>>FRACBITS,ceiling->topheight>>FRACBITS, ceiling->speed >> FRACBITS);

	mode = ((int)ceiling->type >> CeilingChangeShift) & 3;
	if (mode)
	{
	  sector_t * msec;
	  if (((int)ceiling->type & CeilingModel) == 0)
	  {
	    ceiling -> secspecial = line -> frontsector;
	  }
	  else
	  {
	    switch (((int)ceiling->type >> CeilingTargetShift) & 7)
	    {
	      case CtoHnF:
	      case CtoF:
		msec = P_FindModelFloorSector (sec->ceilingheight,secnum);
		break;
	      default:
		msec = P_FindModelCeilingSector (sec->ceilingheight,secnum);
	    }
	    if (msec)
	      ceiling -> secspecial = msec;
	  }
	}
    }
  }

  return rtn;
}

//-----------------------------------------------------------------------------
//
// Add an active ceiling
//
void P_AddActiveCeiling (ceiling_t* c)
{
  ceiling_t *next;

  next = activeceilingshead;
  if (next)
    next -> prev = c;

  activeceilingshead = c;
  c -> next = next;
  c -> prev = NULL;
}

//-----------------------------------------------------------------------------
//
// Remove a ceiling's thinker
//
void P_RemoveActiveCeiling (ceiling_t* c)
{
  ceiling_t *next;
  ceiling_t *prev;

  next = c -> next;
  prev = c -> prev;

  if (next)
    next -> prev = prev;

  if (prev == NULL)
    activeceilingshead = next;
  else
    prev -> next = next;

  c->sector->ceilingdata = NULL;
  P_RemoveThinker (&c->thinker);
}

//-----------------------------------------------------------------------------
//
// Restart a ceiling that's in-stasis
//
static int P_ActivateInStasisCeiling (line_t* line)
{
  int rtn;
  ceiling_t * ceiling;

  rtn = 0;
  for (ceiling = activeceilingshead ; ceiling != NULL ; ceiling=ceiling->next)
  {
    if ((ceiling -> tag == line->tag)
     && (ceiling -> thinker.function.acp1 == (actionf_p1)NULL))
    {
      // ceiling -> direction = ptr_2 -> olddirection;
      ceiling -> thinker.function.acp1 = (actionf_p1)T_MoveCeiling;
      rtn = 1;
    }
  }

  return (rtn);
}

//-----------------------------------------------------------------------------
//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
int EV_CeilingCrushStop (line_t *line)
{
  int rtn;
  ceiling_t * ceiling;

  rtn = 0;
  for (ceiling = activeceilingshead ; ceiling != NULL ; ceiling=ceiling->next)
  {
    if ((ceiling -> tag == line->tag)
     && (ceiling -> thinker.function.acp1 != (actionf_p1)NULL))
    {
      // ceiling -> olddirection = ptr_2 -> direction;
      ceiling -> thinker.function.acp1 = (actionf_p1)NULL;
      // ceiling -> direction = 0;		// in-stasis
      rtn = 1;
    }
  }

  return (rtn);
}

//-----------------------------------------------------------------------------
