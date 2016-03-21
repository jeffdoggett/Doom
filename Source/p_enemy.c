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
//	Enemy thinking, AI.
//	Action Pointer Functions
//	that are associated with states/frames.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_enemy.c,v 1.5 1997/02/03 22:45:11 b1 Exp $";
#endif

#include <stdlib.h>

#ifdef __riscos
#include "acorn.h"
#endif

#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"
#include "f_finale.h"
#include "g_game.h"
#include "p_enemy.h"

// State.
#include "doomstat.h"
#include "r_state.h"

// Data.
#include "sounds.h"

//-----------------------------------------------------------------------------

typedef enum
{
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_NODIR,
    NUMDIRS

} dirtype_t;


//
// P_NewChaseDir related LUT.
//
dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

dirtype_t diags[] =
{
    DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

//-----------------------------------------------------------------------------

static mobj_t* soundtarget;
static mobj_t* current_actor;
static boolean current_allaround;

//-----------------------------------------------------------------------------
/*
  Icarus level 24 (The Haunting) and Hr level 26 (Afterlife)
  are setup to create ghosts when a Vile resurrects crushed bodies.
*/

#if 1
#define allow_ghosts() 1
#else
/* Future expansion */
int allow_ghosts (void)
{
  map_dests_t * mapd_ptr;

  mapd_ptr = G_Access_MapInfoTab (gameepisode, gamemap);
  return (mapd_ptr -> monster_flags & 2);
}
#endif
//-----------------------------------------------------------------------------
//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

void
P_RecursiveSound
( sector_t*	sec,
  int		soundblocks )
{
    int		i;
    line_t*	check;
    sector_t*	other;

    // wake up all monsters in this sector
    if (sec->validcount == validcount
	&& sec->soundtraversed <= soundblocks+1)
    {
	return;		// already flooded
    }

    sec->validcount = validcount;
    sec->soundtraversed = soundblocks+1;
    sec->soundtarget = soundtarget;

    for (i=0 ;i<sec->linecount ; i++)
    {
	check = sec->lines[i];
	if (! (check->flags & ML_TWOSIDED) )
	    continue;

	P_LineOpening (check);

	if (openrange <= 0)
	    continue;	// closed door

	if ( sides[ check->sidenum[0] ].sector == sec)
	    other = sides[ check->sidenum[1] ] .sector;
	else
	    other = sides[ check->sidenum[0] ].sector;

	if (check->flags & ML_SOUNDBLOCK)
	{
	    if (!soundblocks)
		P_RecursiveSound (other, 1);
	}
	else
	    P_RecursiveSound (other, soundblocks);
    }
}


//
// P_IsVisible
//
// killough 9/9/98: whether a target is visible to a monster
//

static boolean P_IsVisible (mobj_t *actor, mobj_t *mo, boolean allaround)
{
  if (!allaround)
    {
      angle_t an = R_PointToAngle2(actor->x, actor->y,
				   mo->x, mo->y) - actor->angle;
      if (an > ANG90 && an < ANG270 &&
	  P_ApproxDistance(mo->x-actor->x, mo->y-actor->y) > MELEERANGE)
	return false;
    }
  return P_CheckSight(actor, mo);
}

//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void
P_NoiseAlert
( mobj_t*	target,
  mobj_t*	emmiter )
{
    soundtarget = target;
    validcount++;
    P_RecursiveSound (emmiter->subsector->sector, 0);
}




//
// P_CheckMeleeRange
//
boolean P_CheckMeleeRange (mobj_t*	actor)
{
  mobj_t*	pl;
  fixed_t	dist;

  pl = actor->target;
  if (pl == NULL)
    return false;

  // killough 7/18/98: friendly monsters don't attack other friends
  if (actor->flags & pl->flags & MF_FRIEND)
    return false;

  dist = P_ApproxDistance (pl->x-actor->x, pl->y-actor->y);

  if (dist >= MELEERANGE-20*FRACUNIT+pl->info->radius)
    return false;

  // [BH] check height between actor and target
  if (pl->z > actor->z + actor->height || actor->z > pl->z + pl->height)
    return false;

  if (! P_CheckSight (actor, actor->target) )
    return false;

  return true;
}

//
// P_HitFriend()
//
// killough 12/98
// This function tries to prevent shooting at friends

static boolean P_HitFriend(mobj_t *actor)
{
  return (boolean) (actor->target &&
    (P_AimLineAttack(actor,
		     R_PointToAngle2(actor->x, actor->y,
				     actor->target->x, actor->target->y),
		     P_ApproxDistance(actor->x-actor->target->x,
				     actor->y-actor->target->y), 0),
     linetarget) && linetarget != actor->target &&
    !((linetarget->flags ^ actor->flags) & MF_FRIEND));
}

//
// P_CheckMissileRange
//
boolean P_CheckMissileRange (mobj_t* actor)
{
    fixed_t	dist;

    if (! P_CheckSight (actor, actor->target) )
	return false;

    if ( actor->flags & MF_JUSTHIT )
    {
	// the target just hit the enemy,
	// so fight back!
	actor->flags &= ~MF_JUSTHIT;
	return true;
    }

    // killough 7/18/98: friendly monsters don't attack other friendly
    // monsters or players (except when attacked, and then only once)
    if (actor->flags & actor->target->flags & MF_FRIEND)
      return false;

    if (actor->reactiontime)
	return false;	// do not attack yet

    // OPTIMIZE: get this from a global checksight
    dist = P_ApproxDistance ( actor->x-actor->target->x,
			     actor->y-actor->target->y) - 64*FRACUNIT;

    if (!actor->info->meleestate)
	dist -= 128*FRACUNIT;	// no melee attack, so fire more

    dist >>= 16;

    if (actor->type == MT_VILE)
    {
	if (dist > 14*64)
	    return false;	// too far away
    }


    if (actor->type == MT_UNDEAD)
    {
	if (dist < 196)
	    return false;	// close for fist attack
	dist >>= 1;
    }


    if (actor->type == MT_CYBORG
	|| actor->type == MT_SPIDER
	|| actor->type == MT_SKULL)
    {
	dist >>= 1;
    }

    if (dist > 200)
	dist = 200;

    if (actor->type == MT_CYBORG && dist > 160)
	dist = 160;

    if (P_Random () < dist)
	return false;

    if (actor->flags & MF_FRIEND && P_HitFriend(actor))
      return false;

    return true;
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//
fixed_t	xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};

extern	line_t**spechit;
extern	int	numspechit;

boolean P_Move (mobj_t*	actor)
{
    fixed_t tryx;
    fixed_t tryy;

    int     speed;

    if (actor->movedir == DI_NODIR)
	return false;

    speed = actor->info->speed;

    tryx = actor->x + speed * xspeed[actor->movedir];
    tryy = actor->y + speed * yspeed[actor->movedir];

    if (!P_TryMove(actor, tryx, tryy))
    {
	boolean good;

	// open any specials
	if ((actor->flags & MF_FLOAT) && floatok)
	{
	    // must adjust height
	    if (actor->z < tmfloorz)
		actor->z += FLOATSPEED;
	    else
		actor->z -= FLOATSPEED;

	    actor->flags |= MF_INFLOAT;
	    return true;
	}

	if (!numspechit)
	    return false;

	actor->movedir = DI_NODIR;

	// if the special is not a door that can be opened, return false
	//
	// killough 8/9/98: this is what caused monsters to get stuck in
	// doortracks, because it thought that the monster freed itself
	// by opening a door, even if it was moving towards the doortrack,
	// and not the door itself.
	//
	// killough 9/9/98: If a line blocking the monster is activated,
	// return true 90% of the time. If a line blocking the monster is
	// not activated, but some other line is, return false 90% of the
	// time. A bit of randomness is needed to ensure it's free from
	// lockups, but for most cases, it returns the correct result.
	//
	// Do NOT simply return false 1/4th of the time (causes monsters to
	// back out when they shouldn't, and creates secondary stickiness).

	for (good = false; numspechit--;)
	    if (P_UseSpecialLine(actor, spechit[numspechit], 0))
		good = (boolean)(good|(spechit[numspechit] == blockline ? 1 : 2));

	return (boolean)(good && ((P_Random() >= 230) ^ (good & 1)));
    }
    else
    {
	actor->flags &= ~MF_INFLOAT;
    }

    if (!(actor->flags & MF_FLOAT))
	actor->z = actor->floorz;
    return true;
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
boolean P_TryWalk (mobj_t* actor)
{
    if (!P_Move (actor))
    {
	return false;
    }

    actor->movecount = P_Random()&15;
    return true;
}




void P_NewChaseDir (mobj_t*	actor)
{
    fixed_t	deltax;
    fixed_t	deltay;

    dirtype_t	d[3];

    int		tdir;
    dirtype_t	olddir;

    dirtype_t	turnaround;

    if (!actor->target)
	I_Error ("P_NewChaseDir: called with no target");

    olddir = (dirtype_t) actor->movedir;
    turnaround=opposite[olddir];

    deltax = actor->target->x - actor->x;
    deltay = actor->target->y - actor->y;

    if (deltax>10*FRACUNIT)
	d[1]= DI_EAST;
    else if (deltax<-10*FRACUNIT)
	d[1]= DI_WEST;
    else
	d[1]=DI_NODIR;

    if (deltay<-10*FRACUNIT)
	d[2]= DI_SOUTH;
    else if (deltay>10*FRACUNIT)
	d[2]= DI_NORTH;
    else
	d[2]=DI_NODIR;

    // try direct route
    if (d[1] != DI_NODIR
	&& d[2] != DI_NODIR)
    {
	actor->movedir = diags[((deltay<0)<<1)+(deltax>0)];
	if (actor->movedir != turnaround && P_TryWalk(actor))
	    return;
    }

    // try other directions
    if (P_Random() > 200
	||  abs(deltay)>abs(deltax))
    {
	tdir=d[1];
	d[1]=d[2];
	d[2]=(dirtype_t)tdir;
    }

    if (d[1]==turnaround)
	d[1]=DI_NODIR;
    if (d[2]==turnaround)
	d[2]=DI_NODIR;

    if (d[1]!=DI_NODIR)
    {
	actor->movedir = d[1];
	if (P_TryWalk(actor))
	{
	    // either moved forward or attacked
	    return;
	}
    }

    if (d[2]!=DI_NODIR)
    {
	actor->movedir =d[2];

	if (P_TryWalk(actor))
	    return;
    }

    // there is no direct path to the player,
    // so pick another direction.
    if (olddir!=DI_NODIR)
    {
	actor->movedir =olddir;

	if (P_TryWalk(actor))
	    return;
    }

    // randomly determine direction of search
    if (P_Random()&1)
    {
	for ( tdir=DI_EAST;
	      tdir<=DI_SOUTHEAST;
	      tdir++ )
	{
	    if (tdir!=turnaround)
	    {
		actor->movedir =tdir;

		if ( P_TryWalk(actor) )
		    return;
	    }
	}
    }
    else
    {
	for ( tdir=DI_SOUTHEAST;
	      tdir != (DI_EAST-1);
	      tdir-- )
	{
	    if (tdir!=turnaround)
	    {
		actor->movedir =tdir;

		if ( P_TryWalk(actor) )
		    return;
	    }
	}
    }

    if (turnaround !=  DI_NODIR)
    {
	actor->movedir =turnaround;
	if ( P_TryWalk(actor) )
	    return;
    }

    actor->movedir = DI_NODIR;	// can not move
}


//
// PIT_FindTarget
//
// killough 9/5/98
//
// Finds monster targets for other monsters
//

static boolean PIT_FindTarget (mobj_t *mo)
{
  mobj_t *actor = current_actor;

  if (!((mo->flags ^ actor->flags) & MF_FRIEND &&        // Invalid target
	mo->health > 0 && (mo->flags & MF_COUNTKILL || mo->type == MT_SKULL)))
    return true;

  // If the monster is already engaged in a one-on-one attack
  // with a healthy friend, don't attack around 60% the time
  {
    const mobj_t *targ = mo->target;
    if (targ && targ->target == mo &&
	P_Random () > 100 &&
	(targ->flags ^ mo->flags) & MF_FRIEND &&
	targ->health*2 >= targ->info->spawnhealth)
      return true;
  }

  if (!P_IsVisible (actor, mo, current_allaround))
    return true;

//P_SetTarget(&actor->lastenemy, actor->target);	// Remember previous target
  actor->target = mo;					// Found target
  return false;
}


//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
boolean
P_LookForPlayers
( mobj_t*	actor,
  boolean	allaround )
{
    int		c;
    int		stop;
    player_t*	player;
//  sector_t*	sector;
    angle_t	an;
    fixed_t	dist;

//  sector = actor->subsector->sector;

    if (actor->flags & MF_FRIEND)
    {  // killough 9/9/98: friendly monsters go about players differently
      int anyone;

#if 0
      if (!allaround) // If you want friendly monsters not to awaken unprovoked
	return false;
#endif

      // Go back to a player, no matter whether it's visible or not
      for (anyone=0; anyone<=1; anyone++)
	for (c=0; c<MAXPLAYERS; c++)
	  if (playeringame[c] && players[c].playerstate==PST_LIVE &&
	      (anyone || P_IsVisible(actor, players[c].mo, allaround)))
	    {
	      actor->target = players[c].mo;

	      // killough 12/98:
	      // get out of refiring loop, to avoid hitting player accidentally

	      if (actor->info->missilestate)
		{
		  P_SetMobjState (actor, (statenum_t) actor->info->seestate);
		  actor->flags &= ~MF_JUSTHIT;
		}

	      return true;
	    }

      return false;
    }


    c = 0;
    // Change mask of 3 to (MAXPLAYERS-1) -- killough 2/15/98:
    stop = (actor->lastlook-1)&(MAXPLAYERS-1);

    for ( ; ; actor->lastlook = (actor->lastlook+1)&3 )
    {
	if (!playeringame[actor->lastlook])
	    continue;

	if (c++ == 2
	    || actor->lastlook == stop)
	{
	    // done looking
	    return false;
	}

	player = &players[actor->lastlook];
	dist = P_ApproxDistance (player->mo->x - actor->x, player->mo->y - actor->y);

	// [BH] monsters won't see partially invisible player unless too close
	if ((player->powers[pw_invisibility]) && (dist > MELEERANGE * 2))
	  continue;

	if (player->health <= 0)
	    continue;		// dead

	if (!P_CheckSight (actor, player->mo))
	    continue;		// out of sight

	if (!allaround)
	{
	    an = R_PointToAngle2 (actor->x,
				  actor->y,
				  player->mo->x,
				  player->mo->y)
		- actor->angle;

	    if (an > ANG90 && an < ANG270)
	    {
//		dist = P_ApproxDistance (player->mo->x - actor->x, player->mo->y - actor->y);
		// if real close, react anyway
		if (dist > MELEERANGE)
		    continue;	// behind back
	    }
	}

	actor->target = player->mo;
	return true;
    }

    return false;
}




//
// Friendly monsters, by Lee Killough 7/18/98
//
// Friendly monsters go after other monsters first, but
// also return to owner if they cannot find any targets.
// A marine's best friend :)  killough 7/18/98, 9/98
//

static boolean P_LookForMonsters (mobj_t *actor, boolean allaround)
{
#if 0
  // TODO: Won't actually work properly until I add support for this!
  thinker_t *cap, *th;

  if (demo_compatibility)
    return false;

  if (actor->lastenemy && actor->lastenemy->health > 0 && monsters_remember &&
      !(actor->lastenemy->flags & actor->flags & MF_FRIEND)) // not friends
    {
      P_SetTarget(&actor->target, actor->lastenemy);
      P_SetTarget(&actor->lastenemy, NULL);
      return true;
    }

  if (demo_version < 203)  // Old demos do not support monster-seeking bots
    return false;

  // Search the threaded list corresponding to this object's potential targets
  cap = &thinkerclasscap[actor->flags & MF_FRIEND ? th_enemies : th_friends];

  // Search for new enemy

  if (cap->cnext != cap)        // Empty list? bail out early
#endif
    {
      int x = (actor->x - bmaporgx)>>MAPBLOCKSHIFT;
      int y = (actor->y - bmaporgy)>>MAPBLOCKSHIFT;
      int d;

      current_actor = actor;
      current_allaround = allaround;

      // Search first in the immediate vicinity.

      if (!P_BlockThingsIterator(x, y, PIT_FindTarget))
	return true;

      for (d=1; d<5; d++)
	{
	  int i = 1 - d;
	  do
	    if (!P_BlockThingsIterator(x+i, y-d, PIT_FindTarget) ||
		!P_BlockThingsIterator(x+i, y+d, PIT_FindTarget))
	      return true;
	  while (++i < d);
	  do
	    if (!P_BlockThingsIterator(x-d, y+i, PIT_FindTarget) ||
		!P_BlockThingsIterator(x+d, y+i, PIT_FindTarget))
	      return true;
	  while (--i + d >= 0);
	}
#if 0
      {   // Random number of monsters, to prevent patterns from forming
	int n = (P_Random () & 31) + 15;

	for (th = cap->cnext; th != cap; th = th->cnext)
	  if (--n < 0)
	    {
	      // Only a subset of the monsters were searched. Move all of
	      // the ones which were searched so far, to the end of the list.

	      (cap->cnext->cprev = cap->cprev)->cnext = cap->cnext;
	      (cap->cprev = th->cprev)->cnext = cap;
	      (th->cprev = cap)->cnext = th;
	      break;
	   }
	  else
	    if (!PIT_FindTarget((mobj_t *) th))   // If target sighted
	      return true;
      }
#endif
    }

  return false;  // No monster found
}

//
// P_LookForTargets
//
// killough 9/5/98: look for targets to go after, depending on kind of monster
//

static boolean P_LookForTargets(mobj_t *actor, boolean allaround)
{
  return (boolean) (actor->flags & MF_FRIEND ?
    P_LookForMonsters(actor, allaround) || P_LookForPlayers (actor, allaround):
    P_LookForPlayers (actor, allaround) || P_LookForMonsters(actor, allaround));
}


//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	targ;

    actor->threshold = 0;	// any shot will wake up
    targ = actor->subsector->sector->soundtarget;

    if (targ
	&& (targ->flags & MF_SHOOTABLE) )
    {
#if 0
	actor->lastenemy = actor->target;
#endif
	actor->target = targ;

	if (actor->flags & MF_AMBUSH )
	{
	    if (P_CheckSight (actor, actor->target))
		goto seeyou;
	}
	else
	    goto seeyou;
    }


    if (!P_LookForTargets (actor, false) )
	return;

    // go into chase state
  seeyou:
    if (actor->info->seesound)
    {
	int		sound;

	switch (actor->info->seesound)
	{
	  case sfx_posit1:
	  case sfx_posit2:
	  case sfx_posit3:
	    sound = sfx_posit1+P_Random()%3;
	    break;

	  case sfx_bgsit1:
	  case sfx_bgsit2:
	    sound = sfx_bgsit1+P_Random()%2;
	    break;

	  default:
	    sound = actor->info->seesound;
	    break;
	}

	if (actor->type==MT_SPIDER
	    || actor->type == MT_CYBORG)
	{
	    // full volume
	    S_StartSound (NULL, sound);
	}
	else
	    S_StartSound (actor, sound);
    }

    P_SetMobjState (actor, (statenum_t) actor->info->seestate);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase (mobj_t* actor, pspdef_t* psp)
{
    int		delta;

    if (actor->reactiontime)
	actor->reactiontime--;


    // modify target threshold
    if  (actor->threshold)
    {
	if (!actor->target
	    || actor->target->health <= 0)
	{
	    actor->threshold = 0;
	}
	else
	    actor->threshold--;
    }

    // turn towards movement direction if not there yet
    if (actor->movedir < 8)
    {
	actor->angle &= (7U<<29);
	delta = actor->angle - (actor->movedir << 29);

	if (delta > 0)
	    actor->angle -= ANG90/2;
	else if (delta < 0)
	    actor->angle += ANG90/2;
    }

    if (!actor->target
	|| !(actor->target->flags&MF_SHOOTABLE))
    {
	// look for a new target
    //if (P_LookForPlayers(actor, true))
    //  return; // got a new target
    if (P_LookForTargets(actor, true))
	    return; 	// got a new target

	P_SetMobjState (actor, (statenum_t) actor->info->spawnstate);
	return;
    }

    // do not attack twice in a row
    if (actor->flags & MF_JUSTATTACKED)
    {
	actor->flags &= ~MF_JUSTATTACKED;
	if (gameskill != sk_nightmare && !fastparm)
	    P_NewChaseDir (actor);
	return;
    }

    // check for melee attack
    if (actor->info->meleestate
	&& P_CheckMeleeRange (actor))
    {
	if (actor->info->attacksound)
	    S_StartSound (actor, actor->info->attacksound);

	P_SetMobjState (actor, (statenum_t) actor->info->meleestate);
	return;
    }

    // check for missile attack
    if (actor->info->missilestate)
    {
	if (gameskill < sk_nightmare
	    && !fastparm && actor->movecount)
	{
	    goto nomissile;
	}

	if (!P_CheckMissileRange (actor))
	    goto nomissile;

	if (P_SetMobjState (actor, (statenum_t) actor->info->missilestate))
	  actor->flags |= MF_JUSTATTACKED;
	return;
    }

    // ?
  nomissile:
    // possibly choose another target
    if (netgame
	&& !actor->threshold
	&& !P_CheckSight (actor, actor->target) )
    {
	if (P_LookForTargets(actor,true))
	    return;	// got a new target
    }

    // chase towards player
    if (--actor->movecount<0
	|| !P_Move (actor))
    {
	P_NewChaseDir (actor);
    }

    // make active sound
    if (actor->info->activesound
	&& P_Random () < 3)
    {
	S_StartSound (actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget (mobj_t* actor, pspdef_t* psp)
{
    if (!actor->target)
	return;

    actor->flags &= ~MF_AMBUSH;

    actor->angle = R_PointToAngle2 (actor->x,
				    actor->y,
				    actor->target->x,
				    actor->target->y);

    if (actor->target->flags & (MF_SHADOW | MF_TRANSLUCENT))
    {
	int t = P_Random();	/* remove dependence on order of evaluation */
	actor->angle += (t-P_Random())<<21;
    }
}


//
// A_PosAttack
//
void A_PosAttack (mobj_t* actor, pspdef_t* psp)
{
    int		angle;
    int		damage;
    int		slope;
    int		t;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    angle = actor->angle;
    slope = P_AimLineAttack (actor, angle, MISSILERANGE, 0);

    S_StartSound (actor, sfx_pistol);
    t = P_Random();	/* remove dependence on order of evaluation */
    angle += (t-P_Random())<<20;
    damage = ((P_Random()%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack (mobj_t* actor, pspdef_t* psp)
{
    int		i;
    int		angle;
    int		bangle;
    int		damage;
    int		slope;

    if (!actor->target)
	return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor,psp);
    bangle = actor->angle;
    slope = P_AimLineAttack (actor, bangle, MISSILERANGE, 0);

    for (i=0 ; i<3 ; i++)
    {
	int t;
	t = P_Random();	/* remove dependence on order of evaluation */
	angle = bangle + ((t-P_Random())<<20);
	damage = ((P_Random()%5)+1)*3;
	P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
    }
}

void A_CPosAttack (mobj_t* actor, pspdef_t* psp)
{
    int		angle;
    int		bangle;
    int		damage;
    int		slope;
    int		t;

    if (!actor->target)
	return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor,psp);
    bangle = actor->angle;
    slope = P_AimLineAttack (actor, bangle, MISSILERANGE, 0);

    t = P_Random();	/* remove dependence on order or evaluation */
    angle = bangle + ((t-P_Random())<<20);
    damage = ((P_Random()%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire (mobj_t* actor, pspdef_t* psp)
{
    // keep firing unless target got out of sight
    A_FaceTarget (actor,psp);

  // killough 12/98: Stop firing if a friend has gotten in the way
  if (actor->flags & MF_FRIEND && P_HitFriend(actor))
    goto stop;

  // killough 11/98: prevent refiring on friends continuously
  if (P_Random () < 40)
  {
    if (actor->target && actor->flags & actor->target->flags & MF_FRIEND)
      goto stop;
    else
      return;
  }

  if (!actor->target || actor->target->health <= 0
      || !P_CheckSight(actor, actor->target))
    stop: P_SetMobjState(actor, (statenum_t) actor->info->seestate);
}


void A_SpidRefire (mobj_t* actor, pspdef_t* psp)
{
  // keep firing unless target got out of sight
  A_FaceTarget (actor,psp);

  // killough 12/98: Stop firing if a friend has gotten in the way
  if (actor->flags & MF_FRIEND && P_HitFriend(actor))
    goto stop;

  if (P_Random () < 10)
    return;

  // killough 11/98: prevent refiring on friends continuously
  if (!actor->target || actor->target->health <= 0
      || actor->flags & actor->target->flags & MF_FRIEND
      || !P_CheckSight(actor, actor->target))
    stop: P_SetMobjState(actor, (statenum_t) actor->info->seestate);
}

void A_BspiAttack (mobj_t *actor, pspdef_t* psp)
{
    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack (mobj_t* actor, pspdef_t* psp)
{
    int		damage;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    if (P_CheckMeleeRange (actor))
    {
	S_StartSound (actor, sfx_claw);
	damage = (P_Random()%8+1)*3;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }


    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_TROOPSHOT);
}


void A_SargAttack (mobj_t* actor, pspdef_t* psp)
{
    int		damage;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    if (P_CheckMeleeRange (actor))
    {
	damage = ((P_Random()%10)+1)*4;
	P_DamageMobj (actor->target, actor, actor, damage);
    }
}

void A_HeadAttack (mobj_t* actor, pspdef_t* psp)
{
    int		damage;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    if (P_CheckMeleeRange (actor))
    {
	damage = (P_Random()%6+1)*10;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_HEADSHOT);
}

void A_CyberAttack (mobj_t* actor, pspdef_t* psp)
{
    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    P_SpawnMissile (actor, actor->target, MT_ROCKET);
}


void A_BruisAttack (mobj_t* actor, pspdef_t* psp)
{
    int		damage;

    if (!actor->target)
	return;

    // [BH] fix baron nobles not facing targets correctly when attacking
    A_FaceTarget(actor, psp);

    if (P_CheckMeleeRange (actor))
    {
	S_StartSound (actor, sfx_claw);
	damage = (P_Random()%8+1)*10;
	P_DamageMobj (actor->target, actor, actor, damage);
	return;
    }

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	mo;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    actor->z += 16*FRACUNIT;	// so missile spawns higher
    mo = P_SpawnMissile (actor, actor->target, MT_TRACER);
    actor->z -= 16*FRACUNIT;	// back to normal

    mo->x += mo->momx;
    mo->y += mo->momy;
    mo->tracer = actor->target;
}

#define TRACEANGLE 0xC000000

void A_Tracer (mobj_t* actor, pspdef_t* psp)
{
    angle_t	exact;
    fixed_t	dist;
    fixed_t	slope;
    mobj_t*	dest;
    mobj_t*	th;

    /* sync fix (see Boom source) */
    if ((gametic-levelstarttic) & 3)
	return;

    // spawn a puff of smoke behind the rocket
    P_SpawnPuff (actor->x, actor->y, actor->z);

    th = P_SpawnMobj (actor->x-actor->momx,
		      actor->y-actor->momy,
		      actor->z, MT_SMOKE);

    th->momz = FRACUNIT;
    th->tics -= P_Random()&3;
    if (th->tics < 1)
	th->tics = 1;

    // adjust direction
    dest = actor->tracer;

    if (!dest || dest->health <= 0)
	return;

    // change angle
    exact = R_PointToAngle2 (actor->x,
			     actor->y,
			     dest->x,
			     dest->y);

    if (exact != actor->angle)
    {
	if (exact - actor->angle > 0x80000000)
	{
	    actor->angle -= TRACEANGLE;
	    if (exact - actor->angle < 0x80000000)
		actor->angle = exact;
	}
	else
	{
	    actor->angle += TRACEANGLE;
	    if (exact - actor->angle > 0x80000000)
		actor->angle = exact;
	}
    }

    exact = actor->angle>>ANGLETOFINESHIFT;
    actor->momx = FixedMul (actor->info->speed, finecosine[exact]);
    actor->momy = FixedMul (actor->info->speed, finesine[exact]);

    // change slope
    dist = P_ApproxDistance (dest->x - actor->x,
			    dest->y - actor->y);

    dist = dist / actor->info->speed;

    if (dist < 1)
	dist = 1;
    slope = (dest->z+40*FRACUNIT - actor->z) / dist;

    if (slope < actor->momz)
	actor->momz -= FRACUNIT/8;
    else
	actor->momz += FRACUNIT/8;
}


void A_SkelWhoosh (mobj_t* actor, pspdef_t* psp)
{
    if (!actor->target)
	return;
    A_FaceTarget (actor,psp);
    S_StartSound (actor,sfx_skeswg);
}

void A_SkelFist (mobj_t* actor, pspdef_t* psp)
{
    int		damage;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);

    if (P_CheckMeleeRange (actor))
    {
	damage = ((P_Random()%10)+1)*6;
	S_StartSound (actor, sfx_skepch);
	P_DamageMobj (actor->target, actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
mobj_t*		corpsehit;
mobj_t*		vileobj;
fixed_t		viletryx;
fixed_t		viletryy;

boolean PIT_VileCheck (mobj_t*	thing)
{
  int	maxdist;
  boolean check;
  fixed_t height;
  fixed_t radius;

  if (!(thing->flags & MF_CORPSE) )
    return true;	// not a monster

  if (thing->tics != -1)
    return true;	// not lying still yet

  if (thing->info->raisestate == S_NULL)
    return true;	// monster doesn't have a raise state

  maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;

  if (abs(thing->x - viletryx) > maxdist
   || abs(thing->y - viletryy) > maxdist )
    return true;		// not actually touching

  corpsehit = thing;
  corpsehit->momx = corpsehit->momy = 0;

  if (allow_ghosts())
  {
    corpsehit->height <<= 2;
    check = P_CheckPosition (corpsehit, corpsehit->x, corpsehit->y);
    corpsehit->height >>= 2;
  }
  else
  {
    // [BH] don't allow crushed monsters to be resurrected as "ghosts"
    //corpsehit->height <<= 2;
    height = corpsehit->height;
    radius = corpsehit->radius;
    corpsehit->height = corpsehit->info->height;
    corpsehit->radius = corpsehit->info->radius;
    corpsehit->flags |= MF_SOLID;

    check = P_CheckPosition (corpsehit, corpsehit->x, corpsehit->y);

    //corpsehit->height >>= 2;
    corpsehit->height = height;
    corpsehit->radius = radius;
    corpsehit->flags &= ~MF_SOLID;
  }

  if (!check)
    return true;		// doesn't fit here

  return false;			// got one, so stop checking
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase (mobj_t* actor, pspdef_t* psp)
{
    int			xl;
    int			xh;
    int			yl;
    int			yh;

    int			bx;
    int			by;

    mobjinfo_t*		info;
    mobj_t*		temp;

    if (actor->movedir != DI_NODIR)
    {
	// check for corpses to raise
	viletryx =
	    actor->x + actor->info->speed*xspeed[actor->movedir];
	viletryy =
	    actor->y + actor->info->speed*yspeed[actor->movedir];

	xl = (viletryx - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
	xh = (viletryx - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
	yl = (viletryy - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
	yh = (viletryy - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

	vileobj = actor;
	for (bx=xl ; bx<=xh ; bx++)
	{
	    for (by=yl ; by<=yh ; by++)
	    {
		// Call PIT_VileCheck to check
		// whether object is a corpse
		// that canbe raised.
		if (!P_BlockThingsIterator(bx,by,PIT_VileCheck))
		{
		    // got one!
		    temp = actor->target;
		    actor->target = corpsehit;
		    A_FaceTarget (actor,psp);
		    actor->target = temp;

		    P_SetMobjState (actor, S_VILE_HEAL1);
		    S_StartSound (corpsehit, sfx_slop);
		    info = corpsehit->info;

		    if (P_SetMobjState (corpsehit, (statenum_t) info->raisestate))
		    {
		      // [BH] don't allow crushed monsters to be resurrected as "ghosts"
		      if (allow_ghosts())
		      {
			corpsehit->height <<= 2;
		      }
		      else
		      {
			corpsehit->height = info->height;
			corpsehit->radius = info->radius;
		      }

		      // killough 7/18/98:
		      // friendliness is transferred from AV to raised corpse
		      corpsehit->flags =
			(info->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

		      corpsehit->health = info->spawnhealth;
		      corpsehit->target = NULL;
#if 0
		      corpsehit->lastenemy = NULL;
#endif
		      // [BH] remove one from killcount since monster is to be resurrected
		      if ((corpsehit->flags & MF_COUNTKILL)
		       && (actor->target->player->killcount))
			actor->target->player->killcount--;
		    }
		    return;
		}
	    }
	}
    }

    // Return to normal attack.
    A_Chase (actor,psp);
}


//
// A_VileStart
//
void A_VileStart (mobj_t* actor, pspdef_t* psp)
{
    S_StartSound (actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//

void A_StartFire (mobj_t* actor, pspdef_t* psp)
{
    S_StartSound(actor,sfx_flamst);
    A_Fire(actor,psp);
}

void A_FireCrackle (mobj_t* actor, pspdef_t* psp)
{
    S_StartSound(actor,sfx_flame);
    A_Fire(actor,psp);
}

void A_Fire (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	dest;
    unsigned	an;

    dest = actor->tracer;
    if (!dest)
	return;

    // don't move it if the vile lost sight
    if (!P_CheckSight (actor->target, dest) )
	return;

    an = dest->angle >> ANGLETOFINESHIFT;

    P_UnsetThingPosition (actor);
    actor->x = dest->x + FixedMul (24*FRACUNIT, finecosine[an]);
    actor->y = dest->y + FixedMul (24*FRACUNIT, finesine[an]);
    actor->z = dest->z;
    P_SetThingPosition (actor);
    actor->floorz = dest->floorz;
    actor->ceilingz = dest->ceilingz;
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	fog;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);

    fog = P_SpawnMobj (actor->target->x,
		       actor->target->y,
		       actor->target->z, MT_FIRE);

    actor->tracer = fog;
    fog->target = actor;
    fog->tracer = actor->target;
    A_Fire (fog,NULL);
}




//
// A_VileAttack
//
void A_VileAttack (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	fire;
    int		an;

    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);

    if (!P_CheckSight (actor, actor->target) )
	return;

    S_StartSound (actor, sfx_barexp);
    P_DamageMobj (actor->target, actor, actor, 20);
    actor->target->momz = 1000*FRACUNIT/actor->target->info->mass;

    an = actor->angle >> ANGLETOFINESHIFT;

    fire = actor->tracer;

    if (!fire)
	return;

    // move the fire between the vile and the player
    fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
    fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
    P_RadiusAttack (fire, actor, 70 );
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.
//
#define	FATSPREAD	(ANG90/8)

void A_FatRaise (mobj_t *actor, pspdef_t* psp)
{
    A_FaceTarget (actor,psp);
    S_StartSound (actor, sfx_manatk);
}


void A_FatAttack1 (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	mo;
    int		an;

    A_FaceTarget (actor,psp);
    // Change direction  to ...
    actor->angle += FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->angle += FATSPREAD;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}

void A_FatAttack2 (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	mo;
    int		an;

    A_FaceTarget (actor,psp);
    // Now here choose opposite deviation.
    actor->angle -= FATSPREAD;
    P_SpawnMissile (actor, actor->target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->angle -= FATSPREAD*2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}

void A_FatAttack3 (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*	mo;
    int		an;

    A_FaceTarget (actor,psp);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->angle -= FATSPREAD/2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);

    mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
    mo->angle += FATSPREAD/2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}


//
// SkullAttack
// Fly at the player like a missile.
//
#define	SKULLSPEED		(20*FRACUNIT)

void A_SkullAttack (mobj_t* actor, pspdef_t* psp)
{
    mobj_t*		dest;
    angle_t		an;
    int			dist;

    if (!actor->target)
	return;

    dest = actor->target;
    actor->flags |= MF_SKULLFLY;

    S_StartSound (actor, actor->info->attacksound);
    A_FaceTarget (actor,psp);
    an = actor->angle >> ANGLETOFINESHIFT;
    actor->momx = FixedMul (SKULLSPEED, finecosine[an]);
    actor->momy = FixedMul (SKULLSPEED, finesine[an]);
    dist = P_ApproxDistance (dest->x - actor->x, dest->y - actor->y);
    dist = dist / SKULLSPEED;

    if (dist < 1)
	dist = 1;
    actor->momz = (dest->z+(dest->height>>1) - actor->z) / dist;
}


//
// A_BetaSkullAttack()
// killough 10/98: this emulates the beta version's lost soul attacks
//


void A_BetaSkullAttack (mobj_t *actor, pspdef_t* psp)
{
  int damage;
  if (!actor->target || actor->target->type == MT_SKULL)
    return;
  S_StartSound(actor, actor->info->attacksound);
  A_FaceTarget(actor,psp);
  damage = (P_Random()%8+1)*actor->info->damage;
  P_DamageMobj(actor->target, actor, actor, damage);
}

void A_Stop(mobj_t *actor, pspdef_t* psp)
{
  actor->momx = actor->momy = actor->momz = 0;
}


//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void
A_PainShootSkull
( mobj_t*	actor,
  angle_t	angle )
{
    fixed_t	x;
    fixed_t	y;
    fixed_t	z;

    mobj_t*	newmobj;
    angle_t	an;
    int		prestep;
    int		count;
    thinker_t*	currentthinker;

    // count total number of skull currently on the level
    count = 0;

    currentthinker = thinker_head;
    while (currentthinker != NULL)
    {
	if (   (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
	    && ((mobj_t *)currentthinker)->type == MT_SKULL)
	    count++;
	currentthinker = currentthinker->next;
    }

    // if there are already 20 skulls on the level,
    // don't spit another one
    if (count > 20)
	return;


    // okay, there's playe for another one
    an = angle >> ANGLETOFINESHIFT;

    prestep =
	4*FRACUNIT
	+ 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;

    x = actor->x + FixedMul (prestep, finecosine[an]);
    y = actor->y + FixedMul (prestep, finesine[an]);
    z = actor->z + 8*FRACUNIT;

    newmobj = P_SpawnMobj (x , y, z, MT_SKULL);

    // killough 7/20/98: PEs shoot lost souls with the same friendliness
    newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (actor->flags & MF_FRIEND);

    // Check for movements.
    if (!P_TryMove (newmobj, newmobj->x, newmobj->y))
    {
	// kill it immediately
	P_DamageMobj (newmobj,actor,actor,10000);
	return;
    }

    newmobj->target = actor->target;
    A_SkullAttack (newmobj,NULL);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//
void A_PainAttack (mobj_t* actor, pspdef_t* psp)
{
    if (!actor->target)
	return;

    A_FaceTarget (actor,psp);
    A_PainShootSkull (actor, actor->angle);
}


void A_PainDie (mobj_t* actor, pspdef_t* psp)
{
  A_Fall (actor,psp);
  if ((actor->flags2 & MF2_MASSACRE) == 0)
  {
    A_PainShootSkull (actor, actor->angle+ANG90);
    A_PainShootSkull (actor, actor->angle+ANG180);
    A_PainShootSkull (actor, actor->angle+ANG270);
  }
}






void A_Scream (mobj_t* actor, pspdef_t* psp)
{
    int		sound;

    switch (actor->info->deathsound)
    {
      case 0:
	return;

      case sfx_podth1:
      case sfx_podth2:
      case sfx_podth3:
	sound = sfx_podth1 + P_Random ()%3;
	break;

      case sfx_bgdth1:
      case sfx_bgdth2:
	sound = sfx_bgdth1 + P_Random ()%2;
	break;

      default:
	sound = actor->info->deathsound;
	break;
    }

    // Check for bosses.
    if (actor->type==MT_SPIDER
	|| actor->type == MT_CYBORG)
    {
	// full volume
	S_StartSound (NULL, sound);
    }
    else
	S_StartSound (actor, sound);
}


void A_XScream (mobj_t* actor, pspdef_t* psp)
{
    S_StartSound (actor, sfx_slop);
}



void A_SkullPop (mobj_t *actor, pspdef_t* psp)
{
    mobj_t      *mo;
    player_t    *player;

    S_StartSound(actor, sfx_pldeth);

    actor->flags &= ~MF_SOLID;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z + 48 * FRACUNIT, MT_GIBDTH);
    mo->momx = (P_Random() - P_Random()) << 9;
    mo->momy = (P_Random() - P_Random()) << 9;
    mo->momz = FRACUNIT * 2 + (P_Random() << 6);

    // Attach player mobj to bloody skull
    player = actor->player;
    actor->player = NULL;
    mo->player = player;
    mo->health = actor->health;
    mo->angle = actor->angle;
    if (player)
    {
	player->mo = mo;
	player->damagecount = 32;
    }
}



void A_Pain (mobj_t* actor, pspdef_t* psp)
{
    if (actor->info->painsound)
	S_StartSound (actor, actor->info->painsound);
}



void A_Fall (mobj_t *actor, pspdef_t* psp)
{
    // actor is on ground, it can be walked over
    actor->flags &= ~MF_SOLID;

    // So change this if corpse objects
    // are meant to be obstacles.
}


//
// A_Explode
//
void A_Explode (mobj_t* thingy, pspdef_t* psp)
{
    P_RadiusAttack ( thingy, thingy->target, 128 );
}

/* ---------------------------------------------------------------------------- */
/*
   This table replaces the convoluted original code A_BossDeath which can still
   be seen further below.

   Episode 255 = Doom2 level
   Episode or map 0 = match any (ie the MT_KEEN works on all levels)
*/
// Episode, Map, trigger,    tag,	function,		type, next

static bossdeath_t bd_action_7 =
{
  4, 8, 1<<MT_SPIDER,	666, (actionf2) EV_DoFloor,  lowerFloorToLowest, NULL
};

static bossdeath_t bd_action_6 =
{
  4, 6, 1<<MT_CYBORG,	666, (actionf2) EV_DoDoor,   blazeOpen, &bd_action_7
};

static bossdeath_t bd_action_5 =
{
  3, 8, 1<<MT_SPIDER,	0,   (actionf2) G_ExitLevel, 0, &bd_action_6
};

static bossdeath_t bd_action_4 =
{
  2, 8, 1<<MT_CYBORG,	0,   (actionf2) G_ExitLevel, 0, &bd_action_5
};

static bossdeath_t bd_action_3 =
{
  1, 8, 1<<MT_BRUISER,	666, (actionf2) EV_DoFloor,  lowerFloorToLowest, &bd_action_4
};

static bossdeath_t bd_action_2 =
{
  255, 7, 1<<MT_BABY,	667, (actionf2) EV_DoFloor,  raiseToTexture, &bd_action_3
};

static bossdeath_t bd_action_1 =
{
  255, 7, 1<<MT_FATSO,	666, (actionf2) EV_DoFloor,  lowerFloorToLowest, &bd_action_2
};

#if 0
static bossdeath_t bd_action_0 =
{
  0, 0, 1<<MT_KEEN,	666, (actionf2) EV_DoDoor,   normalOpen, &bd_action_1
};
#endif


bossdeath_t * boss_death_actions_head = &bd_action_1;

/* ---------------------------------------------------------------------------- */

// make sure there is a player alive for victory

static int A_PlayerAlive (void)
{
  int i;

  i = 0;
  do
  {
    if (playeringame[i] && players[i].health > 0)
    {
      return (1);
    }
  } while (++i < MAXPLAYERS);

  return (0);
}

/* ---------------------------------------------------------------------------- */
// scan the remaining thinkers to see if all bosses are dead

// Hidden in here is how we correct the double action of the floor in MAP07
// where the centre platform could rise too high if the last two Arachnotrons
// are killed such that the last one dies before the death animation for the
// first one has finished.

// P_KillMobj sets the health of dead monsters to -1.
// When this function gets called it sets it to 0.
// All the other monsters heath must be 0 for all to be fully dead.
// JAD 04/01/12

static int A_AllOtherBossesDead (mobj_t* mo)
{
  mobj_t*	mo2;
  thinker_t*	th;

  mo -> health = 0;		// P_KillMobj() sets this to -1

  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    if (th->function.acp1 == (actionf_p1)P_MobjThinker)
    {
	mo2 = (mobj_t *)th;
	if ((mo2 != mo)
	 && (mo2->type == mo->type)
	 && (mo2->health != 0))
	{
	  // other boss not dead
	  return (0);
	}
    }
  }

  return (1);
}

/* ---------------------------------------------------------------------------- */

static void A_Scan_death_tables (unsigned int monsterbits)
{
  unsigned char epi;
  bossdeath_t * bd_ptr;
  line_t	junk;

  if (gamemode == commercial)
    epi = 255;
  else
    epi = gameepisode;

  bd_ptr = boss_death_actions_head;
  do
  {
    if ((bd_ptr -> monsterbits & monsterbits)
     && ((bd_ptr -> episode == 0) || (bd_ptr -> episode == epi))
     && ((bd_ptr -> map == 0) || (bd_ptr -> map == gamemap)))
    {
      // printf ("Actioned tag %u for %X\n", bd_ptr -> tag, bd_ptr -> monsterbits);
      junk.tag = bd_ptr -> tag;
      (bd_ptr -> func)(&junk,bd_ptr -> action);
    }
    bd_ptr = bd_ptr -> next;
  } while (bd_ptr);
}

/* ---------------------------------------------------------------------------- */
//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//

void A_BossDeath (mobj_t* mo, pspdef_t* psp)
{
  if ((A_AllOtherBossesDead (mo))
   && (A_PlayerAlive ()))
    A_Scan_death_tables (1<<mo->type);
}


/* ---------------------------------------------------------------------------- */
//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
#if 0
void A_BossDeath (mobj_t* mo, pspdef_t* psp)
{
    thinker_t*	th;
    mobj_t*	mo2;
    line_t	junk;
    int		i;

    if ( gamemode == commercial)
    {
	if (gamemap != 7)
	    return;

	if ((mo->type != MT_FATSO)
	    && (mo->type != MT_BABY))
	    return;
    }
    else
    {
	switch(gameepisode)
	{
	  case 1:
	    if (gamemap != 8)
		return;

	    if (mo->type != MT_BRUISER)
		return;
	    break;

	  case 2:
	    if (gamemap != 8)
		return;

	    // Aliens TC has a MT_SPIDER at the end
	    // of map 8 rather than the correct MT_CYBORG
	    // This could have been corrected in the DeHackEd
	    // script, ie make the monster death call "A_BrainDie"
	    // rather than this function.
	    // JAD 01/12/98

    	    if (strncasecmp (F_CastName(MT_SPIDER), "THE ALIEN QUEEN",15) == 0)
	    {
	      if (mo->type != MT_SPIDER)
		return;
	    }
	    else
	    {
	      if (mo->type != MT_CYBORG)
		return;
	    }
	    break;

	  case 3:
	    if (gamemap != 8)
		return;

	    if (mo->type != MT_SPIDER)
		return;

	    break;

	  case 4:
	    switch(gamemap)
	    {
	      case 6:
		if (mo->type != MT_CYBORG)
		    return;
		break;

	      case 8:
		if (mo->type != MT_SPIDER)
		    return;
		break;

	      default:
		return;
		break;
	    }
	    break;

	  default:
	    if (gamemap != 8)
		return;
	    break;
	}

    }


    // make sure there is a player alive for victory
    for (i=0 ; i<MAXPLAYERS ; i++)
	if (playeringame[i] && players[i].health > 0)
	    break;

    if (i==MAXPLAYERS)
	return;	// no one left alive, so do not end game

    // scan the remaining thinkers to see
    // if all bosses are dead
    for (th = thinker_head ; th != NULL ; th=th->next)
    {
	if (th->function.acp1 != (actionf_p1)P_MobjThinker)
	    continue;

	mo2 = (mobj_t *)th;
	if (mo2 != mo
	    && mo2->type == mo->type
	    && mo2->health > 0)
	{
	    // other boss not dead
	    return;
	}
    }

    // victory!
    if ( gamemode == commercial)
    {
	if (gamemap == 7)
	{
	    if (mo->type == MT_FATSO)
	    {
		junk.tag = 666;
		EV_DoFloor(&junk,lowerFloorToLowest);
		return;
	    }

	    if (mo->type == MT_BABY)
	    {
		junk.tag = 667;
		EV_DoFloor(&junk,raiseToTexture);
		return;
	    }
	}
    }
    else
    {
	switch(gameepisode)
	{
	  case 1:
	    junk.tag = 666;
	    EV_DoFloor (&junk, lowerFloorToLowest);
	    return;
	    break;

	  case 4:
	    switch(gamemap)
	    {
	      case 6:
		junk.tag = 666;
		EV_DoDoor (&junk, blazeOpen);
		return;
		break;

	      case 8:
		junk.tag = 666;
		EV_DoFloor (&junk, lowerFloorToLowest);
		return;
		break;
	    }
	}
    }

    G_ExitLevel ();
}
#endif


/* ---------------------------------------------------------------------------- */
//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie (mobj_t* mo, pspdef_t* psp)
{
  A_Fall (mo,psp);
#if 0
  A_BossDeath (mo);		// Cannot call this as the dehacked may have called
				// this function for some other monster death.
#else
  // scan the remaining thinkers
  // to see if all Keens are dead
  if (A_AllOtherBossesDead (mo))
  {
    line_t junk;
    junk.tag = 666;
#if 1
    EV_DoDoor (&junk,normalOpen);
#else
    EV_DoFloor (&junk,lowerFloorToLowest);
    EV_DoCeiling (&junk, raiseToHighest);
#endif
  }
#endif
}

/* ---------------------------------------------------------------------------- */

void A_Activate_Death_Sectors (unsigned int monsterbits)
{
  line_t junk;

  // printf ("A_Activate_Death_Sectors (%X)\n", monsterbits);

  A_Scan_death_tables (monsterbits);

  /* Finally do the Commander Keen actions */
  if (monsterbits & (1<<MT_KEEN))
  {
    // printf ("Actioned keen tag\n");
    junk.tag = 666;
    EV_DoDoor (&junk,normalOpen);
  }
}

/* ---------------------------------------------------------------------------- */

void A_Hoof (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_hoof);
    A_Chase (mo,psp);
}

void A_Metal (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_metal);
    A_Chase (mo,psp);
}

void A_BabyMetal (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_bspwlk);
    A_Chase (mo,psp);
}

void A_OpenShotgun2 (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_dbopn);
}

void A_LoadShotgun2 (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_dbload);
}


void A_CloseShotgun2 (mobj_t* mo, pspdef_t* psp)
{
  S_StartSound (mo, sfx_dbcls);
  A_ReFire (mo,psp);
}



unsigned int	braintargetted;
boolean		nobrainspitcheat;

void A_BrainAwake (mobj_t* mo, pspdef_t* psp)
{
  S_StartSound (NULL,sfx_bossit);
}


void A_BrainPain (mobj_t* mo, pspdef_t* psp)
{
  S_StartSound (NULL,sfx_bospn);
}


void A_BrainScream (mobj_t* mo, pspdef_t* psp)
{
    int		x;
    int		y;
    int		z;
    mobj_t*	th;

  // [BH] center the explosions over the boss brain
  //for (x = mo->x - 196 * FRACUNIT ; x < mo->x + 320 * FRACUNIT; x += FRACUNIT * 8)
  for (x = mo->x - 258 * FRACUNIT ; x < mo->x + 258 * FRACUNIT; x += FRACUNIT * 8)
    {
	y = mo->y - 320*FRACUNIT;
	z = 128 + P_Random()*2*FRACUNIT;
	th = P_SpawnMobj (x,y,z, MT_ROCKET);
	th->momz = P_Random()*512;

	P_SetMobjState (th, S_BRAINEXPLODE1);

	th->tics -= P_Random()&7;
	if (th->tics < 1)
	    th->tics = 1;
    }

    S_StartSound (NULL,sfx_bosdth);
}



void A_BrainExplode (mobj_t* mo, pspdef_t* psp)
{
    int		x;
    int		y;
    int		z;
    int		t;
    mobj_t*	th;

    t = P_Random();	/* remove dependence on order of evaluation */
    x = mo->x + (t - P_Random ())*2048;
    y = mo->y;
    z = 128 + P_Random()*2*FRACUNIT;
    th = P_SpawnMobj (x,y,z, MT_ROCKET);
    th->momz = P_Random()*512;

    P_SetMobjState (th, S_BRAINEXPLODE1);

    th->tics -= P_Random()&7;
    if (th->tics < 1)
	th->tics = 1;
}


void A_BrainDie (mobj_t* mo, pspdef_t* psp)
{
    G_ExitLevel ();
}

static mobj_t* A_NextBrainTarget (void)
{
  unsigned int	found_so_far;
  thinker_t*	thinker;
  mobj_t*	m;
  mobj_t*	found_1;

  // find all the target spots
  found_so_far = 0;
  found_1 = NULL;

  for (thinker = thinker_head ;
	 thinker != NULL ;
	 thinker = thinker->next)
  {
    if (thinker->function.acp1 != (actionf_p1)P_MobjThinker)
      continue;	// not a mobj

    m = (mobj_t *)thinker;

    if (m->type == MT_BOSSTARGET)
    {
      if (found_so_far == braintargetted)	// This one the one that we want?
      {
	braintargetted++;			// Yes.
	return (m);
      }
      found_so_far++;
      if (found_1 == NULL)		// Remember first one in case we wrap.
	found_1 = m;
    }
  }

  braintargetted = 1;			// Start again.
  return (found_1);
}


void A_BrainSpit (mobj_t* mo, pspdef_t* psp)
{
  int		dist;
  mobj_t*	targ;
  mobj_t*	newmobj;

  static int	easy = 0;

  if (nobrainspitcheat)
    return;

  easy ^= 1;
  if (gameskill <= sk_easy && (!easy))
    return;

  // shoot a cube at current target
  targ = A_NextBrainTarget ();

  if (targ)
  {
    // spawn brain missile
    newmobj = P_SpawnMissile (mo, targ, MT_SPAWNSHOT);
    newmobj->target = targ;

    // killough 7/18/98: brain friendliness is transferred
    newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);

    dist = P_ApproxDistance (targ->x - (mo->x + mo->momx), targ->y - (mo->y + mo->momy));

    // Use the reactiontime to hold the distance
    // from the target after the next move.

    newmobj->reactiontime = dist;
    S_StartSound(NULL, sfx_bospit);
  }
}



void A_SpawnFly (mobj_t* mo, pspdef_t* psp)
{
  mobj_t*	newmobj;
  mobj_t*	fog;
  mobj_t*	targ;
  int		r;
  int		dist;
  mobjtype_t	type;

  targ = mo->target;
  if (targ)
  {
    dist = P_ApproxDistance (targ->x - (mo->x + mo->momx), targ->y - (mo->y + mo->momy));
    // Will the next move put the cube closer to
    // the target point than it is now?
    if ((unsigned) dist < (unsigned) mo->reactiontime)
    {
      mo->reactiontime = dist;			// Yes. Still flying
      return;
    }

    if ((nomonsters1 | nomonsters2) == 0)
    {
      // First spawn teleport fog.
      fog = P_SpawnMobj (targ->x, targ->y, targ->z, MT_SPAWNFIRE);
      S_StartSound (fog, sfx_telept);

      // Randomly select monster to spawn.
      r = P_Random ();

      // Probability distribution (kind of :),
      // decreasing likelihood.
      if ( r<50 )
	  type = MT_TROOP;
      else if (r<90)
	  type = MT_SERGEANT;
      else if (r<120)
	  type = MT_SHADOWS;
      else if (r<130)
	  type = MT_PAIN;
      else if (r<160)
	  type = MT_HEAD;
      else if (r<162)
	  type = MT_VILE;
      else if (r<172)
	  type = MT_UNDEAD;
      else if (r<192)
	  type = MT_BABY;
      else if (r<222)
	  type = MT_FATSO;
      else if (r<246)
	  type = MT_KNIGHT;
      else
	  type = MT_BRUISER;

      newmobj = P_SpawnMobj (targ->x, targ->y, targ->z, type);

      // killough 7/18/98: brain friendliness is transferred
      newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);

      if ((P_LookForPlayers (newmobj, true) == 0)
       || (P_SetMobjState (newmobj, (statenum_t) newmobj->info->seestate)))
	P_TeleportMove (newmobj, newmobj->x, newmobj->y, newmobj->z, true); // telefrag anything in this spot

      // [BH] increase totalkills
      totalkills++;
    }
  }

  // remove self (i.e., cube).
  P_RemoveMobj (mo);
}

// travelling cube sound
void A_SpawnSound (mobj_t* mo, pspdef_t* psp)
{
  S_StartSound (mo,sfx_boscub);

  /* Note: Batman TC calls this function when BANE dies at the end */
  /* of map 30. If we let it call A_SpawnFly then it's bad news! */

  if (mo->type == MT_SPAWNSHOT)
    A_SpawnFly (mo,psp);
}



void A_PlayerScream (mobj_t* mo, pspdef_t* psp)
{
    // Default death sound.
    int		sound = sfx_pldeth;

    if ( (gamemode == commercial)
	&& 	(mo->health < -50))
    {
	// IF THE PLAYER DIES
	// LESS THAN -50% WITHOUT GIBBING
	sound = sfx_pdiehi;
    }

    S_StartSound (mo, sound);
}


// killough 11/98: kill an object
void A_Die(mobj_t *actor, pspdef_t* psp)
{
    P_DamageMobj(actor, NULL, NULL, actor->health);
}

//
// A_Detonate
// killough 8/9/98: same as A_Explode, except that the damage is variable
//
void A_Detonate(mobj_t *mo, pspdef_t* psp)
{
    P_RadiusAttack(mo, mo->target, mo->info->damage);
}

//
// killough 9/98: a mushroom explosion effect, sorta :)
// Original idea: Linguica
//
void A_Mushroom(mobj_t *actor, pspdef_t* psp)
{
  int		i, j, n;
  fixed_t	misc1;
  fixed_t	misc2;

  // Mushroom parameters are part of code pointer's state
  n = actor->info->damage;
  if ((misc1 = (fixed_t)actor->state->misc1) == 0)
    misc1 = FRACUNIT * 4;
  if ((misc2 = (fixed_t)actor->state->misc2) == 0)
    misc2 = FRACUNIT / 2;

    A_Explode(actor,psp);					// First make normal explosion

    // Now launch mushroom cloud
    for (i = -n; i <= n; i += 8)
	for (j = -n; j <= n; j += 8)
	{
	    mobj_t target = *actor, *mo;

	    target.x += i << FRACBITS;				// Aim in many directions from source
	    target.y += j << FRACBITS;
	    target.z += P_ApproxDistance(i, j) * misc1;		// Aim up fairly high
	    mo = P_SpawnMissile(actor, &target, MT_FATSHOT);	// Launch fireball
	    mo->momx = FixedMul(mo->momx, misc2);
	    mo->momy = FixedMul(mo->momy, misc2);		// Slow down a bit
	    mo->momz = FixedMul(mo->momz, misc2);
	    mo->flags &= ~MF_NOGRAVITY;				// Make debris fall under gravity
	}
}

//
// killough 11/98
//
// The following were inspired by Len Pitre
//
// A small set of highly-sought-after code pointers
//
void A_Spawn (mobj_t *mo, pspdef_t* psp)
{
  mobj_t *newmobj;

  if (mo->state->misc1)
  {
    newmobj = P_SpawnMobj(mo->x, mo->y, (fixed_t)(mo->state->misc2 << FRACBITS) + mo->z, (mobjtype_t)(mo->state->misc1 - 1));
    newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
  }
}

void A_Turn (mobj_t *mo, pspdef_t* psp)
{
    mo->angle += (unsigned int)(((uint64_t)mo->state->misc1 << 32) / 360);
}

void A_Face (mobj_t *mo, pspdef_t* psp)
{
    mo->angle = (unsigned int)(((uint64_t)mo->state->misc1 << 32) / 360);
}

void A_Scratch (mobj_t *mo, pspdef_t* psp)
{
    mo->target && (A_FaceTarget(mo,psp), P_CheckMeleeRange(mo)) ?
	mo->state->misc2 ? S_StartSound(mo, (fixed_t)mo->state->misc2) : (void)0,
	P_DamageMobj(mo->target, mo, mo, (fixed_t)mo->state->misc1) : (void)0;
}

void A_PlaySound (mobj_t *mo, pspdef_t* psp)
{
    S_StartSound(mo->state->misc2 ? NULL : mo, (fixed_t)mo->state->misc1);
}

void A_RandomJump (mobj_t *mo, pspdef_t* psp)
{
  state_t * state;
  player_t* player;

  if ((psp)
   && ((player = mo->player) != NULL))
  {
    state = psp->state;
    if (P_Random() < state->misc2)
      P_SetPsprite (player, psp, (statenum_t)state->misc1);
  }
  else
  {
    state = mo->state;
    if (P_Random() < state->misc2)
      P_SetMobjState (mo, (statenum_t)state->misc1);
  }
}

//
// This allows linedef effects to be activated inside deh frames.
//
void A_LineEffect (mobj_t *mo, pspdef_t* psp)
{
  static line_t	junk;
  player_t	player;
  player_t	*oldplayer;

  junk = *lines;
  oldplayer = mo->player;
  mo->player = &player;
  player.health = 100;
  junk.special = (dushort_t)mo->state->misc1;
  if (!junk.special)
    return;
  junk.tag = (dushort_t)mo->state->misc2;
  if (!P_UseSpecialLine (mo, &junk, 0))
    P_CrossSpecialLine (&junk, 0, mo);
  mo->state->misc1 = junk.special;
  mo->player = oldplayer;
}
