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
// DESCRIPTION: Door animation code (opening/closing)
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_doors.c,v 1.4 1997/02/03 16:47:53 b1 Exp $";
#endif


#include "includes.h"

extern boolean	gamekeydown[NUMKEYS];

/* ----------------------------------------------------------------------- */

char * door_messages_orig [] =
{
  PD_BLUEO,
  PD_REDO,
  PD_YELLOWO,
  PD_BLUEK,
  PD_REDK,
  PD_YELLOWK,
  PD_BLUEC,
  PD_REDC,
  PD_YELLOWC,
  PD_BLUES,
  PD_REDS,
  PD_YELLOWS,
  PD_BLUECO,
  PD_REDCO,
  PD_YELLOWCO,
  PD_BLUESO,
  PD_REDSO,
  PD_YELLOWSO,
  PD_ANY,
  PD_ANYOBJ,
  PD_ALL3,
  PD_ALL3O,
  PD_ALL6,
  PD_ALL6O,
  PD_ALLKEYS,
  NULL
};

char * door_messages [ARRAY_SIZE(door_messages_orig)];

/* ----------------------------------------------------------------------- */


#if 0
//
// Sliding door frame information
//
slidename_t	slideFrameNames[MAXSLIDEDOORS] =
{
    {"GDOORF1","GDOORF2","GDOORF3","GDOORF4",	// front
     "GDOORB1","GDOORB2","GDOORB3","GDOORB4"},	// back

    {"\0","\0","\0","\0"}
};
#endif

/* ---------------------------------------------------------------------------- */

static void T_Makedoorsound (vldoor_t* door)
{
  int sound;

  if (door->direction == 1)		// Open
  {
    sound = sfx_doropn;
    if (door->speed >= VDOORSPEED * 2)
      sound = sfx_bdopn;
  }
  else
  {
    sound = sfx_dorcls;
    if (door->speed >= VDOORSPEED * 2)
      sound = sfx_bdcls;
  }

  // printf ("T_Makedoorsound %u\n", sound);
  S_StartSound ((mobj_t *)&door->sector->soundorg, sound);
}

/* ---------------------------------------------------------------------------- */
//
// VERTICAL DOORS
//

//
// T_VerticalDoor
//
void T_VerticalDoor (vldoor_t* door)
{
    result_e	res;
    line_t* 	line;

    switch(door->direction)
    {
      case 0:
	// WAITING
	if (!--door->topcountdown)
	{
	    switch(door->type)
	    {
	      case normal:
	      case blazeRaise:
		door->direction = -1; // time to go back down
		T_Makedoorsound (door);
		break;

	      case closeThenOpen:
	      case close30ThenOpen:
		door->direction = 1;
		T_Makedoorsound (door);
		break;

	      default:
		break;
	    }
	}
	break;

      case 2:
	//  INITIAL WAIT
	if (!--door->topcountdown)
	{
	    switch(door->type)
	    {
	      case raiseIn5Mins:
		door->direction = 1;
		door->type = normal;
		T_Makedoorsound (door);
		break;

	      default:
		break;
	    }
	}
	break;

      case -1:
	// DOWN
	res = T_MovePlane(door->sector,
			  door->speed,
			  door->sector->floorheight,
			  false,1,door->direction);
	if (res == pastdest)
	{
	    if ((line = door->line) != NULL)
	      EV_TurnTagLightsOff (line);

	    switch(door->type)
	    {
	      case blazeRaise:
	      case blazeClose:
		door->sector->ceilingdata = NULL;
		P_RemoveThinker (&door->thinker);  // unlink and free
		//S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdcls);
		break;

	      case normal:
	      case normalClose:
		door->sector->ceilingdata = NULL;
		P_RemoveThinker (&door->thinker);  // unlink and free
		break;

	      case close30ThenOpen:
		door->direction = 0;
		door->topcountdown = 35*30;
		break;

	      case closeThenOpen:
		door->direction = 0;
		door->topcountdown = door->topwait;
		break;

	      default:
		break;
	    }
	}
	else if (res == crushed)
	{
	    switch(door->type)
	    {
	      case blazeClose:
	      case normalClose:		// DO NOT GO BACK UP!
		break;

	      default:
		door->direction = 1;
		T_Makedoorsound (door);
		break;
	    }
	}
	break;

      case 1:
	// UP
	res = T_MovePlane(door->sector,
			  door->speed,
			  door->topheight,
			  false,1,door->direction);

	if (res == pastdest)
	{
	    if ((line = door->line) != NULL)
	    {
	      EV_LightTurnOn (line, 0);
	    }

	    switch(door->type)
	    {
	      case blazeRaise:
	      case normal:
		if (netgame || (!gamekeydown['`'])) 	// Stay open cheat
		{
		  door->direction = 0;			// wait at top
		  door->topcountdown = door->topwait;
		  break;
		}

	      case closeThenOpen:
	      case close30ThenOpen:
	      case blazeOpen:
	      case normalOpen:
		door->sector->ceilingdata = NULL;
		P_RemoveThinker (&door->thinker);  // unlink and free
		break;

	      default:
		break;
	    }
	}
	break;
    }
}

