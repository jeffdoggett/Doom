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
//	Implements special effects:
//	Texture animation, height or lighting changes
//	 according to adjacent sectors, respective
//	 utility functions, etc.
//	Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_spec.c,v 1.6 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"

extern flatinfo_t* flatinfo;

/* ---------------------------------------------------------------- */

char * special_effects_messages_orig [] =
{
  FOUND_SECRET,
  NULL
};

char * special_effects_messages [ARRAY_SIZE(special_effects_messages_orig)];

typedef enum
{
  PS_FOUND_SECRET
} special_effects_text_t;


/* ---------------------------------------------------------------- */
//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
typedef struct anim_s
{
  struct anim_s* next;
  boolean	istexture;
  int		picnum;
  int		basepic;
  int		numpics;
  int		speed;
} anim_t;

//
//      source animation definition
//
typedef struct
{
  boolean	istexture;	// if false, it is a flat
  char		endname[9];
  char		startname[9];
  int		speed;
} animdef_t;

typedef struct animdef2_s
{
  boolean	istexture;	// if false, it is a flat
  char		endname[9];
  char		startname[9];
  int		speed;
  struct animdef2_s * next;
} animdef2_t;

static void P_SpawnScrollers(void);
static void P_SpawnFriction(void);
static void P_SpawnPushers(void);

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//
static animdef_t animdefs[] =
{
  {false,	"NUKAGE3",	"NUKAGE1",	8},
  {false,	"FWATER4",	"FWATER1",	8},
  {false,	"SWATER4",	"SWATER1", 	8},
  {false,	"LAVA4",	"LAVA1",	8},
  {false,	"BLOOD3",	"BLOOD1",	8},

  // DOOM II flat animations.
  {false,	"RROCK08",	"RROCK05",	8},
  {false,	"SLIME04",	"SLIME01",	8},
  {false,	"SLIME08",	"SLIME05",	8},
  {false,	"SLIME12",	"SLIME09",	8},

  {true,	"BLODGR4",	"BLODGR1",	8},
  {true,	"SLADRIP3",	"SLADRIP1",	8},

  {true,	"BLODRIP4",	"BLODRIP1",	8},
  {true,	"FIREWALL",	"FIREWALA",	8},
  {true,	"GSTFONT3",	"GSTFONT1",	8},
  {true,	"FIRELAVA",	"FIRELAV3",	8},
  {true,	"FIREMAG3",	"FIREMAG1",	8},
  {true,	"FIREBLU2",	"FIREBLU1",	8},
  {true,	"ROCKRED3",	"ROCKRED1",	8},

  {true,	"BFALL4",	"BFALL1",	8},
  {true,	"SFALL4",	"SFALL1",	8},
  {true,	"WFALL4",	"WFALL1",	8},
  {true,	"DBRAIN4",	"DBRAIN1",	8}
};

static anim_t*	animhead;

/* ---------------------------------------------------------------------------- */
/*
   See if we've already seen this animdef.
*/

static animdef_t * find_existing_anim (animdef2_t * lumps, const char * name_1, const char * name_2)
{
  int count;
  animdef_t * an_ptr;
  animdef2_t * an2_ptr;

  /* Look first in the fixed array */

  count = ARRAY_SIZE (animdefs);
  an_ptr = animdefs;
  do
  {
    if ((strcasecmp (an_ptr -> endname, name_1) == 0)
     || (strcasecmp (an_ptr -> startname, name_2) == 0))
      return (an_ptr);
    an_ptr++;
  } while (--count);

  /* Not there, see if in the new linked list. */

  for (an2_ptr = lumps; an2_ptr != NULL; an2_ptr = an2_ptr -> next)
  {
    if ((strcasecmp (an2_ptr -> endname, name_1) == 0)
     || (strcasecmp (an2_ptr -> startname, name_2) == 0))
      return ((animdef_t *) an2_ptr);
  }

  return (NULL);
}

/* ---------------------------------------------------------------------------- */
/*
  Read the ANIMATED lump from the wad
  23 bytes per entry
  Byte 0: true/false
  Bytes 1-9: First name
  Bytes 10-18: Second name
  Bytes 19-22: speed
*/

static animdef2_t * Read_ANIMATED_Lump (void)
{
  int i;
  int lump;
  int size;
  int speed;
  int istexture;
  char * lump_ptr;
  char * ptr;
  animdef_t * an_ptr;
  animdef2_t * lumphead;
  animdef2_t * thisanim;
  animdef2_t ** lastanim;
  char name_1 [10];
  char name_2 [10];

  lumphead = NULL;
  lastanim = &lumphead;

  lump = -1;
  while ((lump = W_NextLumpNumForName ("ANIMATED", lump)) != -1)
  {
    ptr = lump_ptr = W_CacheLumpNum (lump, PU_STATIC);
    size = W_LumpLength (lump);
    do
    {
      istexture = *ptr++;
      i = 0;
      do
      {
	name_1 [i++] = *ptr++;
      } while (i < 9);
      i = 0;
      do
      {
	name_2 [i++] = *ptr++;
      } while (i < 9);

      speed = *ptr++;
      speed |= (*ptr++ << 8);
      speed |= (*ptr++ << 16);
      speed |= (*ptr++ << 24);

      if ((name_1 [0]) && (name_2 [0]))
      {
	/* See whether this anim is a duplicate of an existing one. */
	/* If it is, then just overwrite it, otherwise create a new */
	/* entity in the temporary linked list. */
	an_ptr = find_existing_anim (lumphead, name_1, name_2);
	if (an_ptr == NULL)
	{
	  thisanim = malloc (sizeof (animdef2_t));
	  if (thisanim == NULL)
	    break;

	  *lastanim = thisanim;
	  lastanim = &thisanim -> next;
	  *lastanim = NULL;

	  an_ptr = (animdef_t *) thisanim;
	}

//	printf ("Anim %d %d: %s %s %d %d\n", i, an_ptr -> istexture, name_1, name_2, istexture, speed);
	an_ptr -> istexture = (boolean) istexture;
	strcpy (an_ptr -> endname, name_1);
	strcpy (an_ptr -> startname, name_2);
	an_ptr -> speed = speed;
      }

      size -= 23;
    } while (size >= 23);

    Z_Free (lump_ptr);
  }

  return (lumphead);
}

/* ---------------------------------------------------------------------------- */

static anim_t* read_anim (animdef_t * an_ptr)
{
  int   bp;
  int   pn;
  int	numpics;
  anim_t* thisanim;

  if (an_ptr -> istexture)
  {
    bp = R_CheckTextureNumForName (an_ptr -> startname);
    pn = R_CheckTextureNumForName (an_ptr -> endname);
  }
  else
  {
    bp = R_CheckFlatNumForName (an_ptr -> startname);
    pn = R_CheckFlatNumForName (an_ptr -> endname);
  }

  thisanim = NULL;

  if ((bp != -1) && (pn != -1))
  {
    numpics = pn - bp + 1;
    if (numpics < 2)
    {
      fprintf (stderr, "P_InitPicAnims: bad cycle from %s to %s\n",
		an_ptr -> startname,
		an_ptr -> endname);
    }
    else
    {
      thisanim = malloc (sizeof (anim_t));
      if (thisanim)
      {
	thisanim->basepic = bp;
	thisanim->picnum = pn;
	thisanim->numpics = numpics;
	thisanim->istexture = an_ptr -> istexture;
	thisanim->speed = an_ptr -> speed;
      }
    }
  }

  return (thisanim);
}

/* ---------------------------------------------------------------------------- */

void P_InitPicAnims (void)
{
  int   count;
  anim_t** prevanim;
  anim_t* thisanim;
  animdef_t * animdef;
  animdef2_t * lumphead;
  animdef2_t * next;

  /* Read the ANIMATED lump from the WAD file. */
  /* Any duplicates will overwrite the existing entries. */
  /* Unique animations will be placed in a temporary */
  /* linked list, which we'll free later as it's read. */
  lumphead = Read_ANIMATED_Lump ();

  //	Init animation
  prevanim = &animhead;
  count = ARRAY_SIZE (animdefs);
  animdef = animdefs;

  do
  {
    thisanim = read_anim (animdef);
    if (thisanim)
    {
      *prevanim = thisanim;
      prevanim = &thisanim -> next;
    }
    animdef++;
  } while (--count);

  if (lumphead)				// Have we any extra ones?
  {
    do
    {
      thisanim = read_anim ((animdef_t*) lumphead);
      if (thisanim)
      {
	*prevanim = thisanim;
	prevanim = &thisanim -> next;
      }
      next = lumphead -> next;
      free (lumphead);
      lumphead = next;
    } while (lumphead);
  }

  *prevanim = NULL;
}

/* ---------------------------------------------------------------------------- */
//
// UTILITIES
//



//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t*
getSide
( int		currentSector,
  int		line,
  int		side )
{
  return &sides[ (sectors[currentSector].lines[line])->sidenum[side] ];
}


/* ---------------------------------------------------------------------------- */
//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t*
getSector
( int		currentSector,
  int		line,
  int		side )
{
  return sides[ (sectors[currentSector].lines[line])->sidenum[side] ].sector;
}


/* ---------------------------------------------------------------------------- */
//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int
twoSided
( int	sector,
  int	line )
{
  return (sectors[sector].lines[line])->flags & ML_TWOSIDED;
}




