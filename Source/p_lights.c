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
//	Handle Sector base lighting effects.
//	Muzzle flash?
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_lights.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";
#endif

#include "includes.h"

#define CHASETIME 8
#define CHASESLOTS 8

//-----------------------------------------------------------------------------
//
// FIRELIGHT FLICKER
//

//
// T_FireFlicker
//
void T_FireFlicker (fireflicker_t* flick)
{
    int	amount;

    if (--flick->count)
	return;

    amount = (P_Random()&3)*16;

    if (flick->sector->lightlevel - amount < flick->minlight)
	flick->sector->lightlevel = flick->minlight;
    else
	flick->sector->lightlevel = flick->maxlight - amount;

    flick->count = 4;
}



//
// P_SpawnFireFlicker
//
void P_SpawnFireFlicker (sector_t* sector)
{
    fireflicker_t*	flick;

    // Note that we are resetting sector attributes.
    // Nothing special about it during gameplay.
    sector->special &= ~31;

    flick = Z_Malloc ( sizeof(*flick), PU_LEVSPEC, 0);

    sector->lightingdata = flick;

    P_AddThinker (&flick->thinker, (actionf_p1) T_FireFlicker);

    flick->sector = sector;
    flick->maxlight = sector->lightlevel;
    flick->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel)+16;
    flick->count = 4;
}



//
// BROKEN LIGHT FLASHING
//


//
// T_LightFlash
// Do flashing lights.
//
void T_LightFlash (lightflash_t* flash)
{
    if (--flash->count)
	return;

    if (flash->sector->lightlevel == flash->maxlight)
    {
	flash-> sector->lightlevel = flash->minlight;
	flash->count = (P_Random()&flash->mintime)+1;
    }
    else
    {
	flash-> sector->lightlevel = flash->maxlight;
	flash->count = (P_Random()&flash->maxtime)+1;
    }

}




//
// P_SpawnLightFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnLightFlash (sector_t* sector)
{
    lightflash_t*	flash;

    // nothing special about it during gameplay
    sector->special &= ~31;

    flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);

    sector->lightingdata = flash;

    P_AddThinker (&flash->thinker, (actionf_p1) T_LightFlash);

    flash->sector = sector;
    flash->maxlight = sector->lightlevel;

    flash->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
    flash->maxtime = 64;
    flash->mintime = 7;
    flash->count = (P_Random()&flash->maxtime)+1;
}



//
// STROBE LIGHT FLASHING
//


//
// T_StrobeFlash
//
void T_StrobeFlash (strobe_t* flash)
{
    if (--flash->count)
	return;

    if (flash->sector->lightlevel == flash->minlight)
    {
	flash-> sector->lightlevel = flash->maxlight;
	flash->count = flash->brighttime;
    }
    else
    {
	flash-> sector->lightlevel = flash->minlight;
	flash->count =flash->darktime;
    }
}



//
// P_SpawnStrobeFlash
// After the map has been loaded, scan each sector
// for specials that spawn thinkers
//
void P_SpawnStrobeFlash (sector_t* sector, int fastOrSlow, int inSync, int RemoveSpecial)
{
  strobe_t*	flash;

  // nothing special about it during gameplay except type 4.
  if (RemoveSpecial)
    sector->special &= ~31;

  flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);

  sector->lightingdata = flash;

  P_AddThinker (&flash->thinker, (actionf_p1) T_StrobeFlash);

  flash->sector = sector;
  flash->darktime = fastOrSlow;
  flash->brighttime = STROBEBRIGHT;
  flash->maxlight = sector->lightlevel;
  flash->minlight = P_FindMinSurroundingLight(sector, sector->lightlevel);

  if (flash->minlight == flash->maxlight)
    flash->minlight = 0;

  if (!inSync)
    flash->count = (P_Random()&7)+1;
  else
    flash->count = 1;
}



void P_SpawnLightPhased (sector_t* sector)
{
  unsigned int level;
  unsigned int min_light;
  strobe_t*	flash;

  // nothing special about it during gameplay
  sector->special &= ~31;

  flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);

  sector->lightingdata = flash;

  P_AddThinker (&flash->thinker, (actionf_p1) T_StrobeFlash);

  flash->sector = sector;
  flash->maxlight = sector->lightlevel;
  min_light = P_FindMinSurroundingLight (sector, sector->lightlevel);
  if (min_light >= flash->maxlight)
    min_light = 0;
  flash->minlight = min_light;

  flash->brighttime = CHASETIME;
  flash->darktime = CHASETIME*(CHASESLOTS-1);

  level = sector -> lightlevel;
  flash->count = (CHASESLOTS*CHASETIME) - (CHASETIME * (level % CHASETIME));	// 64 - 8 * pos & 7;
}


//
// Start strobing lights (usually from a trigger)
//
void EV_StartLightStrobing(line_t* line)
{
    int		secnum;
    sector_t*	sec;

    secnum = -1;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
	sec = &sectors[secnum];
#if 0
	if (P_SectorActive(lighting_special, sec))
	    continue;
#else
	if (sec->lightingdata)
	    continue;
#endif
	P_SpawnStrobeFlash (sec,SLOWDARK, 0, 0);
    }
}



