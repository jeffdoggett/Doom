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
//	Teleportation.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_telept.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";
#endif

#include "includes.h"

//-----------------------------------------------------------------------------
//
// TELEPORTATION
//
int
EV_Teleport (line_t* line, int side, mobj_t* thing)
{
  int		tag;
  mobj_t*	m;
  mobj_t*	fog;
  unsigned	an;
  thinker_t*	thinker;
  fixed_t	oldx;
  fixed_t	oldy;
  fixed_t	oldz;
  player_t	*player;

  /* don't teleport missiles */
  /* Don't teleport if hit back of line, */
  /* so you can get out of teleporter. */

  if (side || (thing->flags & MF_MISSILE))
    return (0);

  tag = line->tag;

  for (thinker = thinker_head;
	thinker != NULL;
	thinker = thinker->next)
  {
    if ((thinker->function.acp1 == (actionf_p1) P_MobjThinker)
     && ((m = (mobj_t *) thinker)->type == MT_TELEPORTMAN)
     && (m->subsector->sector->tag == tag))
    {
      oldx = thing->x;
      oldy = thing->y;
      oldz = thing->z;

      if (P_TeleportMove (thing, m->x, m->y, m->z, false))
      {
	thing->z = thing->floorz;  //fixme: not needed?

	/* Adjust player's view, in case there has been a height change */
	/* Voodoo dolls are excluded by making sure player->mo == thing. */
	player = thing->player;
	if (player && (player->mo == thing))
	  player->viewz = thing->z+player->viewheight;

	// spawn teleport fog at source and destination
	fog = P_SpawnMobj (oldx, oldy, oldz, MT_TFOG);
	S_StartSound (fog, sfx_telept);
	an = m->angle >> ANGLETOFINESHIFT;
	fog = P_SpawnMobj (m->x+20*finecosine[an],
				m->y+20*finesine[an],
				thing->z, MT_TFOG);

	// emit sound, where?
	S_StartSound (fog, sfx_telept);

	// don't move for a bit
	if (player)
	  thing->reactiontime = 18;

	thing->angle = m->angle;
	thing->momx = thing->momy = thing->momz = 0;
	return (1);
      }
    }
  }

  return (0);
}

//-----------------------------------------------------------------------------

int EV_SilentTeleport (const line_t *line, int side, mobj_t *thing)
{
  int		tag;
  mobj_t	*m;
  thinker_t	*thinker;
  player_t	*player;
  angle_t	angle;
  fixed_t	z,s,c,momx,momy;
  fixed_t	deltaviewheight;

  //BOOMTRACEOUT("EV_SilentTeleport")

  /* don't teleport missiles */
  /* Don't teleport if hit back of line, */
  /* so you can get out of teleporter. */

  if (side || (thing->flags & MF_MISSILE))
    return (0);

  tag = line->tag;

  for (thinker = thinker_head;
	thinker != NULL;
	thinker = thinker->next)
  {
    if ((thinker->function.acp1 == (actionf_p1) P_MobjThinker)
     && ((m = (mobj_t *) thinker)->type == MT_TELEPORTMAN)
     && (m->subsector->sector->tag == tag))
    {
      /* Height of thing above ground, in case of mid-air teleports: */
      z = thing->z - thing->floorz;

      /* Get the angle between the exit thing and source linedef. */
      /* Rotate 90 degrees, so that walking perpendicularly across */
      /* teleporter linedef causes thing to exit in the direction */
      /* indicated by the exit thing. */
      angle = R_PointToAngle2(0, 0, line->dx, line->dy) - m->angle + ANG90;

      /* Sine, cosine of angle adjustment */
      s = finesine[angle>>ANGLETOFINESHIFT];
      c = finecosine[angle>>ANGLETOFINESHIFT];

      /* Momentum of thing crossing teleporter linedef */
      momx = thing->momx;
      momy = thing->momy;

      /* Whether this is a player, and if so, a pointer to its player_t */
      player = thing->player;

      /* Attempt to teleport, aborting if blocked */
      if (P_TeleportMove(thing, m->x, m->y, m->z, false))
      {
	/* Rotate thing according to difference in angles */
	thing->angle += angle;

	/* Adjust z position to be same height above ground as before */
	thing->z = z + thing->floorz;

	/* Rotate thing's momentum to come out of exit just like it entered */
	thing->momx = FixedMul(momx, c) - FixedMul(momy, s);
	thing->momy = FixedMul(momy, c) + FixedMul(momx, s);

	/* Adjust player's view, in case there has been a height change */
	/* Voodoo dolls are excluded by making sure player->mo == thing. */
	if (player && (player->mo == thing))
	{
	  /* Save the current deltaviewheight, used in stepping */
	  deltaviewheight = player->deltaviewheight;

	  /* Clear deltaviewheight, since we don't want any changes */
	  player->deltaviewheight = 0;

	  /* Set player's view according to the newly set parameters */
	  P_CalcHeight(player);

	  /* Reset the delta to have the same dynamics as before */
	  player->deltaviewheight = deltaviewheight;
	}
	return (1);
      }
    }
  }
  return (0);
}