/* ---------------------------------------------------------------------------- */
//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t*
getNextSector
( line_t*	line,
  sector_t*	sec )
{
  if (!(line->flags & ML_TWOSIDED))
    return NULL;

  if (line->frontsector == sec)
    return line->backsector;

  return line->frontsector;
}



/* ---------------------------------------------------------------------------- */
//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t	P_FindLowestFloorSurrounding (sector_t* sec)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	floor = sec->floorheight;

  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,sec);

    if ((other)
     && (other != sec)
     && (other->floorheight < floor))
      floor = other->floorheight;
  }
  return floor;
}

/* ---------------------------------------------------------------------------- */

fixed_t P_FindNextLowestFloor (const sector_t *sec, int currentheight)
{
  sector_t *other;
  int i;

  //BOOMTRACEOUT("P_FindNextLowestFloor")

  for (i=0 ;i < sec->linecount ; i++)
    if (((other = getNextSector(sec->lines[i],(sector_t*)sec)) != 0) &&
	 other->floorheight < currentheight)
    {
      int height = other->floorheight;
      while (++i < sec->linecount)
	if (((other = getNextSector(sec->lines[i],(sector_t*)sec)) != 0) &&
	    other->floorheight > height &&
	    other->floorheight < currentheight)
	  height = other->floorheight;
      return height;
    }
  return currentheight;
}

/* ---------------------------------------------------------------------------- */
//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t	P_FindHighestFloorSurrounding(sector_t *sec)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	floor;

  floor = MININT;			// Was -500*FRACUNIT - another well known vanilla bug.
  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,sec);

    if ((other)
     && (other != sec)
     && (other->floorheight > floor))
      floor = other->floorheight;
  }

  return floor;
}

/* ---------------------------------------------------------------------------- */
//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS

fixed_t
P_FindNextHighestFloor
( sector_t*	sec,
  int		currentheight )
{
  int		i;
  int		min;
  line_t*	check;
  sector_t*	other;
  fixed_t	height;

  height = currentheight;
  min = MAXINT;

  for (i=0; i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,sec);

    if ((other)
     && (other->floorheight > height)
     && (other->floorheight < min))
      min = other->floorheight;
  }

  if (min == MAXINT)
      min = height;

  return min;
}


/* ---------------------------------------------------------------------------- */
//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t
P_FindLowestCeilingSurrounding(sector_t* sec)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	height = MAXINT;

  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,sec);

    if ((other)
     && (other != sec)
     && (other->ceilingheight < height))
      height = other->ceilingheight;
  }
  return height;
}


/* ---------------------------------------------------------------------------- */
//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t	P_FindHighestCeilingSurrounding(sector_t* sec)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	height;

  height = MININT;
  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,sec);

    if ((other)
     && (other != sec)
     && (other->ceilingheight > height))
      height = other->ceilingheight;
  }

  return height;
}

/* ---------------------------------------------------------------------------- */

fixed_t P_FindNextHighestCeiling (const sector_t *sec, int currentheight)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	height = MAXINT;

  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,(sector_t *)sec);

    if ((other)
     && (other->ceilingheight > currentheight)
     && (other->ceilingheight < height))
      height = other->ceilingheight;
  }

  if (height == MAXINT)
    height = currentheight;

  return height;
}

/* ---------------------------------------------------------------------------- */

fixed_t P_FindNextLowestCeiling(const sector_t *sec, int currentheight)
{
  int		i;
  line_t*	check;
  sector_t*	other;
  fixed_t	height = MININT;

  for (i=0 ;i < sec->linecount ; i++)
  {
    check = sec->lines[i];
    other = getNextSector(check,(sector_t *)sec);

    if ((other)
     && (other->ceilingheight < currentheight)
     && (other->ceilingheight > height))
      height = other->ceilingheight;
  }

  if (height == MININT)
    height = currentheight;

  return height;
}

/* ---------------------------------------------------------------------------- */
//
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//
// Find the next sector with the same tag as a linedef.
// Rewritten by Lee Killough to use chained hashing to improve speed

int P_FindSectorFromLineTag (line_t *line, int start)
{
  int tag;
  int secnum;
  sector_t* sec;

  // If the line has no tag then operate the associated sector.
  // Required in Freedoom II Level 30 (Keyed doors near start).

  if ((tag = line->tag) == 0)
  {
    if (start == -1)			// Only if it is the first time here
    {
      if ((line ->flags & ML_TWOSIDED)
       && ((sec = sides[ line->sidenum[0^1]] .sector) != NULL))
	return (sec-sectors);

      line->special = 0;		// Yet another badly built wad!!
    }
    return (-1);
  }

  secnum = (start >= 0 ? sectors[start].nexttag :
    sectors[(unsigned)tag % (unsigned)numsectors].firsttag);

  while (secnum >= 0 && sectors[secnum].tag != tag)
    secnum = sectors[secnum].nexttag;

  return secnum;
}

/* ---------------------------------------------------------------------------- */
// killough 4/16/98: Same thing, only for linedefs

int P_FindLineFromTag(int tag, int start)
{
  int linenum;

  if (tag == 0)
    return (-1);

  linenum = (start >= 0 ? lines[start].nexttag :
    lines[(unsigned)tag % (unsigned)numlines].firsttag);

  while (linenum >= 0 && lines[linenum].tag != tag)
    linenum = lines[linenum].nexttag;

  return linenum;
}

/* ---------------------------------------------------------------------------- */
// Hash the sector tags across the sectors and linedefs.

static void P_InitTagLists(void)
{
  register int i;

  for (i = numsectors; --i >= 0;) // Initially make all slots empty.
    sectors[i].firsttag = -1;
  for (i = numsectors; --i >= 0;) // Proceed from last to first sector
  { // so that lower sectors appear first
    int j = (unsigned)sectors[i].tag % (unsigned)numsectors; // Hash func

    sectors[i].nexttag = sectors[j].firsttag; // Prepend sector to chain
    sectors[j].firsttag = i;
  }

  // killough 4/17/98: same thing, only for linedefs
  for (i = numlines; --i >= 0;) // Initially make all slots empty.
    lines[i].firsttag = -1;
  for (i = numlines; --i >= 0;) // Proceed from last to first linedef
  { // so that lower linedefs appear first
    int j = (unsigned)lines[i].tag % (unsigned)numlines; // Hash func

    lines[i].nexttag = lines[j].firsttag; // Prepend linedef to chain
    lines[j].firsttag = i;
  }
}

/* ---------------------------------------------------------------------------- */
//
// Find minimum light from an adjacent sector
//
int
P_FindMinSurroundingLight
( sector_t*	sector,
  int		max )
{
  int		i;
  line_t*	line;
  sector_t*	check;

  for (i=0 ; i < sector->linecount ; i++)
  {
    line = sector->lines[i];
    check = getNextSector(line,sector);

    if ((check)
     && (check->lightlevel < max))
      max = check->lightlevel;
  }

  return (max);
}

/* ---------------------------------------------------------------------------- */
//
// Find maximum light from an adjacent sector
//
int
P_FindMaxSurroundingLight
( sector_t*	sector,
  int		min )
{
  int		j;
  sector_t*	temp;
  line_t*	templine;

  for (j = 0;j < sector->linecount; j++)
  {
    templine = sector->lines[j];
    temp = getNextSector(templine,sector);

    if ((temp)
     && (temp->lightlevel > min))
      min = temp->lightlevel;
  }

  return (min);
}

/* ---------------------------------------------------------------------------- */

boolean P_MonsterCanOperate (unsigned int special, triggered_e trigtype)
{
  if ((special & 6) != (trigtype * 2))
    return (false);

#if 0
  if (special >= 0x8000)
  {
  }
  else if (special >= GenFloorBase)
  {
  }
#endif
  else if (special >= GenCeilingBase)
  {
    // CeilingModel is "Allow Monsters" if CeilingChange is 0
    if (((special & CeilingChange) == 0)
     && (special & CeilingModel))
      return (true);
  }
  else if (special >= GenDoorBase)
  {
    if (special & DoorMonster)
      return (true);
  }
  else if (special >= GenLockedBase)
  {
  }
  else if (special >= GenLiftBase)
  {
    if (special & LiftMonster)
      return (true);
  }
  else if (special >= GenStairsBase)
  {
    if (special & StairMonster)
      return (true);
  }
  else if (special >= GenCrusherBase)
  {
    if (special & CrusherMonster)
      return (true);
  }

  return (false);
}

