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
//	Player related stuff.
//	Bobbing POV/weapon, movement.
//	Pending weapon.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: p_user.c,v 1.3 1997/01/28 22:08:29 b1 Exp $";
#endif

#include "includes.h"

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP		32


//
// Movement.
//

// 16 pixels of bob
#define MAXBOB	0x100000

static boolean	onground;


//
// P_Thrust
// Moves the given origin along a given angle.
//
void
P_Thrust
( player_t*	player,
  angle_t	angle,
  fixed_t	move )
{
  mobj_t   *	mo;

  mo = player->mo;
  angle >>= ANGLETOFINESHIFT;

  mo->momx += FixedMul(move,finecosine[angle]);
  mo->momy += FixedMul(move,finesine[angle]);
}




//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight (player_t* player)
{
  int		angle;
  fixed_t	bob;
  mobj_t   *	mo;

  mo = player->mo;

  // Regular movement bobbing
  // (needs to be calculated for gun swing
  // even if not on ground)
  // OPTIMIZE: tablify angle
  // Note: a LUT allows for effects
  //  like a ramp with low health.

  if ((mo->flags & MF_SLIDE)		// No bob if we are riding a conveyor
   && (player->cmd.forwardmove == 0)
   && (player->cmd.sidemove == 0))
  {
    bob = 0;
  }
  else
  {
    bob =
	FixedMul (mo->momx, mo->momx)
	+ FixedMul (mo->momy,mo->momy);

    bob >>= 2;

    if (bob > MAXBOB)
      bob = MAXBOB;
  }

  player->bob = bob;

  if ((player->cheats & CF_NOMOMENTUM) || !onground)
  {
    player->viewz = mo->z + VIEWHEIGHT;

    if (player->viewz > mo->ceilingz-4*FRACUNIT)
      player->viewz = mo->ceilingz-4*FRACUNIT;

    player->viewz = mo->z + player->viewheight;
    return;
  }

  angle = (FINEANGLES/20*leveltime)&FINEMASK;
  bob = FixedMul ( player->bob/2, finesine[angle]);


  // move viewheight
  if (player->playerstate == PST_LIVE)
  {
    player->viewheight += player->deltaviewheight;

    if (player->viewheight > VIEWHEIGHT)
    {
      player->viewheight = VIEWHEIGHT;
      player->deltaviewheight = 0;
    }

    if (player->viewheight < VIEWHEIGHT/2)
    {
      player->viewheight = VIEWHEIGHT/2;
      if (player->deltaviewheight <= 0)
	player->deltaviewheight = 1;
    }

    if (player->deltaviewheight)
    {
      player->deltaviewheight += FRACUNIT/4;
      if (!player->deltaviewheight)
	player->deltaviewheight = 1;
    }
  }
  player->viewz = mo->z + player->viewheight + bob;

  if (player->viewz > mo->ceilingz-4*FRACUNIT)
    player->viewz = mo->ceilingz-4*FRACUNIT;
}



//
// P_MovePlayer
//
void P_MovePlayer (player_t* player)
{
  int j;	/* ARM compiler fails to sign extend char to int! */
  ticcmd_t *	cmd;
  mobj_t   *	mo;

  mo = player->mo;
  cmd = &player->cmd;

  mo->angle += (cmd->angleturn<<16);

  // Do not let the player control movement
  //  if not onground.
  onground = (boolean) (mo->z <= mo->floorz || (mo->flags2 & MF2_ONMOBJ));

  if (onground)
  {
    if ((j = cmd->forwardmove) != 0)
    {
      if (j > 0x7F) j |= ~0xFF;
      P_Thrust (player, mo->angle, j*2048);
    }

    if ((j = cmd->sidemove) != 0)
    {
      if (j > 0x7F) j |= ~0xFF;
      P_Thrust (player, mo->angle-ANG90, j*2048);
    }
  }

  if ( (cmd->forwardmove || cmd->sidemove)
   && mo->state == &states[S_PLAY] )
  {
    P_SetMobjState (mo, S_PLAY_RUN1);
  }
}



//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
#define ANG5   	(ANG90/18)

