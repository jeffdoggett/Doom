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
//
// $Log:$
//
// DESCRIPTION:
//	Switches, buttons. Two-state animation. Exits.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_switch.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";
#endif


#include "includes.h"

static unsigned int Read_SWITCHES_Lump (unsigned int count);

/* ----------------------------------------------------------------------- */
#if 0
//
// CHANGE THE TEXTURE OF A WALL SWITCH TO ITS OPPOSITE
//
switchlist_t alphSwitchList[] =
{
    // Doom shareware episode 1 switches
    {"SW1BRCOM",	"SW2BRCOM",	1},
    {"SW1BRN1",	"SW2BRN1",	1},
    {"SW1BRN2",	"SW2BRN2",	1},
    {"SW1BRNGN",	"SW2BRNGN",	1},
    {"SW1BROWN",	"SW2BROWN",	1},
    {"SW1COMM",	"SW2COMM",	1},
    {"SW1COMP",	"SW2COMP",	1},
    {"SW1DIRT",	"SW2DIRT",	1},
    {"SW1EXIT",	"SW2EXIT",	1},
    {"SW1GRAY",	"SW2GRAY",	1},
    {"SW1GRAY1",	"SW2GRAY1",	1},
    {"SW1METAL",	"SW2METAL",	1},
    {"SW1PIPE",	"SW2PIPE",	1},
    {"SW1SLAD",	"SW2SLAD",	1},
    {"SW1STARG",	"SW2STARG",	1},
    {"SW1STON1",	"SW2STON1",	1},
    {"SW1STON2",	"SW2STON2",	1},
    {"SW1STONE",	"SW2STONE",	1},
    {"SW1STRTN",	"SW2STRTN",	1},

    // Doom registered episodes 2&3 switches
    {"SW1BLUE",	"SW2BLUE",	2},
    {"SW1CMT",		"SW2CMT",	2},
    {"SW1GARG",	"SW2GARG",	2},
    {"SW1GSTON",	"SW2GSTON",	2},
    {"SW1HOT",		"SW2HOT",	2},
    {"SW1LION",	"SW2LION",	2},
    {"SW1SATYR",	"SW2SATYR",	2},
    {"SW1SKIN",	"SW2SKIN",	2},
    {"SW1VINE",	"SW2VINE",	2},
    {"SW1WOOD",	"SW2WOOD",	2},

    // Doom II switches
    {"SW1PANEL",	"SW2PANEL",	3},
    {"SW1ROCK",	"SW2ROCK",	3},
    {"SW1MET2",	"SW2MET2",	3},
    {"SW1WDMET",	"SW2WDMET",	3},
    {"SW1BRIK",	"SW2BRIK",	3},
    {"SW1MOD1",	"SW2MOD1",	3},
    {"SW1ZIM",		"SW2ZIM",	3},
    {"SW1STON6",	"SW2STON6",	3},
    {"SW1TEK",		"SW2TEK",	3},
    {"SW1MARB",	"SW2MARB",	3},
    {"SW1SKULL",	"SW2SKULL",	3},

    {"\0",		"\0",		0}
};
#endif
/* ----------------------------------------------------------------------- */

int		switchlist[(MAXSWITCHES*2) + 1];
extern texture_t** textures;
extern int	numtextures;

/* ----------------------------------------------------------------------- */
//
// P_InitSwitchList
// Only called at game initialization.
//
// For each SW1xxxxx texture we look for a corresponding SW2xxxxx texture. (JAD 27/09/11)

void P_InitSwitchList (void)
{
  int	 	i,j;
  unsigned int	count;
  int *		ptr_B;
  texture_t**	ptr_1;
  texture_t*	ptr_2;
  char sw2name [9];

  sw2name [8] = 0;

  i = 0;
  count = 0;
  // numswitches = 0;
  ptr_1 = textures;
  ptr_B = switchlist;

  do
  {
    ptr_2 = *ptr_1++;
    if (strncasecmp (ptr_2->name, "SW1", 3) == 0)
    {
      strncpy (sw2name, ptr_2->name, 8);
      sw2name [2] = '2';
      j = R_CheckTextureNumForName (sw2name);
      if (j != -1)
      {
	if (count < MAXSWITCHES)
	{
	  *ptr_B++ = i;
	  *ptr_B++ = j;
	  // numswitches++;
	}
	count++;
      }
    }
  } while (++i < numtextures);

  *ptr_B = -1;

  count = Read_SWITCHES_Lump (count);

  if ((M_CheckParm ("-showswitchcount"))
   || (count > MAXSWITCHES))
    printf ("Found %u switch pairs of %u\n", count, MAXSWITCHES);
}

/* ----------------------------------------------------------------------- */
/*
   Read the switches lump.
   Each Entry is 20 bytes.

*/