//-----------------------------------------------------------------------------

#define FUDGEFACTOR 10

int EV_SilentLineTeleport (const line_t *line, int side, mobj_t *thing, boolean reverse)
{
  int i;
  line_t *l;
  int aside;
  int fudge;
  int stepdown;
  player_t *player;
  fixed_t pos;
  angle_t angle;
  fixed_t s,c,x,y,z;
  fixed_t deltaviewheight;


  //BOOMTRACEOUT("EV_SilentLineTeleport")

  if ((line->tag == 0) || side || (thing->flags & MF_MISSILE))
    return (0);

  i = -1;
  while ((i = P_FindLineFromTag(line->tag, i)) >= 0)
  {
    if (((l=lines+i) != line) && (l->backsector))
    {
      /* Get the thing's position along the source linedef */
      pos = abs(line->dx) > abs(line->dy) ?
		FixedDiv(thing->x - line->v1->x, line->dx) :
		FixedDiv(thing->y - line->v1->y, line->dy) ;

      /* Get the angle between the two linedefs, for rotating */
      /* orientation and momentum. Rotate 180 degrees, and flip */
      /* the position across the exit linedef, if reversed. */
      angle = (reverse ? pos = FRACUNIT-pos, 0 : ANG180) +
		R_PointToAngle2(0, 0, l->dx, l->dy) -
		R_PointToAngle2(0, 0, line->dx, line->dy);

      /* Interpolate position across the exit linedef */
      x = l->v2->x - FixedMul(pos, l->dx);
      y = l->v2->y - FixedMul(pos, l->dy);

      /* Sine, cosine of angle adjustment */
      s = finesine[angle>>ANGLETOFINESHIFT];
      c = finecosine[angle>>ANGLETOFINESHIFT];

      /* Maximum distance thing can be moved away from interpolated */
      /* exit, to ensure that it is on the correct side of exit linedef */
      fudge = FUDGEFACTOR;

      /* Whether this is a player, and if so, a pointer to its player_t. */
      /* Voodoo dolls are excluded by making sure thing->player->mo==thing. */
      player = thing->player;
      if ((player) && (player->mo != thing))
	player = NULL;

      /* Whether walking towards first side of exit linedef steps down */
      stepdown = l->frontsector->floorheight < l->backsector->floorheight;

      /* Height of thing above ground */
      z = thing->z - thing->floorz;

      /* Side to exit the linedef on positionally. */
      /* */
      /* Notes: */
      /* */
      /* This flag concerns exit position, not momentum. Due to */
      /* roundoff error, the thing can land on either the left or */
      /* the right side of the exit linedef, and steps must be */
      /* taken to make sure it does not end up on the wrong side. */
      /* */
      /* Exit momentum is always towards side 1 in a reversed */
      /* teleporter, and always towards side 0 otherwise. */
      /* */
      /* Exiting positionally on side 1 is always safe, as far */
      /* as avoiding oscillations and stuck-in-wall problems, */
      /* but may not be optimum for non-reversed teleporters. */
      /* */
      /* Exiting on side 0 can cause oscillations if momentum */
      /* is towards side 1, as it is with reversed teleporters. */
      /* */
      /* Exiting on side 1 slightly improves player viewing */
      /* when going down a step on a non-reversed teleporter. */

      aside = reverse || (player && stepdown);

      /* Make sure we are on correct side of exit linedef. */
      while ((P_PointOnLineSide(x, y, l) != aside) && (--fudge>=0))
	if (abs(l->dx) > abs(l->dy))
	  y -= l->dx < 0 != aside ? -1 : 1;
	else
	  x += l->dy < 0 != aside ? -1 : 1;

      /* Attempt to teleport, aborting if blocked */
      if (P_TeleportMove(thing, x, y, z, false))
      {
	/* Adjust z position to be same height above ground as before. */
	/* Ground level at the exit is measured as the higher of the */
	/* two floor heights at the exit linedef. */
	thing->z = z + sides[l->sidenum[stepdown]].sector->floorheight;

	/* Rotate thing's orientation according to difference in linedef angles */
	thing->angle += angle;

	/* Momentum of thing crossing teleporter linedef */
	x = thing->momx;
	y = thing->momy;

	/* Rotate thing's momentum to come out of exit just like it entered */
	thing->momx = FixedMul(x, c) - FixedMul(y, s);
	thing->momy = FixedMul(y, c) + FixedMul(x, s);

	/* Adjust a player's view, in case there has been a height change */
	if (player)
	{
	  /* Save the current deltaviewheight, used in stepping */
	  deltaviewheight = player->deltaviewheight;

	  /* Clear deltaviewheight, since we don't want any changes now */
	  player->deltaviewheight = 0;

	  /* Set player's view according to the newly set parameters */
	  P_CalcHeight(player);

	  /* Reset the delta to have the same dynamics as before */
	  player->deltaviewheight = deltaviewheight;
	}
	return (1);
      }
    }
  }
  return (0);
}

//-----------------------------------------------------------------------------
