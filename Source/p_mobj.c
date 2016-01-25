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
//	Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_mobj.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"

extern void G_PlayerReborn (int player);

#ifdef USE_BOOM_P_ChangeSector
extern void P_DelSeclist (msecnode_t *node);
#endif

unsigned int mtf_mask = 0;

/* -------------------------------------------------------------------------------------------- */
/*
   Experimental code to read the speed from the mobjinfo table
   and apply the nightmare adjustments so that the mobjinfo
   table doesn't have to be written to in G_InitNew().
*/
#if 0
int P_GetMobjSpeed (mobj_t* mobj)
{
  int speed;

  speed = mobj->info->speed;

  if (gameskill == sk_nightmare)
  {
    switch (mobj->type)
    {
      case MT_BRUISERSHOT:
	speed = (speed * 3) / 4;	// 20 -> 15
	break;

      case MT_HEADSHOT:
      case MT_TROOPSHOT:
	speed >>= 1;
    }
  }

  return (speed);
}
#endif
/* -------------------------------------------------------------------------------------------- */
//
// P_SetMobjState
// Returns true if the mobj is still present.
//

boolean
P_SetMobjState
( mobj_t*	mobj,
  statenum_t	state )
{
  int tics;
  unsigned int guard;
  state_t* st;

  guard = 100;			// Assume that if we go round too many
				// times then we have corruption!
  do
  {
    if ((--guard == 0) || (state == S_NULL) || (state >= NUMSTATES))
    {
      mobj->state = (state_t *) S_NULL;
      P_RemoveMobj (mobj);
      return false;
    }

    st = &states[state];
    mobj->state = st;
    tics = (int) st->tics;
#if 0
    if ((tics > 1)
     && (gameskill == sk_nightmare)
     && (state >= S_SARG_RUN1)
     && (state <= S_SARG_PAIN2))
      tics >>= 1;
#endif
    mobj->tics = tics;
    mobj->sprite = st->sprite;
    mobj->frame = (int) st->frame;

    // Modified handling.
    // Call action functions when the state is set
    if (st->action.acp2)
      st->action.acp2(mobj,NULL);

    state = st->nextstate;
  } while (!mobj->tics);

  return true;
}

/* -------------------------------------------------------------------------------------------- */
//
// P_ExplodeMissile
//
void P_ExplodeMissile (mobj_t* mo)
{
  mo->momx = mo->momy = mo->momz = 0;

  if (P_SetMobjState (mo, (statenum_t) mobjinfo[mo->type].deathstate))
  {
    mo->tics -= P_Random()&3;

    if (mo->tics < 1)
	mo->tics = 1;

    mo->flags &= ~MF_MISSILE;

    if (mo->info->deathsound)
	S_StartSound (mo, mo->info->deathsound);
  }
}

/* -------------------------------------------------------------------------------------------- */
//
// P_XYMovement
//