/* ---------------------------------------------------------------------------- */
//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
boolean
P_CrossSpecialLine
( line_t*	line,
  int		side,
  mobj_t*	thing )
{
  boolean rc;

  //	Triggers that other things can activate
  if (!thing->player)
  {
    // Things that should NOT trigger specials...
    switch(thing->type)
    {
      case MT_ROCKET:
      case MT_PLASMA:
      case MT_BFG:
      case MT_TROOPSHOT:
      case MT_HEADSHOT:
      case MT_BRUISERSHOT:
	return (false);

      default: break;
    }

    switch(line->special)
    {
      case 39:	// TELEPORT TRIGGER
      case 97:	// TELEPORT RETRIGGER
      case 125:	// TELEPORT MONSTERONLY TRIGGER
      case 126:	// TELEPORT MONSTERONLY RETRIGGER
      case 4:	// RAISE DOOR
      case 10:	// PLAT DOWN-WAIT-UP-STAY TRIGGER
      case 88:	// PLAT DOWN-WAIT-UP-STAY RETRIGGER

      case 207:	// W1 silent teleporter (normal kind)
      case 208:	// WR silent teleporter (normal kind)
      case 243:	// W1 silent teleporter (linedef-linedef kind)
      case 244:	// WR silent teleporter (linedef-linedef kind)
      case 262:	// W1 silent teleporter (linedef-linedef kind)
      case 263:	// WR silent line-line reversed
      case 264:	// W1 monster-only silent line-line reversed
      case 265:	// WR monster-only silent line-line reversed
      case 266:	// W1 monster-only silent line-line
      case 267:	// WR monster-only silent line-line
      case 268:	// W1 monster-only silent
      case 269:	// WR monster-only silent
	break;

      default:
	rc = P_MonsterCanOperate (line -> special, WalkedOver);
	if (rc == false)
	  return (rc);
    }
  }

#if 0
  printf ("Crossed line %u\n", line->special);
#endif

  rc = false;

  // Note: could use some const's here.
  switch (line->special)
  {
      // TRIGGERS.
      // All from here to RETRIGGERS.
    case 2:
      // Open Door
      if (EV_DoDoor(line,normalOpen))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 3:
      // Close Door
      if (EV_DoDoor(line,normalClose))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 4:
      // Raise Door
      if (EV_DoDoor(line,normal))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 5:
      // Raise Floor
      if (EV_DoFloor(line,raiseFloor))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 6:
      // Fast Ceiling Crush & Raise
      if (EV_DoCeiling(line,fastCrushAndRaise))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 8:
      // Build Stairs
      if (EV_BuildStairs(line,stairnormalup8))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 10:
      // PlatDownWaitUp
      if (EV_DoPlat(line,downWaitUpStay))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 12:
      // Light Turn On - brightest near
      EV_LightTurnOn(line,0);
      line->special = 0;
      rc = true;
      break;

    case 13:
      // Light Turn On 255
      EV_LightTurnOn(line,255);
      line->special = 0;
      rc = true;
      break;

    case 16:
      // Close Door 30
      if (EV_DoDoor(line,close30ThenOpen))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 17:
      // Start Light Strobing
      EV_StartLightStrobing(line);
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 19:
      // Lower Floor
      if (EV_DoFloor(line,lowerFloor))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 22:
      // Raise floor to nearest height and change texture
      if (EV_DoPlat(line,raiseToNearestAndChange))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 25:
      // Ceiling Crush and Raise
      if (EV_DoCeiling(line,crushAndRaise))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 30:
      // Raise floor to shortest texture height
      //  on either side of lines.
      if (EV_DoFloor(line,raiseToTexture))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 35:
      // Lights Very Dark
      EV_LightTurnOn(line,35);
      line->special = 0;
      rc = true;
      break;

    case 36:
      // Lower Floor (TURBO)
      if (EV_DoFloor(line,turboLower))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 37:
      // LowerAndChange
      if (EV_DoFloor(line,lowerAndChange))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 38:
      // Lower Floor To Lowest
      if (EV_DoFloor( line, lowerFloorToLowest))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 39:
      // TELEPORT!
      if (EV_Teleport( line, side, thing ))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 40:
      // RaiseCeilingLowerFloor

      // A bug in original doom meant that the lowerFloorToLowest could
      // never happen because Standard Doom cannot actually do two things
      // at the same time with the same sector.
      // Eternal doom level 29 relies on this behavior.
#if 1
      {
	if (EV_DoCeiling(line, raiseToHighest))
	{
	  line->special = 0;
	  rc = true;
	}
      }
#else
      {
	int flags;

	flags = 0;
	if (EV_DoCeiling(line, raiseToHighest))
	  flags |= 1;
	if (EV_DoFloor( line, lowerFloorToLowest ))
	  flags |= 2;
	switch (flags)
	{
	  case 1:
	    line->special = 38;
	    break;
	  case 2:		// Boom line def (Ceil raiseToHighest)
	    line->special = GenCeilingBase + CeilingDirection + CtoHnC + WalkOnce;
	    break;
	  case 3:
	    line->special = 0;
	}
	rc = true;
      }
#endif
      break;

    case 44:
      // Ceiling Crush
      if (EV_DoCeiling( line, lowerAndCrush ))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 52:
      // EXIT!
      G_ExitLevel ();
      rc = true;
      break;

    case 53:
      // Perpetual Platform Raise
      if (EV_DoPlat(line,perpetualRaise))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 54:
      // Platform Stop
      EV_StopPlat(line);
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 56:
      // Raise Floor Crush
      if (EV_DoFloor(line,raiseFloorCrush))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 57:
      // Ceiling Crush Stop
      if (EV_CeilingCrushStop(line))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 58:
      // Raise Floor 24
      if (EV_DoFloor(line,raiseFloor24))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 59:
      // Raise Floor 24 And Change
      if (EV_DoFloor(line,changeAndRaiseFloor24))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 104:
      // Turn lights off in sector(tag)
      EV_TurnTagLightsOff(line);
      line->special = 0;
      rc = true;
      break;

    case 108:
      // Blazing Door Raise (faster than TURBO!)
      if (EV_DoDoor (line,blazeRaise))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 109:
      // Blazing Door Open (faster than TURBO!)
      if (EV_DoDoor (line,blazeOpen))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 100:
      // Build Stairs Turbo 16
      if (EV_BuildStairs(line,stairturboup16))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 110:
      // Blazing Door Close (faster than TURBO!)
      if (EV_DoDoor (line,blazeClose))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 119:
      // Raise floor to nearest surr. floor
      if (EV_DoFloor(line,raiseFloorToNearest))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 121:
      // Blazing PlatDownWaitUpStay
      if (EV_DoPlat(line,blazeDWUS))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 124:
      // Secret EXIT
      G_SecretExitLevel ();
      rc = true;
      break;

    case 125:
      // TELEPORT MonsterONLY
      if (!thing->player)
      {
	if (EV_Teleport( line, side, thing ))
	{
	  line->special = 0;
	  rc = true;
	}
      }
      break;

    case 130:
      // Raise Floor Turbo
      if (EV_DoFloor(line,raiseFloorTurbo))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 141:
      // Silent Ceiling Crush & Raise
      if (EV_DoCeiling(line,silentCrushAndRaise))
      {
	line->special = 0;
	rc = true;
      }
      break;

      // RETRIGGERS.  All from here till end.
    case 72:
      // Ceiling Crush
      if (EV_DoCeiling( line, lowerAndCrush ))
	rc = true;
      break;

    case 73:
      // Ceiling Crush and Raise
      if (EV_DoCeiling(line,crushAndRaise))
	rc = true;
      break;

    case 74:
      // Ceiling Crush Stop
      if (EV_CeilingCrushStop(line))
	rc = true;
      break;

    case 75:
      // Close Door
      if (EV_DoDoor(line,normalClose))
	rc = true;
      break;

    case 76:
      // Close Door 30
      if (EV_DoDoor(line,close30ThenOpen))
	rc = true;
      break;

    case 77:
      // Fast Ceiling Crush & Raise
      if (EV_DoCeiling(line,fastCrushAndRaise))
	rc = true;
      break;

    case 79:
      // Lights Very Dark
      EV_LightTurnOn(line,35);
      rc = true;
      break;

    case 80:
      // Light Turn On - brightest near
      EV_LightTurnOn(line,0);
      rc = true;
      break;

    case 81:
      // Light Turn On 255
      EV_LightTurnOn(line,255);
      rc = true;
      break;

    case 82:
      // Lower Floor To Lowest
      if (EV_DoFloor( line, lowerFloorToLowest ))
	rc = true;
      break;

    case 83:
      // Lower Floor
      if (EV_DoFloor(line,lowerFloor))
	rc = true;
      break;

    case 84:
      // LowerAndChange
      if (EV_DoFloor(line,lowerAndChange))
	rc = true;
      break;

    case 86:
      // Open Door
      if (EV_DoDoor(line,normalOpen))
	rc = true;
      break;

    case 87:
      // Perpetual Platform Raise
      if (EV_DoPlat(line,perpetualRaise))
	rc = true;
      break;

    case 88:
      // PlatDownWaitUp
      if (EV_DoPlat(line,downWaitUpStay))
	rc = true;
      break;

    case 89:
      // Platform Stop
      EV_StopPlat(line);
      rc = true;
      break;

    case 90:
      // Raise Door
      EV_DoDoor(line,normal);
      break;

    case 91:
      // Raise Floor
      if (EV_DoFloor(line,raiseFloor))
	rc = true;
      break;

    case 92:
      // Raise Floor 24
      if (EV_DoFloor(line,raiseFloor24))
	rc = true;
      break;

    case 93:
      // Raise Floor 24 And Change
      if (EV_DoFloor(line,changeAndRaiseFloor24))
	rc = true;
      break;

    case 94:
      // Raise Floor Crush
      if (EV_DoFloor(line,raiseFloorCrush))
	rc = true;
      break;

    case 95:
      // Raise floor to nearest height
      // and change texture.
      if (EV_DoPlat(line,raiseToNearestAndChange))
	rc = true;
      break;

    case 96:
      // Raise floor to shortest texture height
      // on either side of lines.
      if (EV_DoFloor(line,raiseToTexture))
	rc = true;
      break;

    case 97:
      // TELEPORT!
      if (EV_Teleport( line, side, thing ))
	rc = true;
      break;

    case 98:
      // Lower Floor (TURBO)
      if (EV_DoFloor(line,turboLower))
	rc = true;
      break;

    case 105:
      // Blazing Door Raise (faster than TURBO!)
      if(EV_DoDoor (line,blazeRaise))
	rc = true;
      break;

    case 106:
      // Blazing Door Open (faster than TURBO!)
      if (EV_DoDoor (line,blazeOpen))
	rc = true;
      break;

    case 107:
      // Blazing Door Close (faster than TURBO!)
      if (EV_DoDoor (line,blazeClose))
	rc = true;
      break;

    case 120:
      // Blazing PlatDownWaitUpStay.
      if (EV_DoPlat(line,blazeDWUS))
	rc = true;
      break;

    case 126:
      // TELEPORT MonsterONLY.
      if ((!thing->player)
       && EV_Teleport( line, side, thing ))
	rc = true;
      break;

    case 128:
      // Raise To Nearest Floor
      if (EV_DoFloor(line,raiseFloorToNearest))
	rc = true;
      break;

    case 129:
      // Raise Floor Turbo
      if (EV_DoFloor(line,raiseFloorTurbo))
	rc = true;
      break;

      /* Extended walk once triggers */

    case 142:
      /* Raise Floor 512 */
      /* 142 W1  EV_DoFloor(raiseFloor512) */
      if (EV_DoFloor(line,raiseFloor512))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 143:
      /* Raise Floor 24 and change */
      /* 143 W1  EV_DoPlat(raiseAndChange24) */
      if (EV_DoPlat(line,raiseAndChange24))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 144:
      /* Raise Floor 32 and change */
      /* 144 W1  EV_DoPlat(raiseAndChange32) */
      if (EV_DoPlat(line,raiseAndChange32))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 145:
      /* Lower Ceiling to Floor */
      /* 145 W1  EV_DoCeiling(lowerToFloor) */
      if (EV_DoCeiling( line, lowerToFloor ))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 146:
      /* Lower Pillar, Raise Donut */
      /* 146 W1  EV_DoDonut() */
      if (EV_DoDonut(line))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 199:
      /* Lower ceiling to lowest surrounding ceiling */
      /* 199 W1 EV_DoCeiling(lowerToLowest) */
      if (EV_DoCeiling(line,lowerToLowest))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 200:
      /* Lower ceiling to highest surrounding floor */
      /* 200 W1 EV_DoCeiling(lowerToMaxFloor) */
      if (EV_DoCeiling(line,lowerToMaxFloor))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 207:
      /* killough 2/16/98: W1 silent teleporter (normal kind) */
      if (EV_SilentTeleport(line, side, thing))
      {
	line->special = 0;
	rc = true;
      }
      break;

      /*jff 3/16/98 renumber 215->153 */
    case 153: /*jff 3/15/98 create texture change no motion type */
      /* Texture/Type Change Only (Trig) */
      /* 153 W1 Change Texture/Type Only */
      if (EV_DoChange(line,trigChangeOnly))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 239: /*jff 3/15/98 create texture change no motion type */
      /* Texture/Type Change Only (Numeric) */
      /* 239 W1 Change Texture/Type Only */
      if (EV_DoChange(line,numChangeOnly))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 219:
      /* Lower floor to next lower neighbor */
      /* 219 W1 Lower Floor Next Lower Neighbor */
      if (EV_DoFloor(line,lowerFloorToNearest))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 227:
      /* Raise elevator next floor */
      /* 227 W1 Raise Elevator next floor */
      if (EV_DoElevator(line,elevateUp))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 231:
      /* Lower elevator next floor */
      /* 231 W1 Lower Elevator next floor */
      if (EV_DoElevator(line,elevateDown))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 235:
      /* Elevator to current floor */
      /* 235 W1 Elevator to current floor */
      if (EV_DoElevator(line,elevateCurrent))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 243: /*jff 3/6/98 make fit within DCK's 256 linedef types */
      /* killough 2/16/98: W1 silent teleporter (linedef-linedef kind) */
      if (EV_SilentLineTeleport(line, side, thing, false))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 262: /*jff 4/14/98 add silent line-line reversed */
      if (EV_SilentLineTeleport(line, side, thing, true))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 264: /*jff 4/14/98 add monster-only silent line-line reversed */
      if (!thing->player
       && EV_SilentLineTeleport(line, side, thing, true))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 266: /*jff 4/14/98 add monster-only silent line-line */
      if (!thing->player &&
    	  EV_SilentLineTeleport(line, side, thing, false))
      {
	line->special = 0;
	rc = true;
      }
      break;

    case 268: /*jff 4/14/98 add monster-only silent */
      if (!thing->player && EV_SilentTeleport(line, side, thing))
      {
	line->special = 0;
	rc = true;
      }
      break;

      /*jff 1/29/98 end of added W1 linedef types */

      /* Extended walk many retriggerable */

      /*jff 1/29/98 added new linedef types to fill all functions */
      /*out so that all have varieties SR, S1, WR, W1 */

    case 147:
      /* Raise Floor 512 */
      /* 147 WR  EV_DoFloor(raiseFloor512) */
      if (EV_DoFloor(line,raiseFloor512))
	rc = true;
      break;

    case 148:
      /* Raise Floor 24 and Change */
      /* 148 WR  EV_DoPlat(raiseAndChange24) */
      if (EV_DoPlat(line,raiseAndChange24))
	rc = true;
      break;

    case 149:
      /* Raise Floor 32 and Change */
      /* 149 WR  EV_DoPlat(raiseAndChange32) */
      if (EV_DoPlat(line,raiseAndChange32))
	rc = true;
      break;

    case 150:
      /* Start slow silent crusher */
      /* 150 WR  EV_DoCeiling(silentCrushAndRaise) */
      if (EV_DoCeiling(line,silentCrushAndRaise))
	rc = true;
      break;

    case 151:
      /* RaiseCeilingLowerFloor */
      /* 151 WR  EV_DoCeiling(raiseToHighest), */
      /*	   EV_DoFloor(lowerFloortoLowest) */
#if 1
      {
	if (EV_DoCeiling (line, raiseToHighest))
	  rc = true;
      }
#else
      {
	int flags;

	flags = 0;				// Standard Doom cannot actually do two things
	if (EV_DoCeiling(line, raiseToHighest))	// at the same time with the same sector.
	  flags |= 1;
	if (EV_DoFloor( line, lowerFloorToLowest ))
	  flags |= 2;
	switch (flags)
	{
	  case 1:
	    line->special = 0x8B00+151;
	    rc = true;
	    break;
	  case 2:				// Shouldn't happen but allow for it!
	    line->special = 0x8A00+151;
	    rc = true;
	    break;
	}
      }
      break;

    case 0x8A00+151:				// Added by JAD to support function above
      if (EV_DoCeiling(line, raiseToHighest))	// May have to change number if clashes
      {
	line->special = 151;
	rc = true;
      }
      break;

    case 0x8B00+151:				// Added by JAD to support function above
      if (EV_DoFloor(line,lowerFloorToLowest))	// May have to change number if clashes
      {
	line->special = 151;
	rc = true;
      }
#endif
      break;

    case 152:
      /* Lower Ceiling to Floor */
      /* 152 WR  EV_DoCeiling(lowerToFloor) */
      if (EV_DoCeiling( line, lowerToFloor ))
	rc = true;
      break;

      /*jff 3/16/98 renumber 153->256 */
    case 256:
      /* Build stairs, step 8 */
      /* 256 WR EV_BuildStairs(build8) */
      if (EV_BuildStairs(line,stairnormalup8))
	rc = true;
      break;

      /*jff 3/16/98 renumber 154->257 */
    case 257:
      /* Build stairs, step 16 */
      /* 257 WR EV_BuildStairs(turbo16) */
      if (EV_BuildStairs(line,stairturboup16))
	rc = true;
      break;

    case 155:
      /* Lower Pillar, Raise Donut */
      /* 155 WR  EV_DoDonut() */
      if (EV_DoDonut(line))
	rc = true;
      break;

    case 156:
      /* Start lights strobing */
      /* 156 WR Lights EV_StartLightStrobing() */
      EV_StartLightStrobing(line);
      rc = true;
      break;

    case 157:
      /* Lights to dimmest near */
      /* 157 WR Lights EV_TurnTagLightsOff() */
      EV_TurnTagLightsOff(line);
      rc = true;
      break;

    case 201:
      /* Lower ceiling to lowest surrounding ceiling */
      /* 201 WR EV_DoCeiling(lowerToLowest) */
      if (EV_DoCeiling(line,lowerToLowest))
	rc = true;
      break;

    case 202:
      /* Lower ceiling to highest surrounding floor */
      /* 202 WR EV_DoCeiling(lowerToMaxFloor) */
      if (EV_DoCeiling(line,lowerToMaxFloor))
	rc = true;
      break;

    case 208:
      /* killough 2/16/98: WR silent teleporter (normal kind) */
      if (EV_SilentTeleport(line, side, thing))
	rc = true;
      break;

    case 212: /*jff 3/14/98 create instant toggle floor type */
      /* Toggle floor between C and F instantly */
      /* 212 WR Instant Toggle Floor */
      if (EV_DoPlat(line,toggleUpDn))
	rc = true;
      break;

      /*jff 3/16/98 renumber 216->154 */
    case 154: /*jff 3/15/98 create texture change no motion type */
      /* Texture/Type Change Only (Trigger) */
      /* 154 WR Change Texture/Type Only */
      if (EV_DoChange(line,trigChangeOnly))
	rc = true;
      break;

    case 240: /*jff 3/15/98 create texture change no motion type */
      /* Texture/Type Change Only (Numeric) */
      /* 240 WR Change Texture/Type Only */
      if (EV_DoChange(line,numChangeOnly))
	rc = true;
      break;

    case 220:
      /* Lower floor to next lower neighbor */
      /* 220 WR Lower Floor Next Lower Neighbor */
      if (EV_DoFloor(line,lowerFloorToNearest))
	rc = true;
      break;

    case 228:
      /* Raise elevator next floor */
      /* 228 WR Raise Elevator next floor */
      if (EV_DoElevator(line,elevateUp))
	rc = true;
      break;

    case 232:
      /* Lower elevator next floor */
      /* 232 WR Lower Elevator next floor */
      if (EV_DoElevator(line,elevateDown))
	rc = true;
      break;

    case 236:
      /* Elevator to current floor */
      /* 236 WR Elevator to current floor */
      if (EV_DoElevator(line,elevateCurrent))
	rc = true;
      break;

    case 244: /*jff 3/6/98 make fit within DCK's 256 linedef types */
      /* killough 2/16/98: WR silent teleporter (linedef-linedef kind) */
      if (EV_SilentLineTeleport(line, side, thing, false))
	rc = true;
      break;

    case 263: /*jff 4/14/98 add silent line-line reversed */
      if (EV_SilentLineTeleport(line, side, thing, true))
	rc = true;
      break;

    case 265: /*jff 4/14/98 add monster-only silent line-line reversed */
      if ((!thing->player)
       && EV_SilentLineTeleport(line, side, thing, true))
	rc = true;
      break;

    case 267: /*jff 4/14/98 add monster-only silent line-line */
      if ((!thing->player)
       && EV_SilentLineTeleport(line, side, thing, false))
	rc = true;
      break;

    case 269: /*jff 4/14/98 add monster-only silent */
      if ((!thing->player)
       && EV_SilentTeleport(line, side, thing))
	rc = true;
      break;

      /*jff 1/29/98 end of added WR linedef types */

    default:
      rc = P_BoomSpecialLine (thing, line, side, WalkedOver);
      //printf ("Line %u\n", line->special);
  }

  return (rc);
}