static unsigned int Read_SWITCHES_Lump (unsigned int count)
{
  int i,j;
  int lump,slump1,slump2;
  int size;
  int pos;
  char * ptr;
  char name_1 [10];
  char name_2 [10];

  lump = -1;
  while ((lump = W_NextLumpNumForName ("SWITCHES", lump)) != -1)
  {
    ptr = W_CacheLumpNum (lump, PU_STATIC);
    size = W_LumpLength (lump);
    pos = 0;

    do
    {
      i = 0;
      do
      {
        name_1 [i++] = ptr [pos++];
      } while (i < 9);
      i = 0;
      do
      {
        name_2 [i++] = ptr [pos++];
      } while (i < 9);
      pos += 2;				// Not interested in the episode number

      if ((name_1 [0]) && (name_2 [0]))
      {
	if (((slump1 = R_CheckTextureNumForName (name_1)) != -1)
	 && ((slump2 = R_CheckTextureNumForName (name_2)) != -1))
	{
	  /* Have we already done this one? */
	  // printf ("Checking switches %s and %s\n", name_1, name_2);
	  i = 0;
	  do
	  {
	    j = switchlist[i];
	    if (j == -1)
	    {
	      if (count < MAXSWITCHES)
	      {
		switchlist [i++] = slump1;
		switchlist [i++] = slump2;
		switchlist [i++] = -1;
		// numswitches++;
	      }
	      count++;
	      break;
	    }
	    if ((slump1 == j)
	     || (slump2 == j))
	    {
	      switchlist [i] = slump1;
	      switchlist [i ^ 1] = slump2;
	      break;
	    }
	  } while (++i);
	}
      }
      size -= 20;
    } while (size >= 20);

    Z_Free (ptr);
  }

  return (count);
}

/* ----------------------------------------------------------------------- */

#if 0
void P_InitSwitchList(void)
{
    int		i;
    int		index;
    int		episode;

    episode = 1;

    if (gamemode == registered)
	episode = 2;
    else
	if ( gamemode == commercial )
	    episode = 3;

    for (index = 0,i = 0;i < MAXSWITCHES;i++)
    {
	if (!alphSwitchList[i].episode)
	{
	    numswitches = index/2;
	    switchlist[index] = -1;
	    break;
	}

	if (alphSwitchList[i].episode <= episode)
	{
#if 0	// UNUSED - debug?
	    int		value;

	    if (R_CheckTextureNumForName(alphSwitchList[i].name1) < 0)
	    {
		I_Error("Can't find switch texture '%s'!",
			alphSwitchList[i].name1);
		continue;
	    }

	    value = R_TextureNumForName(alphSwitchList[i].name1);
#endif
	    switchlist[index++] = R_TextureNumForName(alphSwitchList[i].name1);
	    switchlist[index++] = R_TextureNumForName(alphSwitchList[i].name2);
	}
    }
}
#endif

/* ----------------------------------------------------------------------- */

void T_Button (button_t* button)
{
  mobj_t *	soundorg;

  if ((button -> btimer == 0)
   || (--button -> btimer == 0))
  {
    switch (button -> where)
    {
      case top:
	sides[button -> line->sidenum[0]].toptexture = button -> btexture;
	break;

      case middle:
	sides[button -> line->sidenum[0]].midtexture = button -> btexture;
	break;

      case bottom:
	sides[button -> line->sidenum[0]].bottomtexture = button -> btexture;
	break;
    }
    soundorg = button -> soundorg;
    if (soundorg == NULL)			// Just in case, should never be zero!
      soundorg = (mobj_t *)&button->line->soundorg;
    S_StartSound(soundorg,sfx_swtchn);
    P_RemoveThinker(&button->thinker);
  }
}

/* ----------------------------------------------------------------------- */
//
// Start a button counting down till it turns off.
//
void
P_StartButton
( line_t*	line,
  bwhere_e	w,
  int		texture,
  int		time )
{
  button_t* button;

  button = Z_Malloc (sizeof(*button), PU_LEVSPEC, 0);
  P_AddThinker (&button->thinker, (actionf_p1) T_Button);
  button -> line = line;
  button -> where = w;
  button -> btexture = texture;
  button -> btimer = time;
  button -> soundorg = (mobj_t *)&line->soundorg;
}

