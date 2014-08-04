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
//	Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_tick.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif

#include "includes.h"

// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
//#define PADSAVEP()	save_p += (4 - ((int) save_p & 3)) & 3
#define PADSAVEP()	save_p = (byte*)(((pint)save_p + 3) & ~3)

/* True when the file being loaded appears to have been saved on a
   machine with the opposite endian-ness (e.g. HP-UX PA-RISC -> ARM)
*/
static boolean wrong_endian;
extern int numflats;

//-----------------------------------------------------------------------------
#if 0
static void p_memcpy (void * dest, void * src, unsigned int size)
{
  unsigned int * dest_32;
  unsigned int * src_32;

  if (wrong_endian)
  {
    dest_32 = (unsigned int *) dest;
    src_32  = (unsigned int *) src;
    while (size > 3)
    {
      *dest_32++ = (unsigned int) SwapLONG (*src_32++);
      size -= 4;
    }
    if (size)
    {
      *dest_32 = (unsigned int) SwapLONG (*src_32);
    }
  }
  else
  {
    memcpy (dest, src, size);
  }
}
#endif
//-----------------------------------------------------------------------------

static unsigned int p_load_32 (unsigned int * ptr)
{
  unsigned int rc;

  rc = *ptr;
  if (wrong_endian)
    rc = (unsigned int) SwapLONG (rc);

  return (rc);
}

//-----------------------------------------------------------------------------

static short p_load_16 (byte * save_p)
{
  short*	get;
  unsigned short rc;

  get = (short *) save_p;

  rc = *get;
  if (wrong_endian)
    rc = SwapSHORT (rc);

  return (rc);
}

//-----------------------------------------------------------------------------

#ifdef ALWAYS_SAVE_LITTLE_ENDIAN

static void p_save_2x8 (unsigned char * ptr_c, unsigned int val)
{
  *ptr_c++ = val; val >>= 8;
  *ptr_c++ = val;
}

static void p_save_4x8 (unsigned char * ptr_c, unsigned int val)
{
  *ptr_c++ = val; val >>= 8;
  *ptr_c++ = val; val >>= 8;
  *ptr_c++ = val; val >>= 8;
  *ptr_c++ = val;
}

#define p_save_16(a)	p_save_2x8((unsigned char*)put,a)
#define p_save_32(a)	p_save_4x8((unsigned char*)save_32_p,a)
#else
#define p_save_16(a)	*put++=(a)
#define p_save_32(a)	*save_32_p++=(a)
#endif

//-----------------------------------------------------------------------------

static sector_t * P_GetSectorPtr (unsigned int * save_32_p)
{
  unsigned int sectornum;

  sectornum = p_load_32 (save_32_p);
  if (sectornum >= numsectors)
    sectornum = 0;
  return (&sectors[sectornum]);
}

//-----------------------------------------------------------------------------