/* ---------------------------------------------------------------------------- */
//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
boolean
P_ShootSpecialLine
( mobj_t*	thing,
  line_t*	line )
{
  boolean rc;

  //	Impacts that other things can activate.
  if (!thing->player)
  {
      switch(line->special)
      {
	case 46:
	  // OPEN DOOR IMPACT
	  break;

	default:
	  rc = P_MonsterCanOperate (line -> special, Gunned);
	  if (rc == false)
	    return (rc);
      }
  }

  rc = false;

  switch(line->special)
  {
    case 24:
      // RAISE FLOOR
      if (EV_DoFloor(line,raiseFloor))
      {
	P_ChangeSwitchTexture(line,0);
	rc = true;
      }
      break;

    case 46:
      // OPEN DOOR
      // This is shown as G1 in some docs and GR in others.
      // Doom retro is G1, but the code below is GR....
      if (EV_DoDoor(line,normalOpen))
      {
	P_ChangeSwitchTexture(line,0);
	// We set the line again just in case...
	line -> special = GenDoorBase
			| (ODoor << DoorKindShift)
			| (SpeedNormal << DoorSpeedShift)
			| (1 << DoorDelayShift)
			| DoorMonster
			| GunMany;
	rc = true;
      }
      break;

    case 47:
      // RAISE FLOOR NEAR AND CHANGE
      if (EV_DoPlat(line,raiseToNearestAndChange))
      {
	P_ChangeSwitchTexture(line,0);
	rc = true;
      }
      break;

    case 197:
      /* Exit to next level */
      P_ChangeSwitchTexture(line,0);
      G_ExitLevellater();
      rc = true;
      break;

    case 198:
      /* Exit to secret level */
      P_ChangeSwitchTexture(line,0);
      G_SecretExitLevellater();
      rc = true;
      break;
      /*jff end addition of new gun linedefs */

    default:
      rc = P_BoomSpecialLine (thing, line, 0, Gunned);
  }

  return (rc);
}