/* ----------------------------------------------------------------------- */
//
// Function that changes wall texture.
// Tell it if switch is ok to use again (1=yes, it's a button).
//
void
P_ChangeSwitchTexture
( line_t*	line,
  int 		useAgain )
{
  int     texTop;
  int     texMid;
  int     texBot;
  int     i;
  int     sound;
  int	  swtex;
  side_t * side;
  mobj_t * soundorg;

  side = &sides[line->sidenum[0]];
  texTop = side -> toptexture;
  texMid = side -> midtexture;
  texBot = side -> bottomtexture;

  sound = sfx_swtchn;

  switch (line->special)
  {
    case 11:			// EXIT SWITCH?
    case 51:			// Secret exit
      sound = sfx_swtchx;
  }

  if (!useAgain)
    line->special = 0;

  i = 0;
  do
  {
    swtex = switchlist[i];
    if (swtex == texTop)
    {
      soundorg = (mobj_t *)&line->soundorg;
      S_StartSound (soundorg,sound);
      if (useAgain)
	P_StartButton (line,top,swtex,BUTTONTIME);
      side -> toptexture = switchlist[i^1];
      break;
    }
    else if (swtex == texMid)
    {
      soundorg = (mobj_t *)&line->soundorg;
      S_StartSound(soundorg,sound);
      if (useAgain)
	P_StartButton (line, middle,swtex,BUTTONTIME);
      side -> midtexture = switchlist[i^1];
      break;
    }
    else if (swtex == texBot)
    {
      soundorg = (mobj_t *)&line->soundorg;
      S_StartSound (soundorg,sound);
      if (useAgain)
	P_StartButton(line, bottom,swtex,BUTTONTIME);
      side -> bottomtexture = switchlist[i^1];
      break;
    }
    i++;
  } while (swtex != -1);
}