void P_DeathThink (player_t* player)
{
  angle_t		angle;
  angle_t		delta;
  mobj_t*		mo;

  P_MovePsprites (player);

  // fall to the ground
  if (player->viewheight > 6*FRACUNIT)
    player->viewheight -= FRACUNIT;

  if (player->viewheight < 6*FRACUNIT)
    player->viewheight = 6*FRACUNIT;

  player->deltaviewheight = 0;

  mo = player->mo;

  onground = (boolean) (mo->z <= mo->floorz);
  P_CalcHeight (player);

  if (player->attacker && player->attacker != mo)
  {
    angle = R_PointToAngle2 (mo->x,
			     mo->y,
			     player->attacker->x,
			     player->attacker->y);

    delta = angle - mo->angle;

    if (delta < ANG5 || delta > (unsigned)-ANG5)
    {
      // Looking at killer,
      //  so fade damage flash down.
      mo->angle = angle;

      if (player->damagecount)
	player->damagecount--;
    }
    else if (delta < ANG180)
      mo->angle += ANG5;
    else
      mo->angle -= ANG5;
  }
  else if (player->damagecount)
    player->damagecount--;


  if (player->cmd.buttons & BT_USE)
    player->playerstate = PST_REBORN;
}



//
// P_PlayerThink
//
void P_PlayerThink (player_t* player)
{
  mobj_t*	mo;
  ticcmd_t*	cmd;
  weapontype_t	newweapon;

  mo = player->mo;

  // fixme: do this in the cheat code
  if (player->cheats & CF_NOCLIP)
    mo->flags |= MF_NOCLIP;
  else
    mo->flags &= ~MF_NOCLIP;

  // chain saw run forward
  cmd = &player->cmd;
  if (mo->flags & MF_JUSTATTACKED)
  {
    cmd->angleturn = 0;
    cmd->forwardmove = 0xc800/512;
    cmd->sidemove = 0;
    mo->flags &= ~MF_JUSTATTACKED;
  }


  if (player->playerstate == PST_DEAD)
  {
    P_DeathThink (player);
    return;
  }

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.
  if (mo->reactiontime)
    mo->reactiontime--;
  else
    P_MovePlayer (player);

  P_CalcHeight (player);

  if (mo->subsector->sector->special)
    P_PlayerInSpecialSector (player);

  // Check for weapon change.

  // A special event has no other buttons.
  if (cmd->buttons & BT_SPECIAL)
    cmd->buttons = 0;

  if (cmd->buttons & BT_CHANGE)
  {
    // The actual changing of the weapon is done
    //  when the weapon psprite can do it
    //  (read: not in the middle of an attack).
    newweapon = (weapontype_t) ((cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT);

    if (newweapon == wp_fist
      && player->weaponowned[wp_chainsaw]
      && !(player->readyweapon == wp_chainsaw
      && player->powers[pw_strength]))
    {
      newweapon = wp_chainsaw;
    }

    if ( (gamemode == commercial)
     && newweapon == wp_shotgun
     && player->weaponowned[wp_supershotgun]
     && player->readyweapon != wp_supershotgun)
    {
      newweapon = wp_supershotgun;
    }


    if ((player->weaponowned[newweapon])
     && (newweapon != player->readyweapon)
     && (newweapon != player->pendingweapon))	// Added so that the sound (getpow) only gets played once.
    {
      // Do not go to plasma or BFG in shareware,
      //  even if cheated.
      if ((newweapon != wp_plasma
       && newweapon != wp_bfg)
       || (gamemode != shareware) )
      {
	player->pendingweapon = newweapon;
	if ((newweapon == wp_fist)
	 && (player->powers[pw_strength]))
	  S_StartSound (NULL, sfx_getpow);
      }
    }
  }

  // check for use
  if (cmd->buttons & BT_USE)
  {
    if (!player->usedown)
    {
      P_UseLines (player);
      player->usedown = true;
    }
  }
  else
    player->usedown = false;

  // cycle psprites
  P_MovePsprites (player);

  // Counters, time dependend power ups.

  // Strength counts up to diminish fade.
  if (player->powers[pw_strength])
    player->powers[pw_strength]++;

  if (player->powers[pw_invulnerability])
    player->powers[pw_invulnerability]--;

  if ((player->powers[pw_invisibility])
   && (--player->powers[pw_invisibility] == 0))
    mo->flags &= ~MF_SHADOW;

  if (player->powers[pw_infrared])
    player->powers[pw_infrared]--;

  if (player->powers[pw_ironfeet])
    player->powers[pw_ironfeet]--;

  if (player->damagecount)
    player->damagecount--;

  if (player->bonuscount)
    player->bonuscount--;

  // Handling colormaps.
  if ((player->powers[pw_invulnerability] > 4*32)
   || (player->powers[pw_invulnerability]&8))
    player->fixedcolormap = INVERSECOLORMAP;
  else
  if ((player->powers[pw_infrared] > 4*32)
   || (player->powers[pw_infrared]&8))
    player->fixedcolormap = 1;		// almost full bright
  else
    player->fixedcolormap = 0;
}