/* ---------------------------------------------------------------------------- */

int EV_Check_Lock (player_t * p, int keynum)
{
  if (p)
  {
    switch (keynum)
    {
      case P_PD_BLUEO:
      case P_PD_BLUEK:
	if (p->cards[it_bluecard] || p->cards[it_blueskull])
	  return (1);
	break;

      case P_PD_REDO:
      case P_PD_REDK:
	if (p->cards[it_redcard] || p->cards[it_redskull])
	  return (1);
	break;

      case P_PD_YELLOWO:
      case P_PD_YELLOWK:
	if (p->cards[it_yellowcard] || p->cards[it_yellowskull])
	  return (1);
	break;

      case P_PD_REDC:
      case P_PD_REDCO:
	if (p->cards[it_redcard])
	  return (1);
	break;

      case P_PD_BLUEC:
      case P_PD_BLUECO:
	if (p->cards[it_bluecard])
	  return (1);
	break;

      case P_PD_YELLOWC:
      case P_PD_YELLOWCO:
	if (p->cards[it_yellowcard])
	  return (1);
	break;

      case P_PD_REDS:
      case P_PD_REDSO:
	if (p->cards[it_redskull])
	  return (1);
	break;

      case P_PD_BLUES:
      case P_PD_BLUESO:
	if (p->cards[it_blueskull])
	  return (1);
	break;

      case P_PD_YELLOWS:
      case P_PD_YELLOWSO:
	if (p->cards[it_yellowskull])
	  return (1);
	break;

      case P_PD_ANY:
      case P_PD_ANYOBJ:
	if (p->cards[it_bluecard] || p->cards[it_blueskull]
	 || p->cards[it_redcard] || p->cards[it_redskull]
	 || p->cards[it_yellowcard] || p->cards[it_yellowskull])
	  return (1);
	break;

      case P_PD_ALL3:
      case P_PD_ALL3O:
	if ((p->cards[it_bluecard] || p->cards[it_blueskull])
	 && (p->cards[it_redcard] || p->cards[it_redskull])
	 && (p->cards[it_yellowcard] || p->cards[it_yellowskull]))
	  return (1);
        break;

      case P_PD_ALL6:
      case P_PD_ALL6O:
      case P_PD_ALLKEYS:
	if (p->cards[it_bluecard] && p->cards[it_blueskull]
	 && p->cards[it_redcard] && p->cards[it_redskull]
	 && p->cards[it_yellowcard] && p->cards[it_yellowskull])
	  return (1);
        break;

      default:
        return (0);
    }

    p->message = door_messages [keynum];
    S_StartSound(NULL,sfx_oof);
  }
  return 0;
}

/* ---------------------------------------------------------------------------- */
//
// EV_DoLockedDoor
// Move a locked door up/down
//