//
// TURN LINE'S TAG LIGHTS OFF
//
void EV_TurnTagLightsOff (line_t* line)
{
    int		secnum;
    sector_t*	sector;

    secnum = -1;
    while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
    {
      sector = &sectors[secnum];
      sector->lightlevel = P_FindMinSurroundingLight (sector, sector->lightlevel);
    }
}


//
// TURN LINE'S TAG LIGHTS ON
//
void EV_LightTurnOn (line_t* line, int bright)
{
    int		j;
    int		secnum;
    sector_t*	sector;
    sector_t*	temp;
    line_t*	templine;

    secnum = -1;
    while ((secnum = P_FindSectorFromLineTag (line,secnum)) >= 0)
    {
	sector = &sectors[secnum];
	// bright = 0 means to search
	// for highest light level
	// surrounding sector
	if (!bright)
	{
	    for (j = 0;j < sector->linecount; j++)
	    {
		templine = sector->lines[j];
		temp = getNextSector(templine,sector);

		if (!temp)
		    continue;

		if (temp->lightlevel > bright)
		    bright = temp->lightlevel;
	    }
	}

	sector-> lightlevel = bright;
    }
}


//
// Spawn glowing light
//

void T_Glow (glow_t* g)
{
    switch(g->direction)
    {
      case -1:
	// DOWN
	g->sector->lightlevel -= GLOWSPEED;
	if (g->sector->lightlevel <= g->minlight)
	{
	    g->sector->lightlevel += GLOWSPEED;
	    g->direction = 1;
	}
	break;

      case 1:
	// UP
	g->sector->lightlevel += GLOWSPEED;
	if (g->sector->lightlevel >= g->maxlight)
	{
	    g->sector->lightlevel -= GLOWSPEED;
	    g->direction = -1;
	}
	break;
    }
}


void P_SpawnGlowingLight(sector_t* sector)
{
    glow_t*	g;

    sector->special &= ~31;

    g = Z_Malloc( sizeof(*g), PU_LEVSPEC, 0);

    sector->lightingdata = g;

    P_AddThinker(&g->thinker, (actionf_p1) T_Glow);

    g->sector = sector;
    g->minlight = P_FindMinSurroundingLight(sector,sector->lightlevel);
    g->maxlight = sector->lightlevel;
    g->direction = -1;
}




static /*unsigned int*/ void P_SpawnLightChaseR (sector_t* sector, unsigned int mode, unsigned int min_light, unsigned int max_light, unsigned int level)
{
  unsigned int	i;
  unsigned int	this_mode;
  unsigned int	next_level;
  unsigned int  this_max_light;
  unsigned int  this_min_light;
  sector_t*	other;
  strobe_t*	flash;


  this_max_light = sector -> lightlevel;
  if (this_max_light == 0)
    this_max_light = max_light;

  this_min_light = P_FindMinSurroundingLight (sector, this_max_light);

  if (this_min_light >= this_max_light)
  {
    this_min_light = min_light;
    if (this_min_light >= this_max_light)
      this_min_light = 0;
  }

  sector -> lightlevel = this_max_light;


  /* Follow the sequence of 23,24 specials */
  /* First mark each neighbour as belonging to this connection */
  /* in case there is a circular link. */
  next_level = level + 1;
  for (i=0 ;i < sector->linecount ; i++)
  {
    other = getNextSector(sector->lines[i],sector);
    if (other)
    {
      switch (other -> special & 31)
      {
	case 23:
	case 24:
	  if (other->lightingdata == NULL)
	    other->lightingdata = (void*)((pint)next_level);
      }
    }
  }

  for (i=0 ;i < sector->linecount ; i++)
  {
    other = getNextSector(sector->lines[i],sector);
    if ((other)
     && (other -> lightingdata == ((void*) ((pint)(level+1)))))
    {
      this_mode = other -> special;
      switch (this_mode & 31)
      {
	case 23:
	case 24:
	  other -> special = this_mode & ~31;
	  this_mode &= 31;
	  next_level = level;
	  if (this_mode != mode)
	    next_level++;
	  P_SpawnLightChaseR (other, this_mode, this_min_light, this_max_light, next_level);
      }
    }
  }

  flash = Z_Malloc ( sizeof(*flash), PU_LEVSPEC, 0);

  sector->lightingdata = flash;

  P_AddThinker (&flash->thinker, (actionf_p1) T_StrobeFlash);

  flash->sector = sector;
  flash->minlight = this_min_light;
  flash->maxlight = this_max_light;
  flash->brighttime = CHASETIME;
  flash->darktime = CHASETIME*(CHASESLOTS-1);
  flash->count = (CHASESLOTS*CHASETIME) - (CHASETIME * (level % CHASETIME));	// 64 - 8 * pos & 7;
}

//
// P_SpawnLightChase
//
// Test: Herian2 maps 6, 10, 23 & 29

void P_SpawnLightChase (sector_t* sector)
{
  unsigned int min_light;
  unsigned int max_light;

  // nothing special about it during gameplay
  sector->special &= ~31;

  max_light = sector->lightlevel;
  if (max_light == 0)
    max_light = 255;

  min_light = P_FindMinSurroundingLight (sector, max_light);
  if (min_light >= max_light)
    min_light = 0;

  P_SpawnLightChaseR (sector, 22, min_light, max_light, 0);
}