void P_XYMovement (mobj_t* mo)
{
    int		flags;
    fixed_t 	ptryx;
    fixed_t	ptryy;
    player_t*	player;
    fixed_t	xmove;
    fixed_t	ymove;
    fixed_t	friction;
    fixed_t	stopspeed;
    sector_t *	sector;

    flags = mo->flags;

    if ((mo->momx | mo->momy) == 0)
    {
	mo->flags = flags & ~MF_SLIDE;
	if (flags & MF_SKULLFLY)
	{
	    // the skull slammed into something
	    mo->flags = flags & ~MF_SKULLFLY;
	    mo->momx = mo->momy = mo->momz = 0;

	    P_SetMobjState (mo, (statenum_t) mo->info->spawnstate);
	}
	return;
    }

    player = mo->player;

    if (mo->momx > MAXMOVE)
	mo->momx = MAXMOVE;
    else if (mo->momx < -MAXMOVE)
	mo->momx = -MAXMOVE;

    if (mo->momy > MAXMOVE)
	mo->momy = MAXMOVE;
    else if (mo->momy < -MAXMOVE)
	mo->momy = -MAXMOVE;

    xmove = mo->momx;
    ymove = mo->momy;

    do
    {
	if (xmove > MAXMOVE/2 || ymove > MAXMOVE/2)
	{
	    xmove >>= 1;
	    ymove >>= 1;
	    ptryx = mo->x + xmove;
	    ptryy = mo->y + ymove;
	}
	else
	{
	    ptryx = mo->x + xmove;
	    ptryy = mo->y + ymove;
	    xmove = ymove = 0;
	}

	if (!P_TryMove (mo, ptryx, ptryy))
	{
	    // blocked move
	    if ((flags & (MF_MISSILE|MF_BOUNCES)) == MF_MISSILE)
	    {
		// explode a missile
		if (ceilingline &&
		    ceilingline->backsector &&
		    ceilingline->backsector->ceilingpic == skyflatnum)
		{
		    // Hack to prevent missiles exploding
		    // against the sky.
		    // Does not handle sky floors.
		    P_RemoveMobj (mo);
		    return;
		}
		P_ExplodeMissile (mo);
	    }
	    else if ((mo->player) || (flags & MF_SLIDE))
	    {	// try to slide along it
		P_SlideMove (mo);
	    }
	    else
		mo->momx = mo->momy = 0;
	}
    } while (xmove || ymove);

    // slow down
    if (player && player->cheats & CF_NOMOMENTUM)
    {
	// debug option for no sliding at all
	mo->momx = mo->momy = 0;
	return;
    }

    if ((flags & (MF_MISSILE | MF_SKULLFLY) )	// no friction for missiles ever
     || ((mo->z > mo->floorz)			// no friction when airborne
      && !(mo->flags2 & MF2_ONMOBJ)))
    {
	return;
    }

    if (flags & MF_CORPSE)
    {
	// do not stop sliding
	//  if halfway off a step with some momentum
	if (mo->momx > FRACUNIT/4
	    || mo->momx < -FRACUNIT/4
	    || mo->momy > FRACUNIT/4
	    || mo->momy < -FRACUNIT/4)
	{
//	    if (mo->floorz > mo->subsector->sector->floorheight)
	    if (mo->z > mo->subsector->sector->floorheight)
		return;
	}
    }

    stopspeed = STOPSPEED;
    sector = mo->subsector->sector;

    /* Allow things being carried to fall off the edge. */
    if (flags & MF_SLIDE)
    {
      /* If the sector that we are above is a scrolling one */
      /* then we shouldn't exit here. So the FRICTION is */
      /* applied in T_Scroll() instead. */

      if (mo->floorz > sector->floorheight)
	return;

      /* Cod.wad level 10 throws some very slow moving stuff off */
      /* the end of a conveyor over a drop and it has to slide */
      /* onto a teleport line. */
      stopspeed = 1;
    }

    if (mo->momx > -stopspeed
	&& mo->momx < stopspeed
	&& mo->momy > -stopspeed
	&& mo->momy < stopspeed
	&& (!player
	    || (player->cmd.forwardmove== 0
		&& player->cmd.sidemove == 0 ) ) )
    {
	// if in a walking frame, stop moving
	if ( player&&(unsigned)((player->mo->state - states)- S_PLAY_RUN1) < 4)
	    P_SetMobjState (player->mo, S_PLAY);

	mo->momx = 0;
	mo->momy = 0;
	return;
    }

    friction = FRICTION;

    /* Are we in an icy sector? */
    if (sector -> special & 0x100)
      friction = 0xF000;

    mo->momx = FixedMul (mo->momx, friction);
    mo->momy = FixedMul (mo->momy, friction);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_ZMovement
//
void P_ZMovement (mobj_t* mo)
{
    fixed_t	dist;
    fixed_t	delta;

    // check for smooth step up
    if (mo->player && mo->z < mo->floorz)
    {
	mo->player->viewheight -= mo->floorz-mo->z;

	mo->player->deltaviewheight
	    = (VIEWHEIGHT - mo->player->viewheight)>>3;
    }

    // adjust height
    mo->z += mo->momz;

    if ( mo->flags & MF_FLOAT
	 && mo->target)
    {
	// float down towards target if too close
	if ( !(mo->flags & MF_SKULLFLY)
	     && !(mo->flags & MF_INFLOAT) )
	{
	    dist = P_ApproxDistance (mo->x - mo->target->x,
				    mo->y - mo->target->y);

	    delta =(mo->target->z + (mo->height>>1)) - mo->z;

	    if (delta<0 && dist < -(delta*3) )
		mo->z -= FLOATSPEED;
	    else if (delta>0 && dist < (delta*3) )
		mo->z += FLOATSPEED;
	}

    }

    // clip movement
    if (mo->z <= mo->floorz)
    {
	// hit the floor

	// Note (id):
	//  somebody left this after the setting momz to 0,
	//  kinda useless there.
	if (mo->flags & MF_SKULLFLY)
	{
	    // the skull slammed into something
	    mo->momz = -mo->momz;
	}

	if (mo->momz < 0)
	{
	    if (mo->player
		&& mo->momz < -GRAVITY*8)
	    {
		// Squat down.
		// Decrease viewheight for a moment
		// after hitting the ground (hard),
		// and utter appropriate sound.
		mo->player->deltaviewheight = mo->momz>>3;
		S_StartSound (mo, sfx_oof);
	    }
	    mo->momz = 0;
	}
	mo->z = mo->floorz;

	if ((mo->flags & (MF_MISSILE|MF_NOCLIP)) == MF_MISSILE)
	{
	  if (mo->flags & MF_BOUNCES)
	  {
	    mo -> momz = 0;
	    mo -> momx = 0;
	    mo -> momy = 0;
	  }
	  else
	  {
	    P_ExplodeMissile (mo);
	    return;
	  }
	}
    }
    else if (! (mo->flags & MF_NOGRAVITY) )
    {
	if (mo->momz == 0)
	    mo->momz = -GRAVITY*2;
	else
	    mo->momz -= GRAVITY;
    }

    if (mo->z + mo->height > mo->ceilingz)
    {
	// hit the ceiling
	if (mo->momz > 0)
	    mo->momz = 0;
	{
	    mo->z = mo->ceilingz - mo->height;
	}

	if (mo->flags & MF_SKULLFLY)
	{	// the skull slammed into something
	    mo->momz = -mo->momz;
	}

	if ((mo->flags & (MF_MISSILE|MF_NOCLIP)) == MF_MISSILE)
	{
	  if (mo->flags & MF_BOUNCES)
	  {
	    mo -> momz = 0;
	    mo -> momx = 0;
	    mo -> momy = 0;
	  }
	  else
	  {
	    P_ExplodeMissile (mo);
	  }
	}
    }
}

/* -------------------------------------------------------------------------------------------- */

static void PlayerLandedOnThing (mobj_t *mo, mobj_t *onmobj)
{
    mo->player->deltaviewheight = mo->momz >> 3;
    if (mo->momz < -23 * FRACUNIT)
	P_NoiseAlert (mo, mo);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_NightmareRespawn
//
void
P_NightmareRespawn (mobj_t* mobj)
{
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;
    subsector_t*	ss;
    mobj_t*		mo;
    mapthing_t*		mthing;

    x = mobj->spawnpoint.x << FRACBITS;
    y = mobj->spawnpoint.y << FRACBITS;


   /* haleyjd: stupid nightmare respawning bug fix
    *
    * This fixes the notorious nightmare respawning bug that causes monsters
    * that didn't spawn at level startup to respawn at the point (0,0)
    * regardless of that point's nature. SMMU and Eternity need this for
    * script-spawned things like Halif Swordsmythe, as well.
    */
    if ((x == 0) && (y == 0))
    {
      // spawnpoint was zeroed out, so use point of death instead
      x = mobj->x;
      y = mobj->y;
    }

    // somthing is occupying it's position?
    if (!P_CheckPosition (mobj, x, y) )
	return;	// no respwan

    // spawn a teleport fog at old spot
    // because of removal of the body?
    mo = P_SpawnMobj (mobj->x,
		      mobj->y,
		      mobj->subsector->sector->floorheight , MT_TFOG);
    // initiate teleport sound
    S_StartSound (mo, sfx_telept);

    if ((x != mobj->x) || (y != mobj->y))
    {
      // spawn a teleport fog at the new spot
      ss = R_PointInSubsector (x,y);
      mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_TFOG);
      S_StartSound (mo, sfx_telept);
    }

    // spawn the new monster
    mthing = &mobj->spawnpoint;

    // spawn it
    if (mobj->info->flags & MF_SPAWNCEILING)
	z = ONCEILINGZ;
    else
	z = ONFLOORZ;

    // inherit attributes from deceased one
    mo = P_SpawnMobj (x,y,z, mobj->type);
    mo->spawnpoint = mobj->spawnpoint;
    mo->angle = ANG45 * (mthing->angle/45);

    if (mthing->options & MTF_AMBUSH)
	mo->flags |= MF_AMBUSH;

    // killough 11/98: transfer friendliness from deceased
    mo->flags = (mo->flags & ~MF_FRIEND) | (mobj->flags & MF_FRIEND);

    mo->reactiontime = 18;

    // remove the old monster,
    P_RemoveMobj (mobj);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_MobjThinker
//
void P_MobjThinker (mobj_t* mobj)
{
    // momentum movement
    if (mobj->momx
	|| mobj->momy
	|| (mobj->flags&MF_SKULLFLY) )
    {
	P_XYMovement (mobj);

	// FIXME: decent NOP/NULL/Nil function pointer please.
	if (mobj->thinker.function.aci == -1)
	    return;		// mobj was removed
    }
    if ( (mobj->z != mobj->floorz)
	 || mobj->momz )
    {
	if (mobj->flags2 & MF2_PASSMOBJ)
	{
	    mobj_t *onmo;

	    onmo = P_CheckOnmobj(mobj);
	    if (onmo == NULL)
	    {
		P_ZMovement(mobj);
		mobj->flags2 &= ~MF2_ONMOBJ;
	    }
	    else
	    {
		if (mobj->player)
		{
		    if (mobj->momz < -GRAVITY * 8)
			PlayerLandedOnThing(mobj, onmo);
		    if (onmo->z + onmo->height - mobj->z <= 24 * FRACUNIT)
		    {
			mobj->player->viewheight -= onmo->z + onmo->height - mobj->z;
			mobj->player->deltaviewheight = (VIEWHEIGHT - mobj->player->viewheight) >> 3;
			mobj->z = onmo->z + onmo->height;
			mobj->flags2 |= MF2_ONMOBJ;
			mobj->momz = 0;
		    }
		    else
			// hit the bottom of the blocking mobj
			mobj->momz = 0;
		}
	    }
	}
	else
	{
	   P_ZMovement (mobj);
	}

	// FIXME: decent NOP/NULL/Nil function pointer please.
	if (mobj->thinker.function.aci == -1)
	    return;		// mobj was removed
    }


    // cycle through states,
    // calling action functions at transitions
    if (mobj->tics != -1)
    {
	mobj->tics--;

	// you can cycle through multiple states in a tic
	if (!mobj->tics)
	    if (!P_SetMobjState (mobj, mobj->state->nextstate) )
		return;		// freed itself
    }
    else
    {
	// check for nightmare respawn
	if (! (mobj->flags & MF_COUNTKILL) )
	    return;

	if (!respawnmonsters)
	    return;

	mobj->movecount++;

	if (mobj->movecount < 12*35)
	    return;

	if ( leveltime&31 )
	    return;

	if (P_Random () > 4)
	    return;

	P_NightmareRespawn (mobj);
    }

}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnMobj
//
mobj_t*
P_SpawnMobj
( fixed_t	x,
  fixed_t	y,
  fixed_t	z,
  mobjtype_t	type )
{
    int		tics;
    mobj_t*	mobj;
    state_t*	st;
    mobjinfo_t*	info;

    mobj = Z_Calloc (sizeof(*mobj), PU_LEVEL, NULL);
    info = &mobjinfo[type];

    mobj->type = type;
    mobj->info = info;
    mobj->x = x;
    mobj->y = y;
    mobj->radius = info->radius;
    mobj->height = info->height;
    mobj->flags  = info->flags;

    if (type == MT_PLAYER)         // Players
      mobj->flags |= MF_FRIEND;    // are always friends.

    mobj->flags2 = info->flags2;
    mobj->health = info->spawnhealth;

    if (gameskill != sk_nightmare)
	mobj->reactiontime = info->reactiontime;

    mobj->lastlook = P_Random () % MAXPLAYERS;
    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet
    st = &states[info->spawnstate];

    mobj->state = st;

    tics = (int) st->tics;
#if 0
    if ((tics > 1)
     && (gameskill == sk_nightmare)
     && (info->spawnstate >= S_SARG_RUN1)
     && (info->spawnstate <= S_SARG_PAIN2))
      tics >>= 1;
#endif
    mobj->tics = tics;

    mobj->sprite = st->sprite;
    mobj->frame = (int) st->frame;

    // set subsector and/or block links
    P_SetThingPosition (mobj);

    mobj->floorz = mobj->subsector->sector->floorheight;
    mobj->ceilingz = mobj->subsector->sector->ceilingheight;

    if (z == ONFLOORZ)
	mobj->z = mobj->floorz;
    else if (z == ONCEILINGZ)
	mobj->z = mobj->ceilingz - mobj->info->height;
    else
	mobj->z = z;

    P_AddThinker (&mobj->thinker, (actionf_p1)P_MobjThinker);

    return mobj;
}

/* -------------------------------------------------------------------------------------------- */
//
// P_RemoveMobj
//
mapthing_t	itemrespawnque[ITEMQUESIZE];
int		itemrespawntime[ITEMQUESIZE];
int		iquehead;
int		iquetail;


void P_RemoveMobj (mobj_t* mobj)
{
    if ((mobj->flags & MF_SPECIAL)
	&& !(mobj->flags & MF_DROPPED)
	&& (mobj->type != MT_INV)
	&& (mobj->type != MT_INS))
    {
	itemrespawnque[iquehead] = mobj->spawnpoint;
	itemrespawntime[iquehead] = leveltime;
	iquehead = (iquehead+1)&(ITEMQUESIZE-1);

	// lose one off the end?
	if (iquehead == iquetail)
	    iquetail = (iquetail+1)&(ITEMQUESIZE-1);
    }


    // unlink from sector and block lists
    P_UnsetThingPosition (mobj);

#ifdef USE_BOOM_P_ChangeSector
    // Delete all nodes on the current sector_list
    if (sector_list)
    {
	P_DelSeclist(sector_list);
	sector_list = NULL;
    }
#endif

    // free block
    P_RemoveThinker (&mobj->thinker);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_RespawnSpecials
//
void P_RespawnSpecials (void)
{
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

    subsector_t*	ss;
    mobj_t*		mo;
    mapthing_t*		mthing;

    int			i;

    // only respawn items in deathmatch
    if (deathmatch != 2)
	return;	//

    // nothing left to respawn?
    if (iquehead == iquetail)
	return;

    // wait at least 30 seconds
    if (leveltime - itemrespawntime[iquetail] < 30*35)
	return;

    mthing = &itemrespawnque[iquetail];

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector (x,y);
    mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_IFOG);
    S_StartSound (mo, sfx_itmbk);

    // find which type to spawn
    for (i=0 ; i< NUMMOBJTYPES ; i++)
    {
	if (mthing->type == mobjinfo[i].doomednum)
	    break;
    }

    // spawn it
    if (mobjinfo[i].flags & MF_SPAWNCEILING)
	z = ONCEILINGZ;
    else
	z = ONFLOORZ;

    mo = P_SpawnMobj (x,y,z, (mobjtype_t) i);
    mo->spawnpoint = *mthing;
    mo->angle = ANG45 * (mthing->angle/45);

    // pull it from the que
    iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
void P_SpawnPlayer (mapthing_t* mthing)
{
    player_t*		p;
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

    mobj_t*		mobj;

    int			i;

    // not playing?
    if (!playeringame[mthing->type-1])
	return;

    p = &players[mthing->type-1];

    if (p->playerstate == PST_REBORN)
	G_PlayerReborn (mthing->type-1);

    x 		= mthing->x << FRACBITS;
    y 		= mthing->y << FRACBITS;
    z		= ONFLOORZ;
    mobj	= P_SpawnMobj (x,y,z, MT_PLAYER);

    // set color translations for player sprites
    if (mthing->type > 1)
	mobj->flags |= (mthing->type-1)<<MF_TRANSSHIFT;

    mobj->angle	= ANG45 * (mthing->angle/45);
    mobj->player = p;
    mobj->health = p->health;

    p->mo = mobj;
    p->playerstate = PST_LIVE;
    p->refire = 0;
    p->message = NULL;
    p->damagecount = 0;
    p->bonuscount = 0;
    p->extralight = 0;
    p->fixedcolormap = 0;
    p->viewheight = VIEWHEIGHT;

    // setup gun psprite
    P_SetupPsprites (p);

    // give all cards in death match mode
    if (deathmatch)
	for (i=0 ; i<NUMCARDS ; i++)
	    p->cards[i] = true;

    if (mthing->type-1 == consoleplayer)
    {
	// wake up the status bar
	ST_Start ();
	// wake up the heads up text
	HU_Start ();
    }
}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
unsigned int P_SpawnMapThing (mapthing_t* mthing)
{
    int			i;
    int			bit;
    mobj_t*		mobj;
    mobjinfo_t*		ptr;
    dmstart_t*		dmstart;
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;
    unsigned int	type;
    unsigned int	options;


    type = mthing->type;

    // count deathmatch start positions
    if (type == 11)
    {
	dmstart = Z_Malloc (sizeof(dmstart_t),PU_LEVEL,0);
	memcpy (&dmstart->dmstart, mthing, sizeof(*mthing));

	/* Add to the top of the linked list, will need reversing later... */
	dmstart -> next = deathmatchstartlist;
	deathmatchstartlist = dmstart;
	return (0);
    }

    // check for players specially
    if ((type >= 1)
     && (type <= 4))
    {
	// save spots for respawning in network games
	playerstarts[type-1] = *mthing;
	if (!deathmatch)
	    P_SpawnPlayer (mthing);

	return (0);
    }

    options = mthing->options;

    // check for apropriate skill level
    if (!netgame && (options & MTF_NOTSINGLE))
    {
      if ((gamemode == commercial)		// TNT MAP 31 has a yellow
       && (gamemission == pack_tnt)		// key that is marked as multi
       && (gamemap == 31)			// player erroniously
       && (mthing -> type == 6))
      {
	options &= ~MTF_NOTSINGLE;
	//printf ("Yellow key rescued! %X, %X\n",mthing->x, mthing->y);
      }
      else
      {
	return (0);
      }
    }

    if (gameskill == sk_baby)
	bit = 1;
    else if (gameskill == sk_nightmare)
	bit = 4;
    else
	bit = 1<<(gameskill-1);

    if (!(options & bit) )
	return (0);

    // find which type to spawn
    i = 0;
    ptr = mobjinfo;
    do
    {
      if (ptr -> doomednum == type)
	break;

      ptr++;
      if (++i >= NUMMOBJTYPES)
      {
	if (M_CheckParm ("-showunknown"))
	  printf ("P_SpawnMapThing: Unknown type %i at (%i, %i)\n",
		type, mthing->x, mthing->y);
	mthing->options &= ~31;
	return (0);
      }
    } while (1);

    // don't spawn keycards and players in deathmatch
    if (deathmatch && (ptr -> flags & MF_NOTDMATCH))
	return (0);

    // don't spawn any monsters if -nomonsters
    if ((nomonsters1 | nomonsters2)
	&& ( i == MT_SKULL
	     || (ptr -> flags & MF_COUNTKILL)) )
    {
	return (i);
    }

    // spawn it
    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (ptr -> flags & MF_SPAWNCEILING)
	z = ONCEILINGZ;
    else
	z = ONFLOORZ;

    mobj = P_SpawnMobj (x,y,z, (mobjtype_t) i);
    mobj->spawnpoint = *mthing;

    if (mobj->tics > 0)
	mobj->tics = 1 + (P_Random () % mobj->tics);

    if (options & MTF_AMBUSH)
	mobj->flags |= MF_AMBUSH;

    if (options & MTF_FRIEND)
    {
      /* Many maps seem to use extra bits in the options */
      /* word for apparently undefined purposes. */
      /* For now assume that the MTF_FRIEND is valid if */
      /* there are no bits set in the upper byte. */
      if (mtf_mask)				// Default changed?
      {
	if (mtf_mask & MTF_FRIEND)
	  mobj->flags |= MF_FRIEND;
      }
      else if (options < 0x100)
      {
	mobj->flags |= MF_FRIEND;
      }
    }

    mobj->angle = ANG45 * (mthing->angle/45);

    // killough 7/20/98: exclude friends
    if ((mobj->flags & (MF_COUNTKILL|MF_FRIEND)) == MF_COUNTKILL)
	totalkills++;

    if (mobj->flags & MF_COUNTITEM)
	totalitems++;

    return (0);
}

/* -------------------------------------------------------------------------------------------- */
//
// GAME SPAWN FUNCTIONS
//


//
// P_SpawnPuff
//
extern fixed_t attackrange;

void
P_SpawnPuff
( fixed_t	x,
  fixed_t	y,
  fixed_t	z )
{
    int		t;	/* remove dependence on order of evaluation */
    mobj_t*	th;

    t = P_Random();
    z += ((t-P_Random())<<10);

    th = P_SpawnMobj (x,y,z, MT_PUFF);
    th->momz = FRACUNIT;
    th->tics -= P_Random()&3;

    if (th->tics < 1)
	th->tics = 1;

    // don't make punches spark on the wall
    if (attackrange == MELEERANGE)
	P_SetMobjState (th, S_PUFF3);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnBlood
//
void
P_SpawnBlood
( fixed_t	x,
  fixed_t	y,
  fixed_t	z,
  int		damage )
{
    int		t;	/* remove dependence on order of evaluation */
    mobj_t*	th;

    t = P_Random();
    z += ((t-P_Random())<<10);
    th = P_SpawnMobj (x,y,z, MT_BLOOD);
    th->momz = FRACUNIT*2;
    th->tics -= P_Random()&3;

    if (th->tics < 1)
	th->tics = 1;

    if (damage <= 12 && damage >= 9)
	P_SetMobjState (th,S_BLOOD2);
    else if (damage < 9)
	P_SetMobjState (th,S_BLOOD3);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
void P_CheckMissileSpawn (mobj_t* th)
{
    th->tics -= P_Random()&3;
    if (th->tics < 1)
	th->tics = 1;

    // move a little forward so an angle can
    // be computed if it immediately explodes
    th->x += (th->momx>>1);
    th->y += (th->momy>>1);
    th->z += (th->momz>>1);

    if (!P_TryMove (th, th->x, th->y))
	P_ExplodeMissile (th);
}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnMissile
//
mobj_t*
P_SpawnMissile
( mobj_t*	source,
  mobj_t*	dest,
  mobjtype_t	type )
{
    mobj_t*	th;
    angle_t	an;
    int		dist;

    th = P_SpawnMobj (source->x,
		      source->y,
		      source->z + 4*8*FRACUNIT, type);

    if (th->info->seesound)
	S_StartSound (th, th->info->seesound);

    th->target = source;	// where it came from
    an = R_PointToAngle2 (source->x, source->y, dest->x, dest->y);

    // fuzzy player
    if (dest->flags & MF_SHADOW)
    {
	int t=P_Random();	/* remove dependence on order of evaluation */
	an += (t-P_Random())<<20;
    }

    th->angle = an;
    an >>= ANGLETOFINESHIFT;
    th->momx = FixedMul (th->info->speed, finecosine[an]);
    th->momy = FixedMul (th->info->speed, finesine[an]);

    dist = P_ApproxDistance (dest->x - source->x, dest->y - source->y);
    dist = dist / th->info->speed;

    if (dist < 1)
	dist = 1;

    th->momz = (dest->z - source->z) / dist;
    P_CheckMissileSpawn (th);

    return th;
}

/* -------------------------------------------------------------------------------------------- */
//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
void
P_SpawnPlayerMissile
( mobj_t*	source,
  mobjtype_t	type )
{
  mobj_t *th;
  fixed_t x, y, z, slope = 0;

  // see which target is to be aimed at

  angle_t an = source->angle;

  // killough 7/19/98: autoaiming was not in original beta
  {
    // killough 8/2/98: prefer autoaiming at enemies
    int mask = MF_FRIEND;
    do
    {
      slope = P_AimLineAttack(source, an, 16*64*FRACUNIT, mask);
      if (!linetarget)
	slope = P_AimLineAttack(source, an += 1<<26, 16*64*FRACUNIT, mask);
      if (!linetarget)
	slope = P_AimLineAttack(source, an -= 2<<26, 16*64*FRACUNIT, mask);
      if (!linetarget)
	an = source->angle, slope = 0;
    } while (mask && (mask=0, !linetarget));	// killough 8/2/98
  }

  x = source->x;
  y = source->y;
  z = source->z + 4*8*FRACUNIT;

  th = P_SpawnMobj (x,y,z, type);

  if (th->info->seesound)
      S_StartSound (th, th->info->seesound);

  th->target = source;
  th->angle = an;
  th->momx = FixedMul( th->info->speed,
		       finecosine[an>>ANGLETOFINESHIFT]);
  th->momy = FixedMul( th->info->speed,
		       finesine[an>>ANGLETOFINESHIFT]);
  th->momz = FixedMul( th->info->speed, slope);

  P_CheckMissileSpawn (th);
}

/* -------------------------------------------------------------------------------------------- */