int
EV_DoLockedDoor
( line_t*	line,
  int		keynum,
  vldoor_e	type,
  mobj_t*	thing )
{
  int rc;

  rc = EV_Check_Lock (thing->player, keynum);
  if (rc)
    rc = EV_DoDoorR (line,type,thing);

  return (rc);
}

/* ---------------------------------------------------------------------------- */

int
EV_DoDoor
( line_t*	line,
  vldoor_e	type )
{
  return (EV_DoDoorR (line, type, NULL));
}

/* ---------------------------------------------------------------------------- */
/*
  Init a floor movement structure to some safe defaults.
*/

static vldoor_t* get_door_block (sector_t* sec)
{
  vldoor_t*	door;

  door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
  P_AddThinker (&door->thinker, (actionf_p1) T_VerticalDoor);
  sec->ceilingdata = door;

  door->sector = sec;
  door->topwait = VDOORWAIT;
  door->speed = VDOORSPEED;
  door->direction = 1;
  door->line = NULL;
  return (door);
}

//-----------------------------------------------------------------------------
/* Returns:
	0 = No doors opened
	1 = Opened at least 1 door
*/

int
EV_DoDoorR
( line_t*	line,
  vldoor_e	type,
  mobj_t*	thing )
{
  int		secnum,rtn;
  int		newheight;
  sector_t*	sec;
  vldoor_t*	door;

  rtn = 0;
  secnum = -1;
  while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
  {
    sec = &sectors[secnum];
#if 0
    if (P_SectorActive(ceiling_special, sec))
        continue;
#else
    door = sec->ceilingdata;
    if (door)
    {
      if ((line -> tag == 0)
       && (door->thinker.function.acp1 == (actionf_p1) T_VerticalDoor))
      {
	switch (door->type)
	{
	  case normal:			// ONLY FOR "RAISE" DOORS, NOT "OPEN"s
	  case blazeRaise:
	    if (door->direction == -1)
	    {
	      door->direction = 1;	// go back up
	    }
	    else if ((thing)&&(thing->player))// JDC: bad guys never close doors
	    {
	      door->direction = -1;	// start going down immediately
	    }
	}
      }
      continue;
    }
#endif

    // new door thinker
    rtn = 1;
    door = get_door_block (sec);
    door->type = type;

    switch(type)
    {
      case blazeClose:
	door->speed = VDOORSPEED * 4;

      case normalClose:
	newheight = P_FindLowestCeilingSurrounding(sec);
	door->topheight = newheight - 4*FRACUNIT;
	door->direction = -1;
	T_Makedoorsound (door);
	break;

      case close30ThenOpen:
	door->topheight = sec->ceilingheight;
	door->direction = -1;
	T_Makedoorsound (door);
	break;

      case blazeRaise:
      case blazeOpen:
	door->speed = VDOORSPEED * 4;

      case normal:
      case normalOpen:
	// door->direction = 1;
	newheight = P_FindNextHighestCeiling (sec, sec->ceilingheight);
	if (newheight > sec->ceilingheight)		// Returns current height if none found...
	{
	  newheight -= 4*FRACUNIT;
	  if (newheight > sec->ceilingheight)		// Is door going to move?
	    T_Makedoorsound (door);
	}
	door->topheight = newheight;
	break;
    }
  }
  return rtn;
}

//-----------------------------------------------------------------------------
//
// EV_DoLockedDoor
// Move a locked door up/down
//

int
EV_DoGenLockedDoor
( line_t*	line,
  int		keynum,
  vldoor_e	type,
  mobj_t*	thing )
{
  int rc;

  rc = EV_Check_Lock (thing->player, keynum);
  if (rc)
    rc = EV_DoGenDoor (line,type,thing);

  return (rc);
}

/* ---------------------------------------------------------------------------- */
/* Returns:
	0 = No doors opened
	1 = Opened at least 1 door
*/