/* ----------------------------------------------------------------------- */
//
// P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//
// Returns non-zero if further lines beyond this one
// must not be acted upon.
//
boolean
P_UseSpecialLine
( mobj_t*	thing,
  line_t*	line,
  int		side )
{
  boolean rc;

    // Err...
    // Use the back sides of VERY SPECIAL lines...
    if (side)
    {
	switch(line->special)
	{
	  case 124:
	    // Sliding door open&close
	    // UNUSED?
	    break;

	  default:
	    return false;
	    break;
	}
    }


    // Switches that other things can activate.
    if (!thing->player)
    {
	// never open secret doors
	if (line->flags & ML_SECRET)
	    return false;

	switch(line->special)
	{
	  case 1: 	// MANUAL DOOR RAISE
	  case 32:	// MANUAL BLUE
	  case 33:	// MANUAL RED
	  case 34:	// MANUAL YELLOW
	  case 195:
	  case 174:
	  case 210:
	  case 209:
	    break;

	  default:
	    if ((P_MonsterCanOperate (line -> special, Switched) == false)
	     && (P_MonsterCanOperate (line -> special, Pressed) == false))
	      return false;
	    break;
	}
    }

    rc = false;

    // do something
    switch (line->special)
    {
	// MANUALS
      case 1:		// Vertical Door
      case 26:		// Blue Door/Locked
      case 27:		// Yellow Door /Locked
      case 28:		// Red Door /Locked

      case 31:		// Manual door open
      case 32:		// Blue locked door open
      case 33:		// Red locked door open
      case 34:		// Yellow locked door open

      case 117:		// Blazing door raise
      case 118:		// Blazing door open
	EV_VerticalDoor (line, thing);
	rc = true;
	break;

	//UNUSED - Door Slide Open&Close
	// case 124:
	// EV_SlidingDoor (line, thing);
	// break;

	// SWITCHES
      case 7:
	// Build Stairs
	if (EV_BuildStairs(line,stairnormalup8))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 9:
	// Change Donut
	if (EV_DoDonut(line))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 11:
	// Exit level
	P_ChangeSwitchTexture(line,0);
	G_ExitLevellater ();
	rc = true;
	break;

      case 14:
	// Raise Floor 32 and change texture
	if (EV_DoPlat(line,raiseAndChange32))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 15:
	// Raise Floor 24 and change texture
	if (EV_DoPlat(line,raiseAndChange24))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 18:
	// Raise Floor to next highest floor
	if (EV_DoFloor(line, raiseFloorToNearest))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 20:
	// Raise Plat next highest floor and change texture
	if (EV_DoPlat(line,raiseToNearestAndChange))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 21:
	// PlatDownWaitUpStay
	if (EV_DoPlat(line,downWaitUpStay))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 23:
	// Lower Floor to Lowest
	if (EV_DoFloor(line,lowerFloorToLowest))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 24:				// Gunned actions.
      case 46:				// We just return true here to prevent
      case 47:				// lines beyond this from being actioned.
	rc = true;			// Hacx Map 17:
	break;				// There's some switches behind shootable covers in this level.

      case 29:
	// Raise Door
	if (EV_DoDoor(line,normal))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 41:
	// Lower Ceiling to Floor
	if (EV_DoCeiling(line,lowerToFloor))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 71:
	// Turbo Lower Floor
	if (EV_DoFloor(line,turboLower))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 49:
	// Ceiling Crush And Raise
	if (EV_DoCeiling(line,crushAndRaise))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 50:
	// Close Door
	if (EV_DoDoor(line,normalClose))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 51:
	// Secret EXIT
	P_ChangeSwitchTexture(line,0);
	G_SecretExitLevellater ();
	rc = true;
	break;

      case 55:
	// Raise Floor Crush
	if (EV_DoFloor(line,raiseFloorCrush))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 101:
	// Raise Floor
	if (EV_DoFloor(line,raiseFloor))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 102:
	// Lower Floor to Surrounding floor height
	if (EV_DoFloor(line,lowerFloor))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 103:
	// Open Door
	if (EV_DoDoor(line,normalOpen))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 111:
	// Blazing Door Raise (faster than TURBO!)
	if (EV_DoDoor (line,blazeRaise))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 112:
	// Blazing Door Open (faster than TURBO!)
	if (EV_DoDoor (line,blazeOpen))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 113:
	// Blazing Door Close (faster than TURBO!)
	if (EV_DoDoor (line,blazeClose))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 122:
	// Blazing PlatDownWaitUpStay
	if (EV_DoPlat(line,blazeDWUS))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 127:
	// Build Stairs Turbo 16
	if (EV_BuildStairs(line,stairturboup16))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 131:
	// Raise Floor Turbo
	if (EV_DoFloor(line,raiseFloorTurbo))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 133:
	// BlzOpenDoor BLUE
	if (EV_DoLockedDoor (line,P_PD_BLUEO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,0);
	}
	rc = true;
	break;

      case 135:
	// BlzOpenDoor RED
	if (EV_DoLockedDoor (line,P_PD_REDO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,0);
	}
	rc = true;
	break;

      case 137:
	// BlzOpenDoor YELLOW
	if (EV_DoLockedDoor (line,P_PD_YELLOWO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,0);
	}
	rc = true;
	break;

      case 140:
	// Raise Floor 512
	if (EV_DoFloor(line,raiseFloor512))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

	// BUTTONS
      case 42:
	// Close Door
	if (EV_DoDoor(line,normalClose))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 43:
	// Lower Ceiling to Floor
	if (EV_DoCeiling(line,lowerToFloor))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 45:
	// Lower Floor to Surrounding floor height
	if (EV_DoFloor(line,lowerFloor))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 60:
	// Lower Floor to Lowest
	if (EV_DoFloor(line,lowerFloorToLowest))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 61:
	// Open Door
	if (EV_DoDoor(line,normalOpen))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 62:
	// PlatDownWaitUpStay
	if (EV_DoPlat(line,downWaitUpStay))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 63:
	// Raise Door
	if (EV_DoDoor(line,normal))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 64:
	// Raise Floor to ceiling
	if (EV_DoFloor(line,raiseFloor))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 66:
	// Raise Floor 24 and change texture
	if (EV_DoPlat(line,raiseAndChange24))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 67:
	// Raise Floor 32 and change texture
	if (EV_DoPlat(line,raiseAndChange32))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 65:
	// Raise Floor Crush
	if (EV_DoFloor(line,raiseFloorCrush))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 68:
	// Raise Plat to next highest floor and change texture
	if (EV_DoPlat(line,raiseToNearestAndChange))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 69:
	// Raise Floor to next highest floor
	if (EV_DoFloor(line, raiseFloorToNearest))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 70:
	// Turbo Lower Floor
	if (EV_DoFloor(line,turboLower))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 114:
	// Blazing Door Raise (faster than TURBO!)
	if (EV_DoDoor (line,blazeRaise))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 115:
	// Blazing Door Open (faster than TURBO!)
	if (EV_DoDoor (line,blazeOpen))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 116:
	// Blazing Door Close (faster than TURBO!)
	if (EV_DoDoor (line,blazeClose))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 123:
	// Blazing PlatDownWaitUpStay
	if (EV_DoPlat(line,blazeDWUS))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 132:
	// Raise Floor Turbo
	if (EV_DoFloor(line,raiseFloorTurbo))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 99:
	// BlzOpenDoor BLUE
	if (EV_DoLockedDoor (line,P_PD_BLUEO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,1);
	}
	rc = true;
	break;

      case 134:
	// BlzOpenDoor RED
	if (EV_DoLockedDoor (line,P_PD_REDO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,1);
	}
	rc = true;
	break;

      case 136:
	// BlzOpenDoor YELLOW
	if (EV_DoLockedDoor (line,P_PD_YELLOWO,blazeOpen,thing))
	{
	  P_ChangeSwitchTexture(line,1);
	}
	rc = true;
	break;

      case 138:
	// Light Turn On
	EV_LightTurnOn(line,255);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 139:
	// Light Turn Off
	EV_LightTurnOn(line,35);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

	/* jff 1/29/98 added linedef types to fill all functions out so that */
	/*  all possess SR, S1, WR, W1 types */

      case 158:
	/*  Raise Floor to shortest lower texture */
	/*  158 S1  EV_DoFloor(raiseToTexture), CSW(0) */
	if (EV_DoFloor(line,raiseToTexture))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 159:
	/*  Raise Floor to shortest lower texture */
	/*  159 S1  EV_DoFloor(lowerAndChange) */
	if (EV_DoFloor(line,lowerAndChange))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 160:
	/*  Raise Floor 24 and change */
	/*  160 S1  EV_DoFloor(changeAndRaiseFloor24) */
	if (EV_DoFloor(line,changeAndRaiseFloor24))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 161:
	/*  Raise Floor 24 */
	/*  161 S1  EV_DoFloor(raiseFloor24) */
	if (EV_DoFloor(line,raiseFloor24))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 162:
	/*  Moving floor min n to max n */
	/*  162 S1  EV_DoPlat(perpetualRaise) */
	if (EV_DoPlat(line,perpetualRaise))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 163:
	/*  Stop Moving floor */
	/*  163 S1  EV_DoPlat(perpetualRaise) */
	EV_StopPlat(line);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 164:
	/*  Start fast crusher */
	/*  164 S1  EV_DoCeiling(fastCrushAndRaise) */
	if (EV_DoCeiling(line,fastCrushAndRaise))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 165:
	/*  Start slow silent crusher */
	/*  165 S1  EV_DoCeiling(silentCrushAndRaise) */
	if (EV_DoCeiling(line,silentCrushAndRaise))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 166:
	/*  Raise ceiling, Lower floor */
	/*  166 S1 EV_DoCeiling(raiseToHighest), EV_DoFloor(lowerFloortoLowest) */
	if (EV_DoCeiling(line, raiseToHighest)
	 && (EV_DoFloor(line, lowerFloorToLowest)))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 167:
	/*  Lower floor and Crush */
	/*  167 S1 EV_DoCeiling(lowerAndCrush) */
	if (EV_DoCeiling(line, lowerAndCrush))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 168:
	/*  Stop crusher */
	/*  168 S1 EV_CeilingCrushStop() */
	if (EV_CeilingCrushStop(line))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 169:
	/*  Lights to brightest neighbour sector */
	/*  169 S1  EV_LightTurnOn(0) */
	EV_LightTurnOn(line,0);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 170:
	/*  Lights to near dark */
	/*  170 S1  EV_LightTurnOn(35) */
	EV_LightTurnOn(line,35);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 171:
	/*  Lights on full */
	/*  171 S1  EV_LightTurnOn(255) */
	EV_LightTurnOn(line,255);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 172:
	/*  Start Lights Strobing */
	/*  172 S1  EV_StartLightStrobing() */
	EV_StartLightStrobing(line);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 173:
	/*  Lights to Dimmest Near */
	/*  173 S1  EV_TurnTagLightsOff() */
	EV_TurnTagLightsOff(line);
	P_ChangeSwitchTexture(line,0);
	rc = true;
	break;

      case 174:
	/*  Teleport */
	/*  174 S1  EV_Teleport(side,thing) */
	if (EV_Teleport(line,side,thing))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 175:
	/*  Close Door, Open in 30 secs */
	/*  175 S1  EV_DoDoor(close30ThenOpen) */
	if (EV_DoDoor(line,close30ThenOpen))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 189: /* jff 3/15/98 create texture change no motion type */
	/*  Texture Change Only (Trigger) */
	/*  189 S1 Change Texture/Type Only */
	if (EV_DoChange(line,trigChangeOnly))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 203:
	/*  Lower ceiling to lowest surrounding ceiling */
	/*  203 S1 EV_DoCeiling(lowerToLowest) */
	if (EV_DoCeiling(line,lowerToLowest))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 204:
	/*  Lower ceiling to highest surrounding floor */
	/*  204 S1 EV_DoCeiling(lowerToMaxFloor) */
	if (EV_DoCeiling(line,lowerToMaxFloor))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 209:
	/*  killough 1/31/98: silent teleporter */
	/* jff 209 S1 SilentTeleport */
	if (EV_SilentTeleport(line, side, thing))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 241: /* jff 3/15/98 create texture change no motion type */
	/*  Texture Change Only (Numeric) */
	/*  241 S1 Change Texture/Type Only */
	if (EV_DoChange(line,numChangeOnly))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 221:
	/*  Lower floor to next lowest floor */
	/*  221 S1 Lower Floor To Nearest Floor */
	if (EV_DoFloor(line,lowerFloorToNearest))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 229:
	/*  Raise elevator next floor */
	/*  229 S1 Raise Elevator next floor */
	if (EV_DoElevator(line,elevateUp))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 233:
	/*  Lower elevator next floor */
	/*  233 S1 Lower Elevator next floor */
	if (EV_DoElevator(line,elevateDown))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

      case 237:
	/*  Elevator to current floor */
	/*  237 S1 Elevator to current floor */
	if (EV_DoElevator(line,elevateCurrent))
	{
	  P_ChangeSwitchTexture(line,0);
	  rc = true;
	}
	break;

	/*  jff 1/29/98 end of added S1 linedef types */

	/* jff 1/29/98 added linedef types to fill all functions out so that */
	/*  all possess SR, S1, WR, W1 types */

      case 78: /* jff 3/15/98 create texture change no motion type */
	/*  Texture Change Only (Numeric) */
	/*  78 SR Change Texture/Type Only */
	if (EV_DoChange(line,numChangeOnly))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 176:
	/*  Raise Floor to shortest lower texture */
	/*  176 SR  EV_DoFloor(raiseToTexture), CSW(1) */
	if (EV_DoFloor(line,raiseToTexture))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 177:
	/*  Raise Floor to shortest lower texture */
	/*  177 SR  EV_DoFloor(lowerAndChange) */
	if (EV_DoFloor(line,lowerAndChange))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 178:
	/*  Raise Floor 512 */
	/*  178 SR  EV_DoFloor(raiseFloor512) */
	if (EV_DoFloor(line,raiseFloor512))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 179:
	/*  Raise Floor 24 and change */
	/*  179 SR  EV_DoFloor(changeAndRaiseFloor24) */
	if (EV_DoFloor(line,changeAndRaiseFloor24))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 180:
	/*  Raise Floor 24 */
	/*  180 SR  EV_DoFloor(raiseFloor24) */
	if (EV_DoFloor(line,raiseFloor24))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 181:
	/*  Moving floor min n to max n */
	/*  181 SR  EV_DoPlat(perpetualRaise) */

	EV_DoPlat(line,perpetualRaise);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 182:
	/*  Stop Moving floor */
	/*  182 SR  EV_DoPlat(perpetualRaise) */
	EV_StopPlat(line);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 183:
	/*  Start fast crusher */
	/*  183 SR  EV_DoCeiling(fastCrushAndRaise) */
	if (EV_DoCeiling(line,fastCrushAndRaise))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 184:
	/*  Start slow crusher */
	/*  184 SR  EV_DoCeiling(crushAndRaise) */
	if (EV_DoCeiling(line,crushAndRaise))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 185:
	/*  Start slow silent crusher */
	/*  185 SR  EV_DoCeiling(silentCrushAndRaise) */
	if (EV_DoCeiling(line,silentCrushAndRaise))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 186:
	/*  Raise ceiling, Lower floor */
	/*  186 SR EV_DoCeiling(raiseToHighest), EV_DoFloor(lowerFloortoLowest) */
	if (EV_DoCeiling(line, raiseToHighest)
	 &&  EV_DoFloor(line, lowerFloorToLowest))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 187:
	/*  Lower floor and Crush */
	/*  187 SR EV_DoCeiling(lowerAndCrush) */
	if (EV_DoCeiling(line, lowerAndCrush))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 188:
	/*  Stop crusher */
	/*  188 SR EV_CeilingCrushStop() */
	if (EV_CeilingCrushStop(line))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 190: /* jff 3/15/98 create texture change no motion type */
	/*  Texture Change Only (Trigger) */
	/*  190 SR Change Texture/Type Only */
	if (EV_DoChange(line,trigChangeOnly))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 191:
	/*  Lower Pillar, Raise Donut */
	/*  191 SR  EV_DoDonut() */
	if (EV_DoDonut(line))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 192:
	/*  Lights to brightest neighbour sector */
	/*  192 SR  EV_LightTurnOn(0) */
	EV_LightTurnOn(line,0);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 193:
	/*  Start Lights Strobing */
	/*  193 SR  EV_StartLightStrobing() */
	EV_StartLightStrobing(line);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 194:
	/*  Lights to Dimmest Near */
	/*  194 SR  EV_TurnTagLightsOff() */
	EV_TurnTagLightsOff(line);
	P_ChangeSwitchTexture(line,1);
	rc = true;
	break;

      case 195:
	/*  Teleport */
	/*  195 SR  EV_Teleport(side,thing) */
	if (EV_Teleport(line,side,thing))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 196:
	/*  Close Door, Open in 30 secs */
	/*  196 SR  EV_DoDoor(close30ThenOpen) */
	if (EV_DoDoor(line,close30ThenOpen))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 205:
	/*  Lower ceiling to lowest surrounding ceiling */
	/*  205 SR EV_DoCeiling(lowerToLowest) */
	if (EV_DoCeiling(line,lowerToLowest))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 206:
	/*  Lower ceiling to highest surrounding floor */
	/*  206 SR EV_DoCeiling(lowerToMaxFloor) */
	if (EV_DoCeiling(line,lowerToMaxFloor))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 210:
	/*  killough 1/31/98: silent teleporter */
	/* jff 210 SR SilentTeleport */
	if (EV_SilentTeleport(line, side, thing))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 211: /* jff 3/14/98 create instant toggle floor type */
	/*  Toggle Floor Between C and F Instantly */
	/*  211 SR Toggle Floor Instant */
	if (EV_DoPlat(line,toggleUpDn))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 222:
	/*  Lower floor to next lowest floor */
	/*  222 SR Lower Floor To Nearest Floor */
	if (EV_DoFloor(line,lowerFloorToNearest))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 230:
	/*  Raise elevator next floor */
	/*  230 SR Raise Elevator next floor */
	if (EV_DoElevator(line,elevateUp))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 234:
	/*  Lower elevator next floor */
	/*  234 SR Lower Elevator next floor */
	if (EV_DoElevator(line,elevateDown))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 238:
	/*  Elevator to current floor */
	/*  238 SR Elevator to current floor */
	if (EV_DoElevator(line,elevateCurrent))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 258:
	/*  Build stairs, step 8 */
	/*  258 SR EV_BuildStairs(build8) */
	if (EV_BuildStairs(line,stairnormalup8))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

      case 259:
	/*  Build stairs, step 16 */
	/*  259 SR EV_BuildStairs(turbo16) */
	if (EV_BuildStairs(line,stairturboup16))
	{
	  P_ChangeSwitchTexture(line,1);
	  rc = true;
	}
	break;

	/*  1/29/98 jff end of added SR linedef types */

      default:
	rc = P_BoomSpecialLine (thing, line, side, Switched);
	if (rc == false)
	{
	  rc = P_BoomSpecialLine (thing, line, side, Pressed);
	  if ((rc == false)			// We just return true here to prevent
	   && (line -> special >= 0x2F80)	// lines beyond this from being actioned.
	   && ((unsigned)line -> special < 0x8000)	// Hacx Map 17:
	   && ((line -> special & 6) == (Gunned * 2)))
	  {
	    rc = true;
	  }
	}
    }

    return rc;
}