/* ---------------------------------------------------------------------------- */
//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector (player_t* player, sector_t * sector, dushort_t special)
{
  int damage;

  // Falling, not all the way down yet?
  if (player->mo->z != sector->floorheight)
    return;

  // Has hitten ground.

  switch (special & 0x1F)
  {
    case 0:
      break;

    case 5:
      // HELLSLIME DAMAGE
      if (!player->powers[pw_ironfeet])
	  if (!(leveltime&0x1f))
	      P_DamageMobj (player->mo, NULL, NULL, 10);
      break;

    case 7:
      // NUKAGE DAMAGE
      if (!player->powers[pw_ironfeet])
	  if (!(leveltime&0x1f))
	      P_DamageMobj (player->mo, NULL, NULL, 5);
      break;

    case 16:
      // SUPER HELLSLIME DAMAGE
    case 4:
      // STROBE HURT
      if (!player->powers[pw_ironfeet]
	  || (P_Random()<5) )
      {
	  if (!(leveltime&0x1f))
	      P_DamageMobj (player->mo, NULL, NULL, 20);
      }
      break;

    case 9:
      // SECRET SECTOR
      player->secretcount++;
      sector->special = special & ~31;
      sector->oldspecial |= 9;
      if (player == &players[consoleplayer])
      {
	player->message = special_effects_messages [PS_FOUND_SECRET];
	S_StartSound (NULL, sfx_secret);
      }
      break;

    case 11:
      // EXIT SUPER DAMAGE! (for E1M8 finale)
      player->cheats &= ~CF_GODMODE;

      if ((!(leveltime&0x1f))
       && (player->health > 10))
      {
	damage = 20;
	if (player->health < 30)
	  damage = player->health - 10;
	P_DamageMobj (player->mo, NULL, NULL, damage);
      }

      if (player->health <= 10)
      {
	G_ExitLevel();
	sector->special = special & ~31;
      }
      break;

#if 0
    case 18:			// Appears to be another damage sector (COD level 6, sector 410)
      break;
#endif

    case 19:			// Healing sector (COD level 11)
      if (!(leveltime&0x1f))
	P_GiveBody (player, 1);
      break;

    default:
      if (M_CheckParm ("-showunknown"))
	printf ("P_PlayerInSpecialSector: unknown special %i\n", special);
      sector->special = special & ~31;
  }

  switch (sector->special & 0x60)
  {
    case 0x20: /* 2/5 damage per 31 ticks */
      if (!player->powers[pw_ironfeet])
	if (!(leveltime&0x1f))
	  P_DamageMobj (player->mo, NULL, NULL, 5);
      break;

    case 0x40: /* 5/10 damage per 31 ticks */
      if (!player->powers[pw_ironfeet])
	if (!(leveltime&0x1f))
	  P_DamageMobj (player->mo, NULL, NULL, 10);
      break;

    case 0x60: /* 10/20 damage per 31 ticks */
      if (!player->powers[pw_ironfeet]
	|| (P_Random()<5))
      {
	/* take damage even with suit */
	if (!(leveltime&0x1f))
	  P_DamageMobj (player->mo, NULL, NULL, 20);
      }
  }

  if (sector->special & SECRET_MASK)
  {
    player->secretcount++;
    sector->special &= ~SECRET_MASK;
    sector->oldspecial |= SECRET_MASK;
    if (player == &players[consoleplayer])
    {
      player->message = special_effects_messages [PS_FOUND_SECRET];
      S_StartSound (NULL, sfx_secret);
    }
  }

#if 0
  if (sector->special & FRICTION_MASK)	// Icy
  {
  }
#endif
#if 0
  if (sector->special & PUSH_MASK)	// Wind/current/pushers/pullers are enabled for this sector
  {					// Nova Map 27 has this bit set
  }
#endif

  if (sector->special & 0xFC00)		// Anything else
  {
    if (M_CheckParm ("-showunknown"))
      printf ("P_PlayerInSpecialSector: unknown special %i\n", sector->special);
    sector->special &= ~0xFC00;
  }
}

/* ---------------------------------------------------------------------------- */
//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//
boolean		levelTimer;
int		levelTimeCount;

void P_UpdateSpecials (void)
{
  anim_t*	anim;
  int		pic;
  int		i;


  //	LEVEL TIMER
  if (levelTimer == true)
  {
    levelTimeCount--;
    if (!levelTimeCount)
      G_ExitLevel();
  }

  //	ANIMATE FLATS AND TEXTURES GLOBALLY

  for (anim = animhead; anim != NULL ; anim = anim->next)
  {
    for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
    {
      pic = anim->basepic + ( (leveltime/anim->speed + i)%anim->numpics );
      if (anim->istexture)
	textures[i].translation = pic;
      else
	flatinfo[i].translation = pic;
    }
  }

  skycolumnoffset += skyscrolldelta;
}

/* ---------------------------------------------------------------------------- */
//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//