int
EV_DoGenDoor
( line_t*	line,
  vldoor_e	type,
  mobj_t*	thing )
{
  int		rtn;
  int		door_type;
  int		newheight;
  sector_t*	sec;
  vldoor_t*	door;

  rtn = 0;

  if (((line ->flags & ML_TWOSIDED) == 0)
   || ((sec = sides[ line->sidenum[0^1]] .sector) == NULL))
  {
    line->special = 0;			// Yet another badly built wad!!
    return (rtn);
  }

  // new door thinker
  rtn = 1;
  door = get_door_block (sec);
  door->type = type;

  if (line -> tag)
    door->line = line;

  switch (genshift(type,DoorSpeed,DoorSpeedShift))
  {
    case 0: door->speed = VDOORSPEED/2; break;
    case 1: door->speed = VDOORSPEED; break;
    case 2: door->speed = VDOORSPEED * 2; break;
    default:door->speed = VDOORSPEED * 4;
  }

  if (type >= GenDoorBase)
  {
    switch ((type >> DoorDelayShift) & 3)
    {
      case 0: door->topwait = 35 * 1; break;	// 1 sec
      case 1: door->topwait = 35 * 4; break;	// 4 sec
      case 2: door->topwait = 35 * 9; break;	// 9 sec
      default:door->topwait = 35 * 30; break;	// 30 sec
    }
    door_type = genshift(type,DoorKind,DoorKindShift);
  }
  else
  {
    door_type = genshift(type,LockedKind,LockedKindShift);// Locked doors only do OWC & OSO
  }

  switch (door_type)
  {
    case OdCDoor:				// OWC
      door->type = normal;
      // door->direction = 1;
      newheight = P_FindNextHighestCeiling (sec, sec->ceilingheight);
      if (newheight > sec->ceilingheight)	// Returns current height if none found...
      {
	newheight -= 4*FRACUNIT;
	if (newheight > sec->ceilingheight)	// Is door going to move?
	  T_Makedoorsound (door);
      }
      door->topheight = newheight;
      break;

    case ODoor:					// OSO
      door->type = normalOpen;
      // door->direction = 1;
      newheight = P_FindNextHighestCeiling (sec, sec->ceilingheight);
      if (newheight > sec->ceilingheight)	// Returns current height if none found...
      {
	newheight -= 4*FRACUNIT;
	if (newheight > sec->ceilingheight)	// Is door going to move?
	  T_Makedoorsound (door);
      }
      door->topheight = newheight;
      break;

    case CdODoor:				// CWO
      door->type = closeThenOpen;
      door->topheight = sec->ceilingheight;
      door->direction = -1;
      T_Makedoorsound (door);
      break;

    default: /* CDoor */			// CSC
      door->type = normalClose;
      newheight = P_FindLowestCeilingSurrounding(sec);
      door->topheight = newheight - 4*FRACUNIT;
      door->direction = -1;
      T_Makedoorsound (door);
      break;
  }
  return (rtn);
}