static byte * P_ArchivePlayer (byte * save_p, player_t * ply)
{
  unsigned int i;
  unsigned int t32;
  unsigned int * save_32_p;

  PADSAVEP();
  save_32_p = (unsigned int *) save_p;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (ply -> playerstate);

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!

  p_save_32 (ply -> viewz);
  p_save_32 (ply -> viewheight);
  p_save_32 (ply -> deltaviewheight);
  p_save_32 (ply -> bob);
  p_save_32 (ply -> health);
  p_save_32 (ply -> armourpoints);
  p_save_32 (ply -> armourtype);

  i = 0;
  do
  {
    p_save_32 (ply -> powers [i]);
  } while (++i < ARRAY_SIZE (ply -> powers));

  i = 0;
  do
  {
    p_save_32 ((unsigned int) ply -> cards [i]);
  } while (++i < ARRAY_SIZE (ply -> cards));

  p_save_32 ((unsigned int) ply -> backpack);

  i = 0;
  do
  {
    p_save_32 (ply -> frags [i]);
  } while (++i < ARRAY_SIZE (ply -> frags));

  p_save_32 ((unsigned int) ply -> readyweapon);
  p_save_32 ((unsigned int) ply -> pendingweapon);

  i = 0;
  do
  {
    p_save_32 ((unsigned int) ply -> weaponowned [i]);
  } while (++i < ARRAY_SIZE (ply -> weaponowned));

  i = 0;
  do
  {
    p_save_32 (ply -> ammo [i]);
  } while (++i < ARRAY_SIZE (ply -> ammo));

  i = 0;
  do
  {
    p_save_32 (ply -> maxammo [i]);
  } while (++i < ARRAY_SIZE (ply -> maxammo));

  p_save_32 (ply -> attackdown);
  p_save_32 (ply -> usedown);
  p_save_32 (ply -> cheats);
  p_save_32 (ply -> refire);
  p_save_32 (ply -> killcount);
  p_save_32 (ply -> itemcount);
  p_save_32 (ply -> secretcount);

  p_save_32 (0);					// SPARE!!!

  p_save_32 (ply -> damagecount);
  p_save_32 (ply -> bonuscount);

  p_save_32 (0);					// SPARE!!!

  p_save_32 (ply -> extralight);
  p_save_32 (ply -> fixedcolormap);
  p_save_32 (ply -> colormap);

  i = 0;
  do
  {
    t32 = 0;
    if (ply -> psprites[i].state)
      t32 = ply->psprites[i].state-states;
    p_save_32 (t32);
    p_save_32 (ply -> psprites[i].tics);
    p_save_32 (ply -> psprites[i].sx);
    p_save_32 (ply -> psprites[i].sy);
  } while (++i < ARRAY_SIZE (ply->psprites));

  p_save_32 ((unsigned int) ply -> didsecret);

#ifdef VERBOSE_LOAD
  printf ("P_ArchivePlayer\n");
  printf ("%d %d\n", sizeof (player_t), (byte *) save_32_p - save_p);
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchivePlayer (player_t * ply, byte * save_p)
{
  unsigned int i;
  unsigned int t32;
  unsigned int * save_32_p;

  PADSAVEP();

  memset (ply, 0, sizeof (player_t));

  save_32_p = (unsigned int *) save_p;
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  ply -> playerstate = (playerstate_t) p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  ply -> viewz = p_load_32 (save_32_p); save_32_p++;
  ply -> viewheight = p_load_32 (save_32_p); save_32_p++;
  ply -> deltaviewheight = p_load_32 (save_32_p); save_32_p++;
  ply -> bob = p_load_32 (save_32_p); save_32_p++;
  ply -> health = p_load_32 (save_32_p); save_32_p++;
  ply -> armourpoints = p_load_32 (save_32_p); save_32_p++;
  ply -> armourtype = p_load_32 (save_32_p); save_32_p++;

  i = 0;
  do
  {
    ply -> powers [i] = p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> powers));

  i = 0;
  do
  {
    ply -> cards [i] = (boolean) p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> cards));

  ply -> backpack = (boolean) p_load_32 (save_32_p); save_32_p++;

  i = 0;
  do
  {
    ply -> frags [i] = p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> frags));

  ply -> readyweapon = (weapontype_t) p_load_32 (save_32_p); save_32_p++;
  ply -> pendingweapon = (weapontype_t) p_load_32 (save_32_p); save_32_p++;

  i = 0;
  do
  {
    ply -> weaponowned [i] = (boolean) p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> weaponowned));

  i = 0;
  do
  {
    ply -> ammo [i] = p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> ammo));

  i = 0;
  do
  {
    ply -> maxammo [i] = p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply -> maxammo));

  ply -> attackdown = p_load_32 (save_32_p); save_32_p++;
  ply -> usedown = p_load_32 (save_32_p); save_32_p++;
  ply -> cheats = p_load_32 (save_32_p); save_32_p++;
  ply -> refire = p_load_32 (save_32_p); save_32_p++;
  ply -> killcount = p_load_32 (save_32_p); save_32_p++;
  ply -> itemcount = p_load_32 (save_32_p); save_32_p++;
  ply -> secretcount = p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  ply -> damagecount = p_load_32 (save_32_p); save_32_p++;
  ply -> bonuscount = p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  ply -> extralight = p_load_32 (save_32_p); save_32_p++;
  ply -> fixedcolormap = p_load_32 (save_32_p); save_32_p++;
  ply -> colormap = p_load_32 (save_32_p); save_32_p++;

  i = 0;
  do
  {
    t32 = p_load_32 (save_32_p); save_32_p++;
    if (t32)
      ply -> psprites[i].state = &states[t32];
    ply -> psprites[i].tics = p_load_32 (save_32_p); save_32_p++;
    ply -> psprites[i].sx = p_load_32 (save_32_p); save_32_p++;
    ply -> psprites[i].sy = p_load_32 (save_32_p); save_32_p++;
  } while (++i < ARRAY_SIZE (ply->psprites));

  ply -> didsecret = (boolean) p_load_32 (save_32_p); save_32_p++;

#ifdef VERBOSE_LOAD
  printf ("P_UnArchivePlayer\n");
  printf ("%d %d\n", sizeof (player_t), (byte *) save_32_p - save_p);
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------
//
// P_ArchivePlayers
//
byte * P_ArchivePlayers (byte * save_p)
{
  int		i;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i])
      save_p = P_ArchivePlayer (save_p, &players[i]);
  }
  return (save_p);
}

//-----------------------------------------------------------------------------
//
// P_UnArchivePlayers
//
byte * P_UnArchivePlayers (byte * save_p)
{
  int		i;
  player_t *  ply;
  byte * new_save_p;

  wrong_endian = false;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (!playeringame[i])
	continue;

    ply = &players[i];
    new_save_p = P_UnArchivePlayer (ply, save_p);

    /* If at least one value looks way too large then assume
       that this file has been saved on a machine with an
       opposite endian-ness
    */

    if ((wrong_endian == false)
     && ((ply -> health	> 0xFF0000)
      || (ply -> armourpoints	> 0xFF0000)
      || (ply -> armourtype	> 0xFF0000)
      || (ply -> killcount	> 0xFF0000)
      || (ply -> itemcount	> 0xFF0000)
      || (ply -> secretcount	> 0xFF0000)))
    {
      wrong_endian = true;
#ifdef VERBOSE_LOAD
      printf ("Wrong Endian\n");
#endif
      new_save_p = P_UnArchivePlayer (&players[i], save_p);
    }

    save_p = new_save_p;
  }
  return (save_p);
}