// Parses command line parameters.
void P_SpawnSpecials (void)
{
  line_t*	line;
  sector_t*	sector;
  int		i;

  // See if -TIMER needs to be used.
  levelTimer = false;

  i = M_CheckParm("-avg");
  if (i && deathmatch)
  {
    levelTimer = true;
    levelTimeCount = 20 * 60 * 35;
  }

  i = M_CheckParm("-timer");
  if (i && deathmatch)
  {
    int	time;
    time = atoi(myargv[i+1]) * 60 * 35;
    levelTimer = true;
    levelTimeCount = time;
  }

  if ((M_CheckParm ("-nobadfloors"))
   && (!netgame))
    P_MakeSuitPerm ();

  //	Init special SECTORs.
  for (sector=sectors,i=0 ; i<numsectors ; i++, sector++)
  {
    if (sector->special & SECRET_MASK)
    {
      totalsecret++;
      if (M_CheckParm("-showsecrets"))
	printf ("Secret sector number %d\n", i);
    }

    switch (sector->special & 0x1F)
    {
      case 0:
	break;

      case 1:
	// FLICKERING LIGHTS
	P_SpawnLightFlash (sector);
	break;

      case 2:
	// STROBE FAST
	P_SpawnStrobeFlash(sector,FASTDARK,0,1);
	break;

      case 3:
	// STROBE SLOW
	P_SpawnStrobeFlash(sector,SLOWDARK,0,1);
	break;

      case 4:
	// STROBE FAST/DEATH SLIME
	P_SpawnStrobeFlash(sector,FASTDARK,0,0);
	break;

      case 8:
	// GLOWING LIGHT
	P_SpawnGlowingLight(sector);
	break;

      case 9:
	// SECRET SECTOR
	totalsecret++;
	if (M_CheckParm("-showsecrets"))
	  printf ("Secret sector number %d\n", i);
	break;

      case 10:
	// DOOR CLOSE IN 30 SECONDS
	P_SpawnDoorCloseIn30 (sector);
	break;

      case 12:
	// SYNC STROBE SLOW
	P_SpawnStrobeFlash (sector, SLOWDARK, 1, 1);
	break;

      case 13:
	// SYNC STROBE FAST
	P_SpawnStrobeFlash (sector, FASTDARK, 1, 1);
	break;

      case 14:
	// DOOR RAISE IN 5 MINUTES
	P_SpawnDoorRaiseIn5Mins (sector, i);
	break;

      case 17:
	P_SpawnFireFlicker(sector);
	break;

      case 21:		// Light Phased
	P_SpawnLightPhased (sector);
	break;

      case 22:		// Light sequence start
//	  case 23:		// Light sequence special 1
//	  case 24:		// Light sequence special 2
	P_SpawnLightChase (sector);
	break;

#if 0
      case 26:		// Stairs special 1
      case 27:		// Stairs special 2
	break;
#endif
    }
  }

  P_InitTagLists();

  //if (!compatibility && !demo_compatibility)
  {
    P_SpawnScrollers();
    P_SpawnFriction();
    P_SpawnPushers();

    for (line=lines,i=0; i<numlines; i++,line++)
    {
      switch (line->special)
      {
	int s, sec;

	/* killough 3/7/98: */
	/* support for drawn heights coming from different sector */
	case 242:
	  sec = sides[*line->sidenum].sector-sectors;
	  for (s = -1; (s = P_FindSectorFromLineTag(line,s)) >= 0;)
	    sectors[s].heightsec = sec;
	  break;

	/* killough 3/16/98: Add support for setting */
	/* floor lighting independently (e.g. lava) */

	case 213:
	  sec = sides[*line->sidenum].sector-sectors;
	  for (s = -1; (s = P_FindSectorFromLineTag(line,s)) >= 0;)
	    sectors[s].floorlightsec = sec;
	  break;

	case 260:				// killough 4/11/98: translucent 2s textures
	  {
	    int lump;

	    if (line->tag == 0)
	    {
	      lump = W_CheckNumForName ("TRANMAP");
	      if (lump == -1)
	      {
		R_InitTranslucencyTables ();
		lump = numlumps;
	      }
	      line->tranlump = lump;		// affect this linedef only
	    }
	    else
	    {
	      int j, lumpnum;
	      byte* data;
	      mapsidedef_t* msd;

	      lumpnum = G_MapLump (gameepisode, gamemap) + ML_SIDEDEFS;
	      data = W_CacheLumpNum (lumpnum, PU_STATIC);

#ifdef PADDED_STRUCTS
	      data += (*line->sidenum * 30);
#else
	      data += (*line->sidenum * sizeof(mapsidedef_t));
#endif

	      msd = (mapsidedef_t *) data;

	      if ((R_CheckTextureNumForName (msd->midtexture) != -1)
	       || ((lump = W_CheckNumForName (msd->midtexture)) == -1)
	       || (W_LumpLength (lump) != 65536))
	      {
		lump = W_CheckNumForName ("TRANMAP");
		if (lump == -1)
		{
		  R_InitTranslucencyTables ();
		  lump = numlumps;
		}
	      }

	      for (j = 0; j < numlines; j++)
	      {
		if (lines[j].tag == line->tag)	// affect all matching linedefs
		{
		  lines[j].tranlump = lump;
		}
	      }

	      W_ReleaseLumpNum (lumpnum);
	    }
	  }
	  break;

	/* killough 4/11/98: Add support for setting */
	/* ceiling lighting independently */
	case 261:
	  sec = sides[*line->sidenum].sector-sectors;
	  for (s = -1; (s = P_FindSectorFromLineTag(line,s)) >= 0;)
	    sectors[s].ceilinglightsec = sec;
	  break;

	// killough 10/98:
	//
	// Support for sky textures being transferred from sidedefs.
	// Allows scrolling and other effects (but if scrolling is
	// used, then the same sector tag needs to be used for the
	// sky sector, the sky-transfer linedef, and the scroll-effect
	// linedef). Still requires user to use F_SKY1 for the floor
	// or ceiling texture, to distinguish floor and ceiling sky.

      case 271:   // Regular sky
      case 272:   // Same, only flipped
	/* Valiant.wad uses tag 0 to get everything! */
	/* So we cannot use my hacked version of P_FindSectorFromLineTag */
#if 0
	for (s = -1; (s = P_FindSectorFromLineTag(line,s)) >= 0;)
	  sectors[s].sky = i | PL_SKYFLAT;
#else
	for (sector=sectors,s=0 ; s<numsectors ; s++, sector++)
	{
	  if (line->tag == sector->tag)
	    sector->sky = i | PL_SKYFLAT;
	}
#endif
	break;
      }
    }
  }

  //	Init other misc stuff
  activeceilingshead = NULL;
  activeplatshead = NULL;

  // UNUSED: no horizonal sliders.
  //	P_InitSlidingDoorFrames();
}

/* ---------------------------------------------------------------------------- */

sector_t *P_FindModelFloorSector (fixed_t floordestheight,int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  //BOOMTRACEOUT("P_FindModelFloorSector")

  sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (sec->linecount<linecount?sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
      if (getSide(secnum,i,0)->sector-sectors == secnum)
	sec = getSector(secnum,i,1);
      else
	sec = getSector(secnum,i,0);

      if (sec->floorheight == floordestheight)
	return sec;
    }
  }
  return NULL;
}

/* ---------------------------------------------------------------------------- */

sector_t *P_FindModelCeilingSector (fixed_t ceilingdestheight,int secnum)
{
  int i;
  sector_t *sec=NULL;
  int linecount;

  //BOOMTRACEOUT("P_FindModelCeilingSector")

  sec = &sectors[secnum];
  linecount = sec->linecount;
  for (i = 0; i < (sec->linecount<linecount?sec->linecount : linecount); i++)
  {
    if ( twoSided(secnum, i) )
    {
      if (getSide(secnum,i,0)->sector-sectors == secnum)
	sec = getSector(secnum,i,1);
      else
	sec = getSector(secnum,i,0);

      if (sec->ceilingheight == ceilingdestheight)
	return sec;
    }
  }
  return NULL;
}

/* ---------------------------------------------------------------------------- */

void T_Scroll(scroll_t *s)
{
  fixed_t dx = s->dx, dy = s->dy;

  //BOOMTRACEOUT("T_Scroll")

  if (s->control != -1)
  {
    /* compute scroll amounts based on a sector's height changes */
    fixed_t height = sectors[s->control].floorheight +
      sectors[s->control].ceilingheight;
    fixed_t delta = height - s->last_height;
    s->last_height = height;
    dx = FixedMul(dx, delta);
    dy = FixedMul(dy, delta);
  }

  /* killough 3/14/98: Add acceleration */
  if (s->accel)
  {
    s->vdx = dx += s->vdx;
    s->vdy = dy += s->vdy;
  }

  if (!(dx | dy))		   /* no-op if both (x,y) offsets 0 */
    return;

  switch (s->type)
  {
    side_t *side;
    sector_t *sec;
    fixed_t height, waterheight;  /* killough 4/4/98: add waterheight */
    //msecnode_t *node;
    mobj_t *thing;

    case sc_side:		   /* killough 3/7/98: Scroll wall texture */
      side = sides + s->affectee;
      side->textureoffset += dx;
      side->rowoffset += dy;
      break;

    case sc_floor:		  /* killough 3/7/98: Scroll floor texture */
      sec = sectors + s->affectee;
      sec->floor_xoffs += dx;
      sec->floor_yoffs += dy;
      break;

    case sc_ceiling:	       /* killough 3/7/98: Scroll ceiling texture */
      sec = sectors + s->affectee;
      sec->ceiling_xoffs += dx;
      sec->ceiling_yoffs += dy;
      break;

    case sc_carry:
      sec = sectors + s->affectee;
      height = sec->floorheight;
      waterheight = sec->heightsec != -1 &&
      sectors[sec->heightsec].floorheight > height ?
      sectors[sec->heightsec].floorheight : MININT;

      {
	struct msecnode_s *node;
	for (node = sec->touching_thinglist; node; node = node->m_snext)
	{
	  if ((((thing=node->m_thing)->flags & MF_NOCLIP) == 0)
	   && ((thing->flags & MF_NOGRAVITY) == 0)
	   && ((thing->flags & MF_SLIDE) || (thing->z <= height) || (thing->z < waterheight)))
	  {
	    /* Move objects only if on floor or underwater, */
	    /* non-floating, and clipped. */
	    thing->momx += dx;
	    thing->momy += dy;
	    thing->flags |= MF_SLIDE;
	  }
	}
      }
      break;

    case sc_carry_ceiling:       /* to be added later */
      break;
  }
}