/* ---------------------------------------------------------------------------- */
//
// EV_VerticalDoor : open a door manually, no tag value
//
void
EV_VerticalDoor
( line_t*	line,
  mobj_t*	thing )
{
  player_t*	player;
  sector_t*	sec;
  vldoor_t*	door;

  //	Check for locks
  player = thing->player;

  switch(line->special)
  {
    case 26: // Blue Lock
    case 32:
      if (EV_Check_Lock (player, P_PD_BLUEK) == 0)
	return;
      break;

    case 27: // Yellow Lock
    case 34:
      if (EV_Check_Lock (player, P_PD_YELLOWK) == 0)
	return;
      break;

    case 28: // Red Lock
    case 33:
      if (EV_Check_Lock (player, P_PD_REDK) == 0)
	return;
      break;
  }

  if (((line ->flags & ML_TWOSIDED) == 0)
   || ((sec = sides[ line->sidenum[0^1]] .sector) == NULL))
  {
    line->special = 0;			// Yet another badly built wad!!
    return;
  }

  // if the sector has an active thinker, use it
  door = sec->ceilingdata;
  if (door)
  {
    if (door->thinker.function.acp1 == (actionf_p1) T_VerticalDoor)
    {
      switch(line->special)
      {
	case	1:			// ONLY FOR "RAISE" DOORS, NOT "OPEN"s
	case	26:
	case	27:
	case	28:
	case	117:
	  if (door->direction == -1)
	  {
	    door->direction = 1;	// go back up
	  }
	  else if (player)		// JDC: bad guys never close doors
	  {
	    door->direction = -1;	// start going down immediately
	  }
      }
    }
    return;
  }

  // new door thinker
  door = get_door_block (sec);

  switch(line->special)
  {
    case 1:
    case 26:
    case 27:
    case 28:
      door->type = normal;
      break;

    case 31:
    case 32:
    case 33:
    case 34:
      door->type = normalOpen;
      line->special = 0;
      break;

    case 117:	// blazing door raise
      door->type = blazeRaise;
      door->speed = VDOORSPEED*4;
      break;

    case 118:	// blazing door open
      door->type = blazeOpen;
      line->special = 0;
      door->speed = VDOORSPEED*4;
      break;
  }

  // find the top and bottom of the movement range
  door->topheight = P_FindNextHighestCeiling (sec, sec->ceilingheight) - (4*FRACUNIT);
  T_Makedoorsound (door);
}

/* ---------------------------------------------------------------------------- */

//
// Spawn a door that closes after 30 seconds
//
void P_SpawnDoorCloseIn30 (sector_t* sec)
{
    vldoor_t*	door;

    sec->special &= ~31;
    door = get_door_block (sec);
    door->direction = 0;
    door->type = normal;
    door->topcountdown = 30 * 35;
    door->line = NULL;
}

/* ---------------------------------------------------------------------------- */
//
// Spawn a door that opens after 5 minutes
//
void
P_SpawnDoorRaiseIn5Mins
( sector_t*	sec,
  int		secnum )
{
    vldoor_t*	door;

    sec->special &= ~31;
    door = get_door_block (sec);
    door->direction = 2;
    door->type = raiseIn5Mins;
    door->topheight = P_FindNextHighestCeiling (sec, sec->ceilingheight) - (4*FRACUNIT);
    door->topcountdown = 5 * 60 * 35;
}

/* ---------------------------------------------------------------------------- */
// UNUSED
// Separate into p_slidoor.c?

#if 0		// ABANDONED TO THE MISTS OF TIME!!!
//
// EV_SlidingDoor : slide a door horizontally
// (animate midtexture, then set noblocking line)
//


slideframe_t slideFrames[MAXSLIDEDOORS];

void P_InitSlidingDoorFrames(void)
{
    int		i;
    int		f1;
    int		f2;
    int		f3;
    int		f4;

    // DOOM II ONLY...
    if ( gamemode != commercial)
	return;

    for (i = 0;i < MAXSLIDEDOORS; i++)
    {
	if (!slideFrameNames[i].frontFrame1[0])
	    break;

	f1 = R_TextureNumForName(slideFrameNames[i].frontFrame1);
	f2 = R_TextureNumForName(slideFrameNames[i].frontFrame2);
	f3 = R_TextureNumForName(slideFrameNames[i].frontFrame3);
	f4 = R_TextureNumForName(slideFrameNames[i].frontFrame4);

	slideFrames[i].frontFrames[0] = f1;
	slideFrames[i].frontFrames[1] = f2;
	slideFrames[i].frontFrames[2] = f3;
	slideFrames[i].frontFrames[3] = f4;

	f1 = R_TextureNumForName(slideFrameNames[i].backFrame1);
	f2 = R_TextureNumForName(slideFrameNames[i].backFrame2);
	f3 = R_TextureNumForName(slideFrameNames[i].backFrame3);
	f4 = R_TextureNumForName(slideFrameNames[i].backFrame4);

	slideFrames[i].backFrames[0] = f1;
	slideFrames[i].backFrames[1] = f2;
	slideFrames[i].backFrames[2] = f3;
	slideFrames[i].backFrames[3] = f4;
    }
}