/* ----------------------------------------------------------------------- */

//#define SHOW_BOOM_SPECIAL
#ifdef SHOW_BOOM_SPECIAL
extern char * door_messages [];

static void P_Debug_BoomSpecialLine (int special)
{
  if (special < GenCrusherBase)
    return;

  printf ("P_BoomSpecialLine %X %X : ", special, special & 7);

  if (special >= GenFloorBase)
  {
    printf ("Gen Floor %X %X %X %X %X %X\n",
		genshift(special,FloorCrush,FloorCrushShift),
		genshift(special,FloorChange,FloorChangeShift),
		genshift(special,FloorTarget,FloorTargetShift),
		genshift(special,FloorDirection,FloorDirectionShift),
		genshift(special,FloorModel,FloorModelShift),
		genshift(special,FloorSpeed,FloorSpeedShift));
    return;
  }

  if (special >= GenCeilingBase)
  {
    printf ("Gen Ceiling %X %X %X %X %X %X\n",
		genshift(special,CeilingCrush,CeilingCrushShift),
		genshift(special,CeilingChange,CeilingChangeShift),
		genshift(special,CeilingTarget,CeilingTargetShift),
		genshift(special,CeilingDirection,CeilingDirectionShift),
		genshift(special,CeilingModel,CeilingModelShift),
		genshift(special,CeilingSpeed,CeilingSpeedShift));
    return;
  }

  if (special >= GenDoorBase)
  {
    printf ("Gen Door %X %X %X %X\n",
		genshift(special,DoorDelay,DoorDelayShift),
		genshift(special,DoorMonster,DoorMonsterShift),
		genshift(special,DoorKind,DoorKindShift),
		genshift(special,DoorSpeed,DoorSpeedShift));
    return;
  }

  if (special >= GenLockedBase)
  {
    printf ("Gen Locked door %X %X %X %X\n",
		genshift(special,LockedNKeys,LockedNKeysShift),
		genshift(special,LockedKey,LockedKeyShift),
		genshift(special,LockedKind,LockedKindShift),
		genshift(special,LockedSpeed,LockedSpeedShift));
    return;
  }

  if (special >= GenLiftBase)
  {
    printf ("Gen Lift %X %X %X %X\n",
		genshift(special,LiftTarget,LiftTargetShift),
		genshift(special,LiftDelay,LiftDelayShift),
		genshift(special,LiftMonster,LiftMonsterShift),
		genshift(special,LiftSpeed,LiftSpeedShift));
    return;
  }

  if (special >= GenStairsBase)
  {
    printf ("Gen Stairs %X %X %X %X %X\n",
		genshift(special,StairIgnore,StairIgnoreShift),
		genshift(special,StairDirection,StairDirectionShift),
		genshift(special,StairStep,StairStepShift),
		genshift(special,StairMonster,StairMonsterShift),
		genshift(special,StairSpeed,StairSpeedShift));
    return;
  }

  if (special >= GenCrusherBase)
  {
    printf ("Gen Crusher %X %X %X\n",
		genshift(special,CrusherSilent,CrusherSilentShift),
		genshift(special,CrusherMonster,CrusherMonsterShift),
		genshift(special,CrusherSpeed,CrusherSpeedShift));
    return;
  }
}