//-----------------------------------------------------------------------------
//
// P_ArchiveWorld
//
byte * P_ArchiveWorld (byte * save_p)
{
  int			i;
  int			j;
  sector_t*		sec;
  line_t*		li;
  side_t*		si;
  short*		put;

  put = (short *)save_p;

  // do sectors
  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
  {
    p_save_16 (sec->floorheight >> FRACBITS);
    p_save_16 (sec->ceilingheight >> FRACBITS);
    p_save_16 (sec->floorpic);
    p_save_16 (sec->ceilingpic);
    p_save_16 (sec->lightlevel);
    p_save_16 (sec->special);		// needed?
    p_save_16 (sec->tag);		// needed?
  }


  // do lines
  for (i=0, li = lines ; i<numlines ; i++,li++)
  {
    p_save_16 (li->flags);
    p_save_16 (li->special);
    p_save_16 (li->tag);
    for (j=0 ; j<2 ; j++)
    {
      if (li->sidenum[j] == (dushort_t) -1)
	continue;

      si = &sides[li->sidenum[j]];

      p_save_16 (si->textureoffset >> FRACBITS);
      p_save_16 (si->rowoffset >> FRACBITS);
      p_save_16 (si->toptexture);
      p_save_16 (si->bottomtexture);
      p_save_16 (si->midtexture);
    }
  }

  return ((byte *)put);
}

//-----------------------------------------------------------------------------
//
// P_UnArchiveWorld
//
byte * P_UnArchiveWorld (byte * save_p)
{
  int			i;
  int			j;
  sector_t*		sec;
  line_t*		li;
  side_t*		si;

  // do sectors
  for (i=0, sec = sectors ; i<numsectors ; i++,sec++)
  {
    sec->floorheight = p_load_16(save_p) << FRACBITS; save_p += 2;
    sec->ceilingheight = p_load_16(save_p) << FRACBITS; save_p += 2;
    sec->floorpic = p_load_16(save_p); save_p += 2;
    sec->ceilingpic = p_load_16(save_p); save_p += 2;
    sec->lightlevel = p_load_16(save_p); save_p += 2;
    sec->special = p_load_16(save_p); save_p += 2;		// needed?
    sec->tag = p_load_16(save_p); save_p += 2;			// needed?
#if 0
    sec->specialdata = 0;
#else
    sec->floordata = 0;
    sec->lightingdata = 0;
    sec->ceilingdata = 0;
#endif
    sec->soundtarget = 0;
  }

  // do lines
  for (i=0, li = lines ; i<numlines ; i++,li++)
  {
    li->flags = p_load_16(save_p); save_p += 2;
    li->special = p_load_16(save_p); save_p += 2;
    li->tag = p_load_16(save_p); save_p += 2;
    for (j=0 ; j<2 ; j++)
    {
      if (li->sidenum[j] == (dushort_t) -1)
	  continue;
      si = &sides[li->sidenum[j]];
      si->textureoffset = p_load_16(save_p) << FRACBITS; save_p += 2;
      si->rowoffset = p_load_16(save_p) << FRACBITS; save_p += 2;
      si->toptexture = p_load_16(save_p); save_p += 2;
      si->bottomtexture = p_load_16(save_p); save_p += 2;
      si->midtexture = p_load_16(save_p); save_p += 2;
    }
  }
  return (save_p);
}

//-----------------------------------------------------------------------------
//
// Thinkers
//
typedef enum
{
  tc_end,
  tc_mobj
} thinkerclass_t;

//-----------------------------------------------------------------------------