/* ---------------------------------------------------------------------------- */
//
// Return index into "slideFrames" array
// for which door type to use
//
int P_FindSlidingDoorType(line_t*	line)
{
    int		i;
    int		val;

    for (i = 0;i < MAXSLIDEDOORS;i++)
    {
	val = sides[line->sidenum[0]].midtexture;
	if (val == slideFrames[i].frontFrames[0])
	    return i;
    }

    return -1;
}

/* ---------------------------------------------------------------------------- */

void T_SlidingDoor (slidedoor_t*	door)
{
    switch(door->status)
    {
      case sd_opening:
	if (!door->timer--)
	{
	    if (++door->frame == SNUMFRAMES)
	    {
		// IF DOOR IS DONE OPENING...
		sides[door->line->sidenum[0]].midtexture = 0;
		sides[door->line->sidenum[1]].midtexture = 0;
		door->line->flags &= ML_BLOCKING^0xff;

		if (door->type == sdt_openOnly)
		{
		    door->frontsector->ceilingdata = NULL;
		    P_RemoveThinker (&door->thinker);
		    break;
		}

		door->timer = SDOORWAIT;
		door->status = sd_waiting;
	    }
	    else
	    {
		// IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
		door->timer = SWAITTICS;

		sides[door->line->sidenum[0]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    frontFrames[door->frame];
		sides[door->line->sidenum[1]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    backFrames[door->frame];
	    }
	}
	break;

      case sd_waiting:
	// IF DOOR IS DONE WAITING...
	if (!door->timer--)
	{
	    // CAN DOOR CLOSE?
	    if (door->frontsector->thinglist != NULL ||
		door->backsector->thinglist != NULL)
	    {
		door->timer = SDOORWAIT;
		break;
	    }

	    //door->frame = SNUMFRAMES-1;
	    door->status = sd_closing;
	    door->timer = SWAITTICS;
	}
	break;

      case sd_closing:
	if (!door->timer--)
	{
	    if (--door->frame < 0)
	    {
		// IF DOOR IS DONE CLOSING...
		door->line->flags |= ML_BLOCKING;
		door->frontsector->ceilingdata = NULL;
		P_RemoveThinker (&door->thinker);
		break;
	    }
	    else
	    {
		// IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
		door->timer = SWAITTICS;

		sides[door->line->sidenum[0]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    frontFrames[door->frame];
		sides[door->line->sidenum[1]].midtexture =
		    slideFrames[door->whichDoorIndex].
		    backFrames[door->frame];
	    }
	}
	break;
    }
}

/* ---------------------------------------------------------------------------- */

void
EV_SlidingDoor
( line_t*	line,
  mobj_t*	thing )
{
    sector_t*		sec;
    slidedoor_t*	door;

    // DOOM II ONLY...
    if (gamemode != commercial)
	return;

    // Make sure door isn't already being animated
    sec = line->frontsector;
    door = NULL;
    if (sec->ceilingdata)
    {
	if (!thing->player)
	    return;

	door = sec->ceilingdata;
	if (door->type == sdt_openAndClose)
	{
	    if (door->status == sd_waiting)
		door->status = sd_closing;
	}
	else
	    return;
    }

    // Init sliding door vars
    if (!door)
    {
	door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
	P_AddThinker (&door->thinker, (actionf_p1)T_SlidingDoor);
	sec->ceilingdata = door;

	door->type = sdt_openAndClose;
	door->status = sd_opening;
	door->whichDoorIndex = P_FindSlidingDoorType(line);

	if (door->whichDoorIndex < 0)
	    I_Error("EV_SlidingDoor: Can't use texture for sliding door!");

	door->frontsector = sec;
	door->backsector = line->backsector;
	door->timer = SWAITTICS;
	door->frame = 0;
	door->line = line;
    }
}
#endif
/* ---------------------------------------------------------------------------- */