#endif
/* ----------------------------------------------------------------------- */

boolean P_BoomSpecialLine (mobj_t* thing, line_t* line, int side, triggered_e trigtype)
{
  unsigned int special;
  unsigned int key;
  //int amount;
  ceiling_e ceilingtype;
  boolean rc;


  rc = false;
  special = line -> special;

  if ((special & 6) != (trigtype * 2))
    return (rc);

  if (special >= 0x8000)
    return (rc);

#ifdef SHOW_BOOM_SPECIAL
  P_Debug_BoomSpecialLine (special);
#endif

  if (special >= GenFloorBase)
  {
    rc = (boolean) EV_DoFloor (line, (floor_e) special);
  }
  else if (special >= GenCeilingBase)
  {
    rc = (boolean) EV_DoCeiling (line, (ceiling_e) special);
  }
  else if (special >= GenDoorBase)
  {
    rc = (boolean) EV_DoGenDoor (line, (vldoor_e) special, thing);
  }
  else if (special >= GenLockedBase)
  {
    switch (genshift(special,LockedKey,LockedKeyShift))
    {
      case AnyKey:
	if ((special & 7) < 6)
	  key = P_PD_ANYOBJ;
	else
	  key = P_PD_ANY;
	break;

      case RCard:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_REDO;
	    break;
	  }
	  key = P_PD_REDCO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_REDK;
	  break;
	}
	key = P_PD_REDC;
	break;

      case BCard:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_BLUEO;
	    break;
	  }
	  key = P_PD_BLUECO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_BLUEK;
	  break;
	}
	key = P_PD_BLUEC;
	break;

      case YCard:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_YELLOWO;
	    break;
	  }
	  key = P_PD_YELLOWCO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_YELLOWK;
	  break;
	}
	key = P_PD_YELLOWC;
	break;

      case RSkull:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_REDO;
	    break;
	  }
	  key = P_PD_REDSO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_REDK;
	  break;
	}
	key = P_PD_REDS;
	break;

      case BSkull:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_BLUEO;
	    break;
	  }
	  key = P_PD_BLUESO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_BLUEK;
	  break;
	}
	key = P_PD_BLUES;
	break;

      case YSkull:
	if ((special & 7) < 6)
	{
	  if (special & LockedSkullOrCard)
	  {
	    key = P_PD_YELLOWO;
	    break;
	  }
	  key = P_PD_YELLOWSO;
	  break;
	}
	if (special & LockedSkullOrCard)
	{
	  key = P_PD_YELLOWK;
	  break;
	}
	key = P_PD_YELLOWS;
	break;

      default: /* AllKeys */
	if ((special & 7) < 6)
	{
	  if (special & LockedNKeys)
	    key = P_PD_ALL3O;
	  else
	    key = P_PD_ALL6O;
	}
	else
	{
	  if (special & LockedNKeys)
	    key = P_PD_ALL3;
	  else
	    key = P_PD_ALL6;
	}
    }