static byte * P_ArchiveMobj (byte * save_p, mobj_t* mobj)
{
  unsigned int t32;
  unsigned int thinker;
  unsigned int * save_32_p;
  unsigned short* put;

  *save_p++ = tc_mobj;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (mobj->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);

  p_save_32 (mobj->x);
  p_save_32 (mobj->y);
  p_save_32 (mobj->z);

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!

  p_save_32 ((unsigned int) mobj->angle);
  p_save_32 ((unsigned int) mobj->sprite);
  p_save_32 (mobj->frame);

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!

  p_save_32 (mobj->floorz);
  p_save_32 (mobj->ceilingz);
  p_save_32 (mobj->radius);
  p_save_32 (mobj->height);
  p_save_32 (mobj->momx);
  p_save_32 (mobj->momy);
  p_save_32 (mobj->momz);
  p_save_32 (mobj->validcount);
  p_save_32 (mobj->type);

  p_save_32 (0);					// SPARE!!!

  p_save_32 (mobj->tics);
  p_save_32 (mobj->state - states);
  p_save_32 (mobj->flags);
  p_save_32 (mobj->health);
  p_save_32 (mobj->movedir);
  p_save_32 (mobj->movecount);
  p_save_32 (mobj->flags2);
  p_save_32 (mobj->reactiontime);
  p_save_32 (mobj->threshold);

  t32 = 0;
  if (mobj->player) t32 = ((mobj->player-players) + 1);
  p_save_32 (t32);

  p_save_32 (mobj->lastlook);

  put = (unsigned short *) save_32_p;			// 16 bit
  p_save_16 (mobj->spawnpoint.x);
  p_save_16 (mobj->spawnpoint.y);
  p_save_16 (mobj->spawnpoint.angle);
  p_save_16 (mobj->spawnpoint.type);
  p_save_16 (mobj->spawnpoint.options);
  p_save_16 (0);					// SPARE!!!
  save_32_p = (unsigned int *) put;

  p_save_32 (0);					// SPARE!!!

  // printf ("P_ArchiveMobj %u %u\n", sizeof (mobj_t), (byte *) save_32_p - save_p);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveMobj (byte * save_p)
{
  mobj_t* mobj;
  unsigned int t32;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  mobj = Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
  memset (mobj, 0, sizeof (*mobj));

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  if (thinker)
    P_AddThinker (&mobj->thinker, (actionf_p1)P_MobjThinker);
  else
    P_AddThinker (&mobj->thinker, NULL);

  mobj->x = p_load_32 (save_32_p); save_32_p++;
  mobj->y = p_load_32 (save_32_p); save_32_p++;
  mobj->z = p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  mobj->angle = (angle_t) p_load_32 (save_32_p); save_32_p++;
  mobj->sprite = (spritenum_t) p_load_32 (save_32_p); save_32_p++;
  mobj->frame = p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  mobj->floorz = p_load_32 (save_32_p); save_32_p++;
  mobj->ceilingz = p_load_32 (save_32_p); save_32_p++;
  mobj->radius = p_load_32 (save_32_p); save_32_p++;
  mobj->height = p_load_32 (save_32_p); save_32_p++;
  mobj->momx = p_load_32 (save_32_p); save_32_p++;
  mobj->momy = p_load_32 (save_32_p); save_32_p++;
  mobj->momz = p_load_32 (save_32_p); save_32_p++;
  mobj->validcount = p_load_32 (save_32_p); save_32_p++;
  mobj->type = (mobjtype_t) p_load_32 (save_32_p); save_32_p++;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  mobj->tics = p_load_32 (save_32_p); save_32_p++;
  t32 = p_load_32 (save_32_p); save_32_p++;
  mobj->state = &states[t32];
  mobj->flags = p_load_32 (save_32_p); save_32_p++;
  mobj->health = p_load_32 (save_32_p); save_32_p++;

  mobj->movedir = p_load_32 (save_32_p); save_32_p++;
  mobj->movecount = p_load_32 (save_32_p); save_32_p++;
  mobj->flags2 = p_load_32 (save_32_p); save_32_p++;
  mobj->reactiontime = p_load_32 (save_32_p); save_32_p++;
  mobj->threshold = p_load_32 (save_32_p); save_32_p++;

  t32 = p_load_32 (save_32_p); save_32_p++;
  if (t32)
  {
    mobj->player = &players[t32-1];
    mobj->player->mo = mobj;
  }

  mobj->lastlook = p_load_32 (save_32_p); save_32_p++;


  save_p = (byte *) save_32_p;
  mobj->spawnpoint.x = p_load_16 (save_p); save_p += 2;
  mobj->spawnpoint.y = p_load_16 (save_p); save_p += 2;
  mobj->spawnpoint.angle = p_load_16 (save_p); save_p += 2;
  mobj->spawnpoint.type = p_load_16 (save_p); save_p += 2;
  mobj->spawnpoint.options = p_load_16 (save_p); save_p += 2;
  /* p_load_16 (save_p); */ save_p += 2;
  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!

  P_SetThingPosition (mobj);
  mobj->info = &mobjinfo[mobj->type];
  mobj->floorz = mobj->subsector->sector->floorheight;
  mobj->ceilingz = mobj->subsector->sector->ceilingheight;

#ifdef VERBOSE_LOAD
  printf ("P_UnarchiveMobj (%d)\n", ((byte *) save_32_p) - save_p);
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------
//
// P_ArchiveThinkers
//
byte * P_ArchiveThinkers (byte * save_p)
{
  thinker_t*		th;

  // save off the current thinkers
  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    if (th->function.acp1 == (actionf_p1)P_MobjThinker)
    {
      save_p = P_ArchiveMobj (save_p, (mobj_t*) th);
      continue;
    }

    // I_Error ("P_ArchiveThinkers: Unknown thinker function");
  }

  // add a terminating marker
  *save_p++ = tc_end;
  return (save_p);
}

//-----------------------------------------------------------------------------
//
// P_UnArchiveThinkers
//
byte * P_UnArchiveThinkers (byte * save_p)
{
  byte		tclass;
  mobj_t*	mobj;
  thinker_t*	next;
  thinker_t*	currentthinker;

  // remove all the current thinkers
  currentthinker = thinker_head;
  while (currentthinker != NULL)
  {
    next = currentthinker->next;

    if (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
    {
      mobj = (mobj_t *)currentthinker;
      mobj->flags &= ~MF_SPECIAL;	// Ensure does not get added to the respawn queue
      P_RemoveMobj (mobj);
    }

    Z_Free (currentthinker);
    currentthinker = next;
  }

  P_InitThinkers ();

  // read in saved thinkers
  while (1)
  {
      tclass = *save_p++;
      switch (tclass)
      {
	case tc_end:
#ifdef VERBOSE_LOAD
	  printf ("P_UnArchiveThinkers\n");
	  printf ("%d\n", sizeof (*mobj));
#endif
	  return (save_p); 	// end of list

	case tc_mobj:
	  save_p = P_UnArchiveMobj (save_p);
	  break;

	default:
	  I_Error ("Unknown tclass %i in savegame",tclass);
      }
  }
}

//-----------------------------------------------------------------------------
//
// P_ArchiveSpecials
//
enum
{
  tc_ceiling,
  tc_door,
  tc_floor,
  tc_plat,
  tc_flash,
  tc_strobe,
  tc_glow,
  tc_endspecials,
  tc_scroll,
  tc_elevator,
  tc_fireflicker,
  tc_button
} specials_e;

//-----------------------------------------------------------------------------

static byte * P_ArchiveCeiling (byte * save_p, ceiling_t* ceiling)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_ceiling;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (ceiling->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (ceiling->secspecial - sectors);
  p_save_32 (thinker);
  p_save_32 ((unsigned int) ceiling->type);
  p_save_32 (ceiling->sector - sectors);
  p_save_32 ((unsigned int) ceiling->bottomheight);
  p_save_32 ((unsigned int) ceiling->topheight);
  p_save_32 ((unsigned int) ceiling->speed);
  p_save_32 ((unsigned int) ceiling->crush);
  p_save_32 ((unsigned int) ceiling->direction);
  p_save_32 ((unsigned int) ceiling->tag);
  p_save_32 ((unsigned int) 0 /*ceiling->olddirection*/);// Spare
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveCeiling (byte * save_p)
{
  ceiling_t* ceiling;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  ceiling->secspecial = P_GetSectorPtr (save_32_p); save_32_p++;
  thinker = p_load_32 (save_32_p); save_32_p++;
  ceiling->type = (ceiling_e) p_load_32 (save_32_p); save_32_p++;
  ceiling->sector = P_GetSectorPtr (save_32_p); save_32_p++;
  ceiling->sector->ceilingdata = ceiling;
  ceiling->bottomheight = p_load_32 (save_32_p); save_32_p++;
  ceiling->topheight = p_load_32 (save_32_p); save_32_p++;
  ceiling->speed = p_load_32 (save_32_p); save_32_p++;
  ceiling->crush = (boolean) p_load_32 (save_32_p); save_32_p++;
  ceiling->direction = p_load_32 (save_32_p); save_32_p++;
  ceiling->tag = p_load_32 (save_32_p); save_32_p++;
  /*ceiling->olddirection = p_load_32 (save_32_p); */ save_32_p++;	// Spare

  if (ceiling->direction == 0)			// Old savegame file?
    ceiling->direction = 1;			// Just make something up

  if (thinker)
    P_AddThinker (&ceiling->thinker, (actionf_p1)T_MoveCeiling);
  else
    P_AddThinker (&ceiling->thinker, NULL);

  P_AddActiveCeiling (ceiling);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveCeiling\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveDoor (byte * save_p, vldoor_t* door)
{
  unsigned int linenum;
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_door;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;
  if (door->thinker.function.acp1)
    thinker = 1;

  linenum = 0;
  if (door->line)
    linenum = (door->line - lines) + 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (linenum);
  p_save_32 (thinker);
  p_save_32 ((unsigned int) door->type);
  p_save_32 (door->sector - sectors);
  p_save_32 (door->topheight);
  p_save_32 (door->speed);
  p_save_32 (door->direction);
  p_save_32 (door->topwait);
  p_save_32 (door->topcountdown);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveDoor (byte * save_p)
{
  vldoor_t* door;
  unsigned int linenum;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  door = Z_Malloc (sizeof(*door), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  linenum = p_load_32 (save_32_p); save_32_p++;
  thinker = p_load_32 (save_32_p); save_32_p++;
  door -> type = (vldoor_e) p_load_32 (save_32_p); save_32_p++;
  door -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  door -> sector->ceilingdata = door;
  door -> topheight = p_load_32 (save_32_p); save_32_p++;
  door -> speed = p_load_32 (save_32_p); save_32_p++;
  door -> direction = p_load_32 (save_32_p); save_32_p++;
  door -> topwait = p_load_32 (save_32_p); save_32_p++;
  door -> topcountdown = p_load_32 (save_32_p); save_32_p++;

  if ((linenum == 0) || (linenum >= numlines))
    door -> line = NULL;
  else
    door -> line = &lines [linenum-1];

  if (thinker)
    P_AddThinker (&door->thinker, (actionf_p1)T_VerticalDoor);
  else
    P_AddThinker (&door->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveDoor\n");
#if 0
  printf ("type = %u\n", door->type);
  printf ("sector = %u\n", door->sector-sectors);
  printf ("topheight = %d\n", door->topheight>>FRACBITS);
  printf ("speed = %u\n", door->speed>>FRACBITS);
  printf ("direction = %d\n", door->direction);
  printf ("topwait = %u\n", door->topwait);
  printf ("topcountdown = %u\n", door->topcountdown);
  printf ("ceiling = %d\n", door->sector->ceilingheight>>FRACBITS);
  printf ("floor = %d\n", door->sector->floorheight>>FRACBITS);
#endif
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveFloor (byte * save_p, floormove_t* floor)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_floor;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (floor->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
//p_save_32 ((unsigned int) floor -> type);
  p_save_32 (0);					// SPARE!!!
  p_save_32 ((unsigned int) floor -> crush);
  p_save_32 (floor -> sector - sectors);
  p_save_32 (floor -> direction);
  p_save_32 (floor -> newspecial);
  p_save_32 (floor -> newtexture);
  p_save_32 (floor -> floordestheight);
  p_save_32 (floor -> speed);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveFloor (byte * save_p)
{
  floormove_t* floor;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
//floor -> type = (floor_e) p_load_32 (save_32_p); save_32_p++;
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  floor -> crush = (boolean) p_load_32 (save_32_p); save_32_p++;
  floor -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  floor -> sector->floordata = floor;
  floor -> direction = p_load_32 (save_32_p); save_32_p++;
  floor -> newspecial = p_load_32 (save_32_p); save_32_p++;
  floor -> newtexture = p_load_32 (save_32_p); save_32_p++;
  floor -> floordestheight = p_load_32 (save_32_p); save_32_p++;
  floor -> speed = p_load_32 (save_32_p); save_32_p++;

  if (floor -> newtexture >= numflats)			// Old games did not always save this!
  {
    floor -> newtexture = floor -> sector -> floorpic;	// so bodge it.
    floor -> newspecial = floor -> sector -> special;
  }

  if (thinker)
    P_AddThinker (&floor->thinker, (actionf_p1)T_MoveFloor);
  else
    P_AddThinker (&floor->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveFloor\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchivePlat (byte * save_p, plat_t* plat)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_plat;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (plat->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (plat->sector - sectors);
  p_save_32 (plat->speed);
  p_save_32 (plat->low);
  p_save_32 (plat->high);
  p_save_32 (plat->wait);
  p_save_32 (plat->count);
  p_save_32 ((unsigned int) plat->status);
  p_save_32 ((unsigned int) plat->oldstatus);
  p_save_32 ((unsigned int) plat->crush);
  p_save_32 (plat->tag);
  p_save_32 ((unsigned int) plat->type);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchivePlat (byte * save_p)
{
  plat_t* plat;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  plat = Z_Malloc (sizeof(*plat), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  plat -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  plat -> sector->floordata = plat;
  plat -> speed = p_load_32 (save_32_p); save_32_p++;
  plat -> low = p_load_32 (save_32_p); save_32_p++;
  plat -> high = p_load_32 (save_32_p); save_32_p++;
  plat -> wait = p_load_32 (save_32_p); save_32_p++;
  plat -> count = p_load_32 (save_32_p); save_32_p++;
  plat -> status = (plat_e) p_load_32 (save_32_p); save_32_p++;
  plat -> oldstatus = (plat_e) p_load_32 (save_32_p); save_32_p++;
  plat -> crush = (boolean) p_load_32 (save_32_p); save_32_p++;
  plat -> tag = p_load_32 (save_32_p); save_32_p++;
  plat -> type = (plattype_e) p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&plat->thinker, (actionf_p1)T_PlatRaise);
  else
    P_AddThinker (&plat->thinker, NULL);

  P_AddActivePlat (plat);
#ifdef VERBOSE_LOAD
  printf ("P_UnArchivePlat\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveLightFlash (byte * save_p, lightflash_t* flash)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_flash;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (flash->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (flash->sector - sectors);
  p_save_32 (flash->count);
  p_save_32 (flash->maxlight);
  p_save_32 (flash->minlight);
  p_save_32 (flash->maxtime);
  p_save_32 (flash->mintime);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveLightFlash (byte * save_p)
{
  lightflash_t* flash;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  flash = Z_Malloc (sizeof(*flash), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  flash -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  flash -> sector->lightingdata = flash;
  flash -> count = p_load_32 (save_32_p); save_32_p++;
  flash -> maxlight = p_load_32 (save_32_p); save_32_p++;
  flash -> minlight = p_load_32 (save_32_p); save_32_p++;
  flash -> maxtime = p_load_32 (save_32_p); save_32_p++;
  flash -> mintime = p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&flash->thinker, (actionf_p1)T_LightFlash);
  else
    P_AddThinker (&flash->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveLightFlash\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveStrobeFlash (byte * save_p, strobe_t* strobe)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_strobe;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (strobe->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (strobe->sector - sectors);
  p_save_32 (strobe->count);
  p_save_32 (strobe->minlight);
  p_save_32 (strobe->maxlight);
  p_save_32 (strobe->darktime);
  p_save_32 (strobe->brighttime);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveStrobeFlash (byte * save_p)
{
  strobe_t* strobe;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  strobe = Z_Malloc (sizeof(*strobe), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  strobe -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  strobe -> sector->lightingdata = strobe;
  strobe -> count = p_load_32 (save_32_p); save_32_p++;
  strobe -> minlight = p_load_32 (save_32_p); save_32_p++;
  strobe -> maxlight = p_load_32 (save_32_p); save_32_p++;
  strobe -> darktime = p_load_32 (save_32_p); save_32_p++;
  strobe -> brighttime = p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&strobe->thinker, (actionf_p1)T_StrobeFlash);
  else
    P_AddThinker (&strobe->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveStrobeFlash\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveGlow (byte * save_p, glow_t* glow)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_glow;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (glow->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (glow->sector - sectors);
  p_save_32 (glow->minlight);
  p_save_32 (glow->maxlight);
  p_save_32 (glow->direction);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveGlow (byte * save_p)
{
  glow_t* glow;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  glow = Z_Malloc (sizeof(*glow), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  glow -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  glow -> sector->lightingdata = glow;
  glow -> minlight = p_load_32 (save_32_p); save_32_p++;
  glow -> maxlight = p_load_32 (save_32_p); save_32_p++;
  glow -> direction = p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&glow->thinker, (actionf_p1)T_Glow);
  else
    P_AddThinker (&glow->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveGlow\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveScroll (byte * save_p, scroll_t* scroll)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_scroll;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (scroll->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (scroll->dx);
  p_save_32 (scroll->dy);
  p_save_32 (scroll->affectee);
  p_save_32 (scroll->control);
  p_save_32 (scroll->last_height);
  p_save_32 (scroll->vdx);
  p_save_32 (scroll->vdy);
  p_save_32 (scroll->accel);
  p_save_32 ((unsigned int) scroll->type);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveScroll (byte * save_p)
{
  scroll_t* scroll;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  scroll = Z_Malloc (sizeof(*scroll), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  scroll->dx = p_load_32 (save_32_p); save_32_p++;
  scroll->dy = p_load_32 (save_32_p); save_32_p++;
  scroll->affectee = p_load_32 (save_32_p); save_32_p++;
  scroll->control = p_load_32 (save_32_p); save_32_p++;
  scroll->last_height = p_load_32 (save_32_p); save_32_p++;
  scroll->vdx = p_load_32 (save_32_p); save_32_p++;
  scroll->vdy = p_load_32 (save_32_p); save_32_p++;
  scroll->accel = p_load_32 (save_32_p); save_32_p++;
  scroll->type = (scrolltype_e) p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&scroll->thinker, (actionf_p1)T_Scroll);
  else
    P_AddThinker (&scroll->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveScroll\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveElevator (byte * save_p, elevator_t* elevator)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_elevator;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (elevator->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 ((unsigned int) elevator->type);
  p_save_32 (elevator->sector - sectors);
  p_save_32 (elevator->direction);
  p_save_32 (elevator->floordestheight);
  p_save_32 (elevator->ceilingdestheight);
  p_save_32 (elevator->speed);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveElevator (byte * save_p)
{
  elevator_t* elevator;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  elevator = Z_Malloc (sizeof(*elevator), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  elevator -> type = (elevator_e) p_load_32 (save_32_p); save_32_p++;
  elevator -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  elevator -> sector->floordata = elevator; /*jff 2/22/98 */
  elevator -> sector->ceilingdata = elevator; /*jff 2/22/98 */
  elevator -> direction = p_load_32 (save_32_p); save_32_p++;
  elevator -> floordestheight = p_load_32 (save_32_p); save_32_p++;
  elevator -> ceilingdestheight = p_load_32 (save_32_p); save_32_p++;
  elevator -> speed = p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&elevator->thinker, (actionf_p1)T_MoveElevator);
  else
    P_AddThinker (&elevator->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveElevator\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveFireFlicker (byte * save_p, fireflicker_t* flick)
{
  unsigned int thinker;
  unsigned int * save_32_p;

  *save_p++ = tc_fireflicker;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  thinker = 0;

  if (flick->thinker.function.acp1)
    thinker = 1;

  p_save_32 (0);					// SPARE!!!
  p_save_32 (0);					// SPARE!!!
  p_save_32 (thinker);
  p_save_32 (flick->sector - sectors);
  p_save_32 (flick->count);
  p_save_32 (flick->maxlight);
  p_save_32 (flick->minlight);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveFireFlicker (byte * save_p)
{
  fireflicker_t* flick;
  unsigned int thinker;
  unsigned int * save_32_p;

  PADSAVEP();
  flick = Z_Malloc (sizeof(*flick), PU_LEVSPEC, NULL);

  save_32_p = (unsigned int *) save_p;

  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  /* p_load_32 (save_32_p); */ save_32_p++;		// SPARE!!!
  thinker = p_load_32 (save_32_p); save_32_p++;
  flick -> sector = P_GetSectorPtr (save_32_p); save_32_p++;
  flick -> sector->lightingdata = flick;
  flick -> count = p_load_32 (save_32_p); save_32_p++;
  flick -> maxlight = p_load_32 (save_32_p); save_32_p++;
  flick -> minlight = p_load_32 (save_32_p); save_32_p++;

  if (thinker)
    P_AddThinker (&flick->thinker, (actionf_p1)T_FireFlicker);
  else
    P_AddThinker (&flick->thinker, NULL);

#ifdef VERBOSE_LOAD
  printf ("P_UnArchiveFireFlicker\n");
#endif
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_ArchiveButton (byte * save_p, button_t * button)
{
  unsigned int * save_32_p;

  *save_p++ = tc_button;
  PADSAVEP();

  save_32_p = (unsigned int *) save_p;

  p_save_32 (button->line - lines);
  p_save_32 ((int)button->where);
  p_save_32 (button->btexture);
  p_save_32 (button->btimer);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------

static byte * P_UnArchiveButton (byte * save_p)
{
  int t32;
  unsigned int * save_32_p;
  button_t button;

  PADSAVEP();
  save_32_p = (unsigned int *) save_p;

  t32 = p_load_32 (save_32_p); save_32_p++;
  button.line = &lines [t32];
  button.where = (bwhere_e) p_load_32 (save_32_p); save_32_p++;
  button.btexture = p_load_32 (save_32_p); save_32_p++;
  button.btimer = p_load_32 (save_32_p); save_32_p++;

  P_StartButton (button.line, button.where, button.btexture, button.btimer);
  return ((byte *) save_32_p);
}

//-----------------------------------------------------------------------------
//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
byte * P_ArchiveSpecials (byte * save_p)
{
  int		done_one;
  thinker_t*	th;
  plat_t *	plat;
  ceiling_t *	ceiling;

  // save off the current thinkers
  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    if (th->function.acv == (actionf_v)NULL)
    {
      done_one = 0;

      for (ceiling = activeceilingshead ; ceiling != NULL ; ceiling=ceiling->next)
      {
	if (ceiling == (ceiling_t *)th)
	{
	  save_p = P_ArchiveCeiling (save_p, (ceiling_t *) th);
	  done_one = 1;
	  break;
	}
      }

      for (plat = activeplatshead ; plat != NULL ; plat=plat->next)
      {
	if (plat == (plat_t*) th)
	{
	  save_p = P_ArchivePlat (save_p, (plat_t*) th);
	  done_one = 1;
	  break;
	}
      }

      if (done_one)
	continue;
    }

    if (th->function.acp1 == (actionf_p1)T_MoveCeiling)
    {
      save_p = P_ArchiveCeiling (save_p, (ceiling_t *) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_VerticalDoor)
    {
      save_p = P_ArchiveDoor (save_p, (vldoor_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_MoveFloor)
    {
      save_p = P_ArchiveFloor (save_p, (floormove_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_PlatRaise)
    {
      save_p = P_ArchivePlat (save_p, (plat_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_LightFlash)
    {
      save_p = P_ArchiveLightFlash (save_p, (lightflash_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_StrobeFlash)
    {
      save_p = P_ArchiveStrobeFlash (save_p, (strobe_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_Glow)
    {
      save_p = P_ArchiveGlow (save_p, (glow_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_Scroll)
    {
      save_p = P_ArchiveScroll (save_p, (scroll_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_MoveElevator)
    {
      save_p = P_ArchiveElevator (save_p, (elevator_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_FireFlicker)
    {
      save_p = P_ArchiveFireFlicker (save_p, (fireflicker_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)T_Button)
    {
      save_p = P_ArchiveButton (save_p, (button_t*) th);
      continue;
    }

    if (th->function.acp1 == (actionf_p1)P_MobjThinker)	// Done already
      continue;

    if (th->function.aci == -1)				// Idle....
      continue;

    if (M_CheckParm ("-showunknown"))
      I_Error ("P_ArchiveSpecials: Unknown thinker %X\n", th->function.aci);
  }

  // add a terminating marker
  *save_p++ = tc_endspecials;
  return (save_p);
}


//-----------------------------------------------------------------------------
//
// P_UnArchiveSpecials
//
byte * P_UnArchiveSpecials (byte * save_p)
{
  byte tclass;


  // read in saved thinkers
  while (1)
  {
    tclass = *save_p++;
    switch (tclass)
    {
      case tc_endspecials:
	return (save_p);	// end of list

      case tc_ceiling:
	save_p = P_UnArchiveCeiling (save_p);
	break;

      case tc_door:
	save_p = P_UnArchiveDoor (save_p);
	break;

      case tc_floor:
	save_p = P_UnArchiveFloor (save_p);
	break;

      case tc_plat:
	save_p = P_UnArchivePlat (save_p);
	break;

      case tc_flash:
	save_p = P_UnArchiveLightFlash (save_p);
	break;

      case tc_strobe:
	save_p = P_UnArchiveStrobeFlash (save_p);
	break;

      case tc_glow:
	save_p = P_UnArchiveGlow (save_p);
	break;

      case tc_scroll:
	save_p = P_UnArchiveScroll (save_p);
	break;

      case tc_elevator:
	save_p = P_UnArchiveElevator (save_p);
	break;

      case tc_fireflicker:
	save_p = P_UnArchiveFireFlicker (save_p);
	break;

      case tc_button:
	save_p = P_UnArchiveButton (save_p);
	break;

      default:
	{
	  unsigned int size;		// Assume that later versions of the game
	  size = *save_p++;		// have put the size here....
	  if ((size & 3) == 0)	// Assume structures are word aligned.
	  {
	    save_p += size;
	    break;
	  }
	}

	I_Error ("P_UnarchiveSpecials:Unknown tclass %i "
	   "in savegame",tclass);
    }
  }
}

//-----------------------------------------------------------------------------