/* ---------------------------------------------------------------------------- */

static void Add_Scroller(int type, fixed_t dx, fixed_t dy,
			 int control, int affectee, int accel)
{
  scroll_t *s = Z_Malloc(sizeof *s, PU_LEVSPEC, 0);

  s->type = (scrolltype_e) type;
  s->dx = dx;
  s->dy = dy;
  s->accel = accel;
  s->vdx = s->vdy = 0;

  //BOOMTRACEOUT("Add_Scroller")

  if ((s->control = control) != -1)
  {
    s->last_height =
      sectors[control].floorheight + sectors[control].ceilingheight;
  }
  s->affectee = affectee;
  P_AddThinker (&s->thinker, (actionf_p1) T_Scroll);
}

/* ---------------------------------------------------------------------------- */

static void Add_WallScroller(fixed_t dx, fixed_t dy, const line_t *l, int control, int accel)
{
  fixed_t i;
  fixed_t x = abs(l->dx), y = abs(l->dy), d;

  //BOOMTRACEOUT("Add_WallScroller")

  if (y > x)
  {
    d = x; x = y; y = d;
  }


  if (x == 0)			// Avoid divide by zero
  {
    i = SLOPERANGE;
  }
  else
  {
    i = FixedDiv (y,x) >> DBITS;
    if (i > SLOPERANGE)
      i = SLOPERANGE;
  }

  d = FixedDiv(x, finesine[(tantoangle[i] + ANG90) >> ANGLETOFINESHIFT]);
  x = -FixedDiv(FixedMul(dy, l->dy) + FixedMul(dx, l->dx), d);
  y = -FixedDiv(FixedMul(dx, l->dy) - FixedMul(dy, l->dx), d);
  Add_Scroller(sc_side, x, y, control, *l->sidenum, accel);
}

/* ---------------------------------------------------------------------------- */
/* Amount (dx,dy) vector linedef is shifted right to get scroll amount */
#define SCROLL_SHIFT 5

/* Factor to scale scrolling effect into mobj-carrying properties = 3/32. */
/* (This is so scrolling floors and objects on them can move at same speed.) */
#define CARRYFACTOR ((fixed_t)(FRACUNIT*.09375))

