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
//	Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_plats.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"

plat_t*		activeplatshead;
extern boolean	gamekeydown[NUMKEYS];


static int P_ActivateInStasis (int tag);

//-----------------------------------------------------------------------------
//
// Move a plat up and down
//
void T_PlatRaise(plat_t* plat)
{
  result_e	res;

  switch(plat->status)
  {
    case up:
      res = T_MovePlane(plat->sector,
			plat->speed,
			plat->high,
			plat->crush,0,1);

      if (res == crushed && (!plat->crush))
      {
	  plat->count = plat->wait;
	  plat->status = down;
	  S_StartSound((mobj_t *)&plat->sector->soundorg,
		       sfx_pstart);
      }
      else if (res != pastdest)
      {
	if (plat->type == raiseAndChange
	    || plat->type == raiseToNearestAndChange)
	{
	  if (!(leveltime&7))
	    S_StartSound((mobj_t *)&plat->sector->soundorg,sfx_stnmov);
	}
      }
      else
      {
	plat->count = plat->wait;
	plat->status = waiting;
	S_StartSound((mobj_t *)&plat->sector->soundorg,sfx_pstop);

	switch (plat->type)
	{
	  case blazeDWUS:
	  case downWaitUpStay:
	  case raiseAndChange:
	  case raiseToNearestAndChange:
	    P_RemoveActivePlat(plat);
	    break;

	  default:
	    break;
	}
      }
      break;

    case down:
      res = T_MovePlane(plat->sector,plat->speed,plat->low,false,0,-1);

      if (res == pastdest)
      {
	  plat->count = plat->wait;
	  plat->status = waiting;
	  S_StartSound((mobj_t *)&plat->sector->soundorg,sfx_pstop);
	  switch (plat->type)
	  {
	    case downToLowestCeiling:
	      P_RemoveActivePlat(plat);
	      break;

	    default:
	      break;
	  }
      }
      break;

    case waiting:
      if ((netgame || !gamekeydown['`'])	// Stay open cheat
       && (!--plat->count))
      {
	  if (plat->sector->floorheight == plat->low)
	      plat->status = up;
	  else
	      plat->status = down;
	  S_StartSound((mobj_t *)&plat->sector->soundorg,sfx_pstart);
      }
    case in_stasis:
      break;
  }
}