#ifdef SHOW_BOOM_SPECIAL
    printf ("Key = %u (%s)\n", key, door_messages [key]);
#endif
    rc = (boolean) EV_DoGenLockedDoor (line, key, (vldoor_e) special, thing);
  }
  else if (special >= GenLiftBase)
  {
    rc = (boolean) EV_DoPlat (line, (plattype_e) special);
  }
  else if (special >= GenStairsBase)
  {
    rc = (boolean) EV_BuildStairs (line, (stair_e) special);
    if (rc)
    {
      /* Generalised retriggerable stairs */
      /* alternate direction each time. */
      line -> special = special ^ StairDirection;
    }
  }
  else if (special >= GenCrusherBase)
  {
    switch (genshift(special,CrusherSpeed,CrusherSpeedShift))
    {
      case 0:
      case 1:
	ceilingtype = lowerAndCrush;
	break;
      default:
	ceilingtype = fastCrushAndRaise;
	if (special & CrusherSilent)
	  ceilingtype = silentCrushAndRaise;
    }

    rc = (boolean) EV_DoCeiling(line, ceilingtype);
  }

  if (rc == false)
  {
    /* We return TRUE for locked doors because we do not want */
    /* any further lines processing behind the door. */
    /* (Hacx/wad level 01) */
    if ((special >= GenLockedBase)
     && (special < 0x3C00))
      rc = true;
  }
  else
  {
    if ((special & TriggerType) > 1)
      P_ChangeSwitchTexture (line, special & 1);
    else if ((special & 1) == 0)
      line->special = 0;
  }

  return (rc);
}

/* ----------------------------------------------------------------------- */