/* Initialise the scrollers */
static void P_SpawnScrollers(void)
{
  int i;
  line_t *l = lines;

  //BOOMTRACEOUT("P_SpawnScrollers")

  for (i=0;i<numlines;i++,l++)
  {
    fixed_t dx = l->dx >> SCROLL_SHIFT;	/* direction and speed of scrolling */
    fixed_t dy = l->dy >> SCROLL_SHIFT;
    int control = -1, accel = 0;		/* no control sector or acceleration */
    int special = l->special;

    if (special >= 245 && special <= 249)	/* displacement scrollers (245,246,247,248,249) */
    {
      special += 250-245;
      control = sides[*l->sidenum].sector - sectors;
    }
    else if (special >= 214 && special <= 218)	/* accelerative scrollers (214,215,216,217,218) */
    {
      accel = 1;
      special += 250-214;
      control = sides[*l->sidenum].sector - sectors;
    }

    switch (special)
    {
      register int s;

      case 250:				/* scroll effect ceiling */
	for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
	  Add_Scroller(sc_ceiling, -dx, dy, control, s, accel);
	break;

      case 251:				/* scroll effect floor */
      case 253:				/* scroll and carry objects on floor */
	for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
	  Add_Scroller(sc_floor, -dx, dy, control, s, accel);
	if (special != 253)
	  break;

      case 252:				/* carry objects on floor */
	dx = FixedMul(dx,CARRYFACTOR);
	dy = FixedMul(dy,CARRYFACTOR);
	for (s=-1; (s = P_FindSectorFromLineTag(l,s)) >= 0;)
	  Add_Scroller(sc_carry, dx, dy, control, s, accel);
	break;

	/* killough 3/1/98: scroll wall according to linedef */
	/* (same direction and speed as scrolling floors) */
      case 254:
	for (s=-1; (s = P_FindLineFromTag(l->tag,s)) >= 0;)
	  if (s != i)
	    Add_WallScroller(dx, dy, lines+s, control, accel);
	break;

      case 255:				/* killough 3/2/98: scroll according to sidedef offsets */
	s = lines[i].sidenum[0];
	Add_Scroller(sc_side, -sides[s].textureoffset,
		     sides[s].rowoffset, -1, s, accel);
	break;

      case 48:				/* scroll first side */
	Add_Scroller(sc_side,  FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
	break;

      case 85:				/* jff 1/30/98 2-way scroll */
	Add_Scroller(sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
	break;
    }
  }
}

/* ---------------------------------------------------------------------------- */
//
// FRICTION EFFECTS
//
// phares 03/12/98: Start of friction effects
//
// As the player moves, friction is applied by decreasing the x and y
// momentum values on each tic. By varying the percentage of decrease,
// we can simulate muddy or icy conditions. In mud, the player slows
// down faster. In ice, the player slows down more slowly.
//
// The amount of friction change is controlled by the length of a linedef
// with type 223. A length < 100 gives you mud. A length > 100 gives you ice.
//
// Also, each sector where these effects are to take place is given a
// new special type _______. Changing the type value at runtime allows
// these effects to be turned on or off.
//
// Sector boundaries present problems. The player should experience these
// friction changes only when his feet are touching the sector floor. At
// sector boundaries where floor height changes, the player can find
// himself still 'in' one sector, but with his feet at the floor level
// of the next sector (steps up or down). To handle this, Thinkers are used
// in icy/muddy sectors. These thinkers examine each object that is touching
// their sectors, looking for players whose feet are at the same level as
// their floors. Players satisfying this condition are given new friction
// values that are applied by the player movement code later.
//
// killough 08/28/98:
//
// Completely redid code, which did not need thinkers, and which put a heavy
// drag on CPU. Friction is now a property of sectors, NOT objects inside
// them. All objects, not just players, are affected by it, if they touch
// the sector's floor. Code simpler and faster, only calling on friction
// calculations when an object needs friction considered, instead of doing
// friction calculations on every sector during every tic.
//
// Although this -might- ruin BOOM demo sync involving friction, it's the only
// way, short of code explosion, to fix the original design bug. Fixing the
// design bug in BOOM's original friction code, while maintaining demo sync
// under every conceivable circumstance, would double or triple code size, and
// would require maintenance of buggy legacy code which is only useful for old
// demos. DOOM demos, which are more important IMO, are not affected by this
// change.
//

//
// Initialize the sectors where friction is increased or decreased
static void P_SpawnFriction(void)
{
    line_t  *l = lines;

    for (int i = 0; i < numlines; i++, l++)
	if (l->special == 223)
	{
	    int length = P_ApproxDistance(l->dx, l->dy) >> FRACBITS;
	    int friction = BETWEEN(0, (0x1EB8 * length) / 0x80 + 0xD000, FRACUNIT);
	    int movefactor;

	    // The following check might seem odd. At the time of movement,
	    // the move distance is multiplied by 'friction/0x10000', so a
	    // higher friction value actually means 'less friction'.
	    if (friction > ORIG_FRICTION)       // ice
		movefactor = MAX(32, ((0x10092 - friction) * 0x70) / 0x0158);
	    else
		movefactor = MAX(32, ((friction - 0xDB34) * 0x0A) / 0x80);

	    for (int s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
	    {
		// killough 08/28/98:
		//
		// Instead of spawning thinkers, which are slow and expensive,
		// modify the sector's own friction values. Friction should be
		// a property of sectors, not objects which reside inside them.
		// Original code scanned every object in every friction sector
		// on every tic, adjusting its friction, putting unnecessary
		// drag on CPU. New code adjusts friction of sector only once
		// at level startup, and then uses this friction value.
		sectors[s].friction = friction;
		sectors[s].movefactor = movefactor;
	    }
	}
}

//
// PUSH/PULL EFFECT
//
// phares 03/20/98: Start of push/pull effects
//
// This is where push/pull effects are applied to objects in the sectors.
//
// There are four kinds of push effects
//
// 1) Pushing Away
//
//    Pushes you away from a point source defined by the location of an
//    MT_PUSH Thing. The force decreases linearly with distance from the
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the MT_PUSH. The force is felt only if the point
//    MT_PUSH can see the target object.
//
// 2) Pulling toward
//
//    Same as Pushing Away except you're pulled toward an MT_PULL point
//    source. This force crosses sector boundaries and is felt w/in a circle
//    whose center is at the MT_PULL. The force is felt only if the point
//    MT_PULL can see the target object.
//
// 3) Wind
//
//    Pushes you in a constant direction. Full force above ground, half
//    force on the ground, nothing if you're below it (water).
//
// 4) Current
//
//    Pushes you in a constant direction. No force above ground, full
//    force if on the ground or below it (water).
//
// The magnitude of the force is controlled by the length of a controlling
// linedef. The force vector for types 3 & 4 is determined by the angle
// of the linedef, and is constant.
//
// For each sector where these effects occur, the sector special type has
// to have the PUSH_MASK bit set. If this bit is turned off by a switch
// at run-time, the effect will not occur. The controlling sector for
// types 1 & 2 is the sector containing the MT_PUSH/MT_PULL Thing.

#define PUSH_FACTOR 7

//
// Add a push thinker to the thinker list
static void Add_Pusher(push_type_t type, int x_mag, int y_mag, mobj_t *source, int affectee)
{
    sector_t *sec;
    sec = &sectors[affectee];

    // Be sure the special sector type is still turned on. If so, proceed.
    // Else, bail out; the sector type has been changed on us.
    if (!(sec->special & PUSH_MASK))
	return;

    pusher_t *p = Z_Calloc (sizeof(*p), PU_LEVSPEC, NULL);

    p->source = source;
    p->type = type;
    p->x_mag = x_mag >> FRACBITS;
    p->y_mag = y_mag >> FRACBITS;
    p->magnitude = P_ApproxDistance(p->x_mag, p->y_mag);

    // point source exist?
    if (source)
    {
	p->radius = p->magnitude << (FRACBITS + 1);	// where force goes to zero
	p->x = p->source->x;
	p->y = p->source->y;
    }

    p->affectee = affectee;
    P_AddThinker (&p->thinker, (actionf_p1) T_Pusher);
}

//
// PIT_PushThing determines the angle and magnitude of the effect.
// The object's x and y momentum values are changed.
//
// tmpusher belongs to the point source (MT_PUSH/MT_PULL).
//
// killough 10/98: allow to affect things besides players

static pusher_t *tmpusher;  // pusher structure for blockmap searches

static boolean PIT_PushThing(mobj_t *thing)
{
    if ((sentient(thing) || (thing->flags & MF_SHOOTABLE)) && !(thing->flags & MF_NOCLIP))
    {
	fixed_t sx = tmpusher->x;
	fixed_t sy = tmpusher->y;
	fixed_t speed = (tmpusher->magnitude - ((P_ApproxDistance(thing->x - sx, thing->y - sy) >> FRACBITS) >> 1))
		    << (FRACBITS - PUSH_FACTOR - 1);

	// killough 10/98: make magnitude decrease with square
	// of distance, making it more in line with real nature,
	// so long as it's still in range with original formula.
	//
	// Removes angular distortion, and makes effort required
	// to stay close to source, grow increasingly hard as you
	// get closer, as expected. Still, it doesn't consider z :(
	if (speed > 0)
	{
	    int x = (thing->x - sx) >> FRACBITS;
	    int y = (thing->y - sy) >> FRACBITS;

	    speed = (fixed_t)(((int64_t)tmpusher->magnitude << 23) / ((int64_t)x * x + (int64_t)y * y + 1));
	}

	// If speed <= 0, you're outside the effective radius. You also have
	// to be able to see the push/pull source point.
	if (speed > 0 && P_CheckSight(thing, tmpusher->source))
	{
	    angle_t pushangle = R_PointToAngle2(thing->x, thing->y, sx, sy);

	    if (tmpusher->source->type == MT_PUSH)
		pushangle += ANG180;    // away

	    pushangle >>= ANGLETOFINESHIFT;
	    thing->momx += FixedMul(speed, finecosine[pushangle]);
	    thing->momy += FixedMul(speed, finesine[pushangle]);
	}
    }

    return true;
}

//
// T_Pusher looks for all objects that are inside the radius of
// the effect.
//
void T_Pusher(pusher_t *p)
{
    sector_t *sec;
    int xspeed, yspeed;
    int hs;
    int ht = 0;

    sec = &sectors [p->affectee];

    // Be sure the special sector type is still turned on. If so, proceed.
    // Else, bail out; the sector type has been changed on us.
    if (!(sec->special & PUSH_MASK))
	return;

    // For constant pushers (wind/current) there are 3 situations:
    //
    // 1) Affected Thing is above the floor.
    //
    //    Apply the full force if wind, no force if current.
    //
    // 2) Affected Thing is on the ground.
    //
    //    Apply half force if wind, full force if current.
    //
    // 3) Affected Thing is below the ground (underwater effect).
    //
    //    Apply no force if wind, full force if current.
    if (p->type == p_push)
    {
      if (p->source) 				// Just in case
      {
	// Seek out all pushable things within the force radius of this
	// point pusher. Crosses sectors, so use blockmap.
	int xl;
	int xh;
	int yl;
	int yh;
	int radius;

	tmpusher = p;				// MT_PUSH/MT_PULL point source
	radius = p->radius;			// where force goes to zero
	tmbbox[BOXTOP] = p->y + radius;
	tmbbox[BOXBOTTOM] = p->y - radius;
	tmbbox[BOXRIGHT] = p->x + radius;
	tmbbox[BOXLEFT] = p->x - radius;

	xl = P_GetSafeBlockX(tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS);
	xh = P_GetSafeBlockX(tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS);
	yl = P_GetSafeBlockY(tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS);
	yh = P_GetSafeBlockY(tmbbox[BOXTOP] - bmaporgy + MAXRADIUS);

	for (int bx = xl; bx <= xh; bx++)
	    for (int by = yl; by <= yh; by++)
		P_BlockThingsIterator(bx, by, &PIT_PushThing);
      }
      return;
    }

    // constant pushers p_wind and p_current
    if ((hs = sec->heightsec) != 0)		     // special water sector?
	ht = sectors[hs].floorheight;

    // things touching this sector
    for (msecnode_t *node = sec->touching_thinglist; node; node = node->m_snext)
    {
	mobj_t  *thing = node->m_thing;

	if (!thing->player || (thing->flags & (MF_NOGRAVITY | MF_NOCLIP)))
	    continue;

	if (p->type == p_wind)
	{
	    if (!sec->heightsec)		// NOT special water sector
	    {
		if (thing->z > thing->floorz)	// above ground
		{
		    xspeed = p->x_mag;		// full force
		    yspeed = p->y_mag;
		}
		else				// on ground
		{
		    xspeed = p->x_mag >> 1;	// half force
		    yspeed = p->y_mag >> 1;
		}
	    }
	    else				// special water sector
	    {
		if (thing->z > ht)		// above ground
		{
		    xspeed = p->x_mag;		// full force
		    yspeed = p->y_mag;
		}
		else
		{
		    if (thing->player->viewz < ht) // underwater
			xspeed = yspeed = 0;	   // no force
		    else			   // wading in water
		    {
			xspeed = p->x_mag >> 1;	// half force
			yspeed = p->y_mag >> 1;
		    }
		}
	    }
	}
	else					// p_current
	{
	    if (!sec->heightsec)		// NOT special water sector
	    {
		if (thing->z > sec->floorheight)// above ground
		{
		    xspeed = 0;			// no force
		    yspeed = 0;
		}
		else				// on ground
		{
		    xspeed = p->x_mag;		// full force
		    yspeed = p->y_mag;
		}
	    }
	    else				// special water sector
	    {
		if (thing->z > ht)		// above ground
		{
		    xspeed = 0;			// no force
		    yspeed = 0;
		}
		else				// underwater
		{
		    xspeed = p->x_mag;		// full force
		    yspeed = p->y_mag;
		}
	    }
	}
	thing->momx += xspeed << (FRACBITS - PUSH_FACTOR);
	thing->momy += yspeed << (FRACBITS - PUSH_FACTOR);
    }
}

// P_GetPushThing() returns a pointer to an MT_PUSH or MT_PULL thing, NULL otherwise.
mobj_t *P_GetPushThing(int s)
{
    sector_t    *sec = sectors + s;
    mobj_t      *thing = sec->thinglist;

    while (thing)
    {
	if (thing->type == MT_PUSH || thing->type == MT_PULL)
	    return thing;

	thing = thing->snext;
    }

    return NULL;
}

//
// Initialize the sectors where pushers are present
//
static void P_SpawnPushers(void)
{
    line_t  *l = lines;
    mobj_t  *thing;

    for (int i = 0; i < numlines; i++, l++)
	switch (l->special)
	{
	    case 224:
		for (int s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
		    Add_Pusher(p_wind, l->dx, l->dy, NULL, s);
		break;

	    case 225:
		for (int s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
		    Add_Pusher(p_current, l->dx, l->dy, NULL, s);
		break;

	    case 226:
		for (int s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
		    if ((thing = P_GetPushThing(s)) != NULL)    // No MT_P* means no effect
			Add_Pusher(p_push, l->dx, l->dy, thing, s);
		break;
	}
}