//-----------------------------------------------------------------------------
//
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int
EV_DoPlat
( line_t*	line,
  plattype_e	type)
{
  plat_t*	plat;
  int		secnum;
  int		rtn;
  sector_t*	sec;


  //	Activate all <type> plats that are in_stasis
#if 1
  rtn = P_ActivateInStasis (line->tag);
#else
  rtn = 0;
  switch(type)
  {
    case perpetualRaise:
      rtn = P_ActivateInStasis (line->tag);
      break;

    default:
      if ((int) type >= GenLiftBase)
      {
	switch ((((int) type) >> LiftTargetShift) & 3)
	{
	  case 3:					// PERP
	    rtn = P_ActivateInStasis (line->tag);
	    break;
	}
      }
      break;
  }
#endif

  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    sec = &sectors[secnum];

#if 0
    if (P_SectorActive(floor_special, sec))
        continue;
#else
    if (sec->floordata)
	continue;
#endif
    // Find lowest & highest floors around sector
    rtn = 1;
    plat = Z_Malloc( sizeof(*plat), PU_LEVSPEC, 0);
    P_AddThinker(&plat->thinker, (actionf_p1) T_PlatRaise);

    plat->type = type;
    plat->sector = sec;
    plat->sector->floordata = plat;
    plat->crush = false;
    plat->tag = line->tag;
    plat->high = sec->floorheight;

    switch(type)
    {
      case raiseToNearestAndChange:
	plat->speed = PLATSPEED/2;
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	plat->high = P_FindNextHighestFloor(sec,sec->floorheight);
	plat->wait = 0;
	plat->status = up;
	// NO MORE DAMAGE, IF APPLICABLE
	P_NoMoreSectorDamage (sec);
	S_StartSound((mobj_t *)&sec->soundorg,sfx_stnmov);
	break;

      case raiseAndChange32:
        plat->high += (8*FRACUNIT);
      case raiseAndChange24:
      case raiseAndChange:
	plat->high += (24*FRACUNIT);
	plat->type = raiseAndChange;
	plat->speed = PLATSPEED/2;
	sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
	plat->wait = 0;
	plat->status = up;
	S_StartSound((mobj_t *)&sec->soundorg,sfx_stnmov);
	break;

      case downWaitUpStay:
	plat->speed = PLATSPEED * 4;
	plat->low = P_FindLowestFloorSurrounding(sec);

	if (plat->low > sec->floorheight)
	    plat->low = sec->floorheight;

//	plat->high = sec->floorheight;
	plat->wait = 35*PLATWAIT;
	plat->status = down;
	S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	break;

      case blazeDWUS:
	plat->speed = PLATSPEED * 8;
	plat->low = P_FindLowestFloorSurrounding(sec);

	if (plat->low > sec->floorheight)
	    plat->low = sec->floorheight;

//	plat->high = sec->floorheight;
	plat->wait = 35*PLATWAIT;
	plat->status = down;
	S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	break;

      case perpetualRaise:
	plat->speed = PLATSPEED;
	plat->low = P_FindLowestFloorSurrounding(sec);

	if (plat->low > sec->floorheight)
	    plat->low = sec->floorheight;

	plat->high = P_FindHighestFloorSurrounding(sec);

	if (plat->high < sec->floorheight)
	    plat->high = sec->floorheight;

	plat->wait = 35*PLATWAIT;
	plat->status = (plat_e) (P_Random()&1);

	S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	break;

      case downToLowestCeiling:
	plat->speed = PLATSPEED;
	plat->low = P_FindLowestCeilingSurrounding(sec);

	if (plat->low > sec->floorheight)
	    plat->low = sec->floorheight;

//	plat->high = sec->floorheight;
	plat->wait = 0;
	plat->status = down;
	S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	break;

      case nextLowestNeighbourFloor:
	plat->speed = PLATSPEED;
	plat->low = P_FindNextLowestFloor(sec, sec->floorheight);

	if (plat->low > sec->floorheight)
	    plat->low = sec->floorheight;

//	plat->high = sec->floorheight;
	plat->wait = 0;
	plat->status = down;
	S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	break;

      default:
	if ((int) type >= GenLiftBase)
	{
	  switch ((((int) type) >> LiftTargetShift) & 3)
	  {
	    case F2LnF:					// LNF
	      plat->low = P_FindLowestFloorSurrounding(sec);

	      if (plat->low > sec->floorheight)
		plat->low = sec->floorheight;

//	      plat->high = sec->floorheight;
	      plat->status = down;
	      plat->type = downWaitUpStay;
	      break;

	    case F2NnF:					// NNF
	      plat->low = P_FindNextLowestFloor(sec, sec->floorheight);

	      if (plat->low > sec->floorheight)
		plat->low = sec->floorheight;

//	      plat->high = sec->floorheight;
	      plat->status = down;
	      plat->type = nextLowestNeighbourFloor;
	      break;

	    case F2LnC:					// LNC
	      plat->low = P_FindLowestCeilingSurrounding(sec);

	      if (plat->low > sec->floorheight)
		plat->low = sec->floorheight;

//	      plat->high = sec->floorheight;
	      plat->status = down;
	      plat->type = downToLowestCeiling;
	      break;

	    default:	/* LnF2HnF */			// PERP
	      plat->low = P_FindLowestFloorSurrounding(sec);

	      if (plat->low > sec->floorheight)
		plat->low = sec->floorheight;

	      plat->high = P_FindHighestFloorSurrounding(sec);

	      if (plat->high < sec->floorheight)
		plat->high = sec->floorheight;

	      plat->status = (plat_e) (P_Random()&1);
	      plat->type = perpetualRaise;
          }

	  switch ((((int) type) >> LiftSpeedShift) & 3)
	  {
	    case 0: plat->speed = PLATSPEED/2; break;
	    case 1: plat->speed = PLATSPEED; break;
	    case 2: plat->speed = PLATSPEED*4; break;
	    default: plat->speed = PLATSPEED*8; break;
	  }

	  switch ((((int) type) >> LiftDelayShift) & 3)
	  {
	    case 0: plat->wait = (35*PLATWAIT)*1; break;
	    case 1: plat->wait = (35*PLATWAIT)*3; break;
	    case 2: plat->wait = (35*PLATWAIT)*5; break;
	    default: plat->wait = (35*PLATWAIT)*10; break;
	  }

	  S_StartSound((mobj_t *)&sec->soundorg,sfx_pstart);
	}
    }
    P_AddActivePlat(plat);
  }
  return rtn;
}

//-----------------------------------------------------------------------------

static int P_ActivateInStasis (int tag)
{
  int rc;
  plat_t * plat;

  rc = 0;
  for (plat = activeplatshead ; plat != NULL ; plat=plat->next)
  {
    if ((plat -> tag == tag)
     && (plat -> status == in_stasis))
    {
      plat -> status = plat -> oldstatus;
      plat -> thinker.function.acp1 = (actionf_p1) T_PlatRaise;
      rc = 1;
    }
  }

  return (rc);
}

//-----------------------------------------------------------------------------

void EV_StopPlat (line_t* line)
{
  plat_t * plat;

  for (plat = activeplatshead ; plat != NULL ; plat=plat->next)
  {
    if ((plat -> tag == line->tag)
     && (plat -> status != in_stasis))
    {
      plat -> oldstatus = plat -> status;
      plat -> status = in_stasis;
      plat -> thinker.function.acv = (actionf_v) NULL;
    }
  }
}

//-----------------------------------------------------------------------------

void P_AddActivePlat(plat_t* plat)
{
  plat_t *next;

  next = activeplatshead;
  if (next)
    next -> prev = plat;

  activeplatshead = plat;
  plat -> next = next;
  plat -> prev = NULL;
}

//-----------------------------------------------------------------------------

void P_RemoveActivePlat(plat_t* plat)
{
  plat_t *next;
  plat_t *prev;

  next = plat -> next;
  prev = plat -> prev;

  if (next)
    next -> prev = prev;

  if (prev == NULL)
    activeplatshead = next;
  else
    prev -> next = next;

  plat->sector->floordata = NULL;
  P_RemoveThinker (&plat->thinker);
}

//-----------------------------------------------------------------------------
