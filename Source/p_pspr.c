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
//	Weapon sprite animation, weapon objects.
//	Action functions for weapons.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_pspr.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"

#define LOWERSPEED		(FRACUNIT*6)
#define RAISESPEED		(FRACUNIT*6)

// plasma cells for a bfg attack
unsigned int bfg_cells = 40;

extern int setblocks;
int	weaponrecoil;

extern void P_Thrust (player_t* player, angle_t angle, fixed_t move);

//-----------------------------------------------------------------------------

static const int recoil[] = // phares
{
  10,	// wp_fist
  10,	// wp_pistol
  30,	// wp_shotgun
  10,	// wp_chaingun
  100,	// wp_missile
  20,	// wp_plasma
  100,	// wp_bfg
  0,	// wp_chainsaw
  80	// wp_supershotgun
};

//-----------------------------------------------------------------------------
/* This was this:
#define WEAPONBOTTOM		128*FRACUNIT
#define WEAPONTOP		32*FRACUNIT
when the screenheight was 200 but I couldn't find the formula for
deriving 240 screenheight for all screen magnifications - hence
the function below to work it out.
J.A.Doggett 26/02/98.
*/
#if 0
static fixed_t weapon_top (void)
{
  if (weaponscale > 1)
    return (32*FRACUNIT);

  return (132*FRACUNIT);
}

//#define weapon_bottom() (weapon_top()+(96*FRACUNIT))

static fixed_t weapon_bottom (void)
{
  if (weaponscale > 1)
    return ((32+96)*FRACUNIT);

  return ((132+96)*FRACUNIT);
}
#endif

#define weapon_top()	(weaponscale > 1)?(32*FRACUNIT):(132*FRACUNIT)
#define weapon_bottom()	(weaponscale > 1)?((32+96)*FRACUNIT):((132+96)*FRACUNIT)

//-----------------------------------------------------------------------------
//
// P_SetPsprite
//
void P_SetPsprite (player_t* player, pspdef_t* psp, statenum_t stnum)
{
  state_t*	state;
  fixed_t	y;

  do
  {
    if (!stnum)
    {
      // object removed itself
      psp->state = NULL;
      break;
    }

    state = &states[stnum];
    psp->state = state;
    psp->tics = (int) state->tics;	// could be 0

    if (state->misc1)
    {
      // coordinate set
      psp->sx = (fixed_t) (state->misc1 << FRACBITS);
      y = (fixed_t) (state->misc2 << FRACBITS);
      if (weaponscale <= 1)		// Mayhem19.wad uses this for the
        y += (100*FRACUNIT);		// replacement for the BFG.
      psp->sy = y;
    }

    // Call action routine.
    // Modified handling.
    if (state->action.acp2)
    {
      state->action.acp2 (player->mo, psp);

      if (!psp->state)
	  break;
    }

    stnum = psp->state->nextstate;
  } while (!psp->tics);
  // an initial state of 0 could cycle through
}

//-----------------------------------------------------------------------------
//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
static void P_BringUpWeapon (player_t* player)
{
    statenum_t	newstate;

    if (player->pendingweapon == wp_nochange)
	player->pendingweapon = player->readyweapon;

    if (player->pendingweapon == wp_chainsaw)
	S_StartSound (player->mo, sfx_sawup);

    newstate = (statenum_t) weaponinfo[player->pendingweapon].upstate;

    player->pendingweapon = wp_nochange;
    player->psprites[ps_weapon].sy = weapon_bottom();

    P_SetPsprite (player, &player->psprites[ps_weapon], newstate);
}

//-----------------------------------------------------------------------------
//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
static boolean P_CheckAmmo (player_t* player)
{
    ammotype_t		ammo;
    int			count;

    ammo = weaponinfo[player->readyweapon].ammo;

    // Minimal amount for one shot varies.
    if (player->readyweapon == wp_bfg)
	count = bfg_cells;
    else if (player->readyweapon == wp_supershotgun)
	count = 2;	// Double barrel.
    else
	count = 1;	// Regular.

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if (ammo == am_noammo || player->ammo[ammo] >= count)
	return true;

    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
    do
    {
	if (player->weaponowned[wp_plasma]
	    && player->ammo[am_cell]
	    && (gamemode != shareware) )
	{
	    player->pendingweapon = wp_plasma;
	}
	else if (player->weaponowned[wp_supershotgun]
		 && player->ammo[am_shell]>=2
		 && (gamemode == commercial) )
	{
	    player->pendingweapon = wp_supershotgun;
	}
	else if (player->weaponowned[wp_chaingun]
		 && player->ammo[am_clip])
	{
	    player->pendingweapon = wp_chaingun;
	}
	else if (player->weaponowned[wp_shotgun]
		 && player->ammo[am_shell])
	{
	    player->pendingweapon = wp_shotgun;
	}
	else if (player->ammo[am_clip])
	{
	    player->pendingweapon = wp_pistol;
	}
	else if (player->weaponowned[wp_chainsaw])
	{
	    player->pendingweapon = wp_chainsaw;
	}
	else if (player->weaponowned[wp_missile]
		 && player->ammo[am_misl])
	{
	    player->pendingweapon = wp_missile;
	}
	else if (player->weaponowned[wp_bfg]
		 && player->ammo[am_cell]>=bfg_cells
		 && (gamemode != shareware) )
	{
	    player->pendingweapon = wp_bfg;
	}
	else
	{
	    // If everything fails.
	    player->pendingweapon = wp_fist;
	}

    } while (player->pendingweapon == wp_nochange);

    // Now set appropriate weapon overlay.
    P_SetPsprite (player,
		  &player->psprites[ps_weapon],
		  (statenum_t) weaponinfo[player->readyweapon].downstate);

    return false;
}

//-----------------------------------------------------------------------------

static void A_DecrementAmmo (player_t* player, int amount)
{
    int newamount;
    ammotype_t ammo;

    ammo = weaponinfo[player->readyweapon].ammo;
    if (ammo < NUMAMMO)
    {
      newamount = player->ammo[ammo] - amount;
      if (newamount < 0)
        newamount = 0;
      player->ammo[ammo] = newamount;
    }
}

//-----------------------------------------------------------------------------
//
// P_FireWeapon.
//
static void P_FireWeapon (player_t* player)
{
    statenum_t	newstate;

    if (!P_CheckAmmo (player))
	return;

    P_SetMobjState (player->mo, S_PLAY_ATK1);
    newstate = (statenum_t) weaponinfo[player->readyweapon].atkstate;
    P_SetPsprite (player, &player->psprites[ps_weapon], newstate);
    P_NoiseAlert (player->mo, player->mo);
}

//-----------------------------------------------------------------------------
//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon (player_t* player)
{
    P_SetPsprite (player,
		  &player->psprites[ps_weapon],
		  (statenum_t) weaponinfo[player->readyweapon].downstate);
}

//-----------------------------------------------------------------------------
//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady (mobj_t* mo, pspdef_t* psp)
{
    statenum_t	newstate;
    int		angle;
    player_t*	player;

    // get out of attack state
    if (mo->state == &states[S_PLAY_ATK1]
	|| mo->state == &states[S_PLAY_ATK2] )
    {
	P_SetMobjState (mo, S_PLAY);
    }

    if ((player = mo->player) == NULL)
      return;

    if (player->readyweapon == wp_chainsaw
	&& psp
	&& psp->state == &states[S_SAW])
    {
	S_StartSound (mo, sfx_sawidl);
    }

    // check for change
    //  if player is dead, put the weapon away
    if (player->pendingweapon != wp_nochange || !player->health)
    {
	// change weapon
	//  (pending weapon should allready be validated)
	newstate = (statenum_t) weaponinfo[player->readyweapon].downstate;
	P_SetPsprite (player, &player->psprites[ps_weapon], newstate);
	return;
    }

    // check for fire
    //  the missile launcher and bfg do not auto fire
    if (player->cmd.buttons & BT_ATTACK)
    {
	if ( !player->attackdown
	     || (player->readyweapon != wp_missile
		 && player->readyweapon != wp_bfg) )
	{
	    player->attackdown = true;
	    P_FireWeapon (player);
	    return;
	}
    }
    else
	player->attackdown = false;

    if (psp)
    {
      // bob the weapon based on movement speed
      angle = (128*leveltime)&FINEMASK;
      psp->sx = FRACUNIT + FixedMul (player->bob, finecosine[angle]);
      angle &= FINEANGLES/2-1;
      psp->sy = weapon_top () + FixedMul (player->bob, finesine[angle]);
    }
}

//-----------------------------------------------------------------------------
//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire (mobj_t* mo, pspdef_t* psp)
{
    player_t*	player;

    if ((player = mo->player) == NULL)
      return;

    // check for fire
    //  (if a weaponchange is pending, let it go through instead)
    if ( (player->cmd.buttons & BT_ATTACK)
	 && player->pendingweapon == wp_nochange
	 && player->health)
    {
	player->refire++;
	P_FireWeapon (player);
    }
    else
    {
	player->refire = 0;
	P_CheckAmmo (player);
    }
}

//-----------------------------------------------------------------------------

void A_CheckReload (mobj_t* mo, pspdef_t* psp)
{
    player_t*	player;

    if ((player = mo->player) == NULL)
      return;

    P_CheckAmmo (player);
#if 0
    if (player->ammo[am_shell]<2)
	P_SetPsprite (player, &player->psprites[ps_weapon], S_DSNR1);
#endif
}

//-----------------------------------------------------------------------------
//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower (mobj_t* mo, pspdef_t* psp)
{
    fixed_t wb;
    player_t*	player;

    if (((player = mo->player) == NULL)
     || (psp == NULL))
      return;

    psp->sy += LOWERSPEED;
    wb = weapon_bottom ();

    // Is already down.
    if (psp->sy < wb )
	return;

    // Player is dead.
    if (player->playerstate == PST_DEAD)
    {
	psp->sy = wb;

	// don't bring weapon back up
	return;
    }

    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if (!player->health)
    {
	// Player is dead, so keep the weapon off screen.
	P_SetPsprite (player, &player->psprites[ps_weapon], S_NULL);
	return;
    }

    player->readyweapon = player->pendingweapon;

    P_BringUpWeapon (player);
}

//-----------------------------------------------------------------------------
//
// A_Raise
//
void A_Raise (mobj_t* mo, pspdef_t* psp)
{
    fixed_t wt;
    statenum_t	newstate;
    player_t*	player;

    if (((player = mo->player) == NULL)
     || (psp == NULL))
      return;

    wt = weapon_top ();
    psp->sy -= RAISESPEED;

    if (psp->sy > wt )
	return;

    psp->sy = wt;

    // The weapon has been raised all the way,
    //  so change to the ready state.
    newstate = (statenum_t) weaponinfo[player->readyweapon].readystate;

    P_SetPsprite (player, &player->psprites[ps_weapon], newstate);
}

//-----------------------------------------------------------------------------
// Weapons now recoil, amount depending on the weapon. // phares
// // |
// The P_SetPsprite call in each of the weapon firing routines // V
// was moved here so the recoil could be synced with the
// muzzle flash, rather than the pressing of the trigger.
// The BFG delay caused this to be necessary.

static void A_FireSomething (player_t* player, unsigned int adder)
{
  fixed_t move;

  P_SetPsprite (player, &player->psprites[ps_flash], (statenum_t) (weaponinfo[player->readyweapon].flashstate + adder));

  if ((weaponrecoil)
   && (!netgame)
   && ((move = recoil [player->readyweapon]) != 0))
    P_Thrust (player, ANG180 + player->mo->angle, 2048 * move);
}

//-----------------------------------------------------------------------------
//
// A_GunFlash
//
void A_GunFlash (mobj_t* mo, pspdef_t* psp)
{
  player_t*	player;

  P_SetMobjState (mo, S_PLAY_ATK2);

  if ((player = mo->player) != NULL)
    A_FireSomething (player, 0);
}

//-----------------------------------------------------------------------------
//
// WEAPON ATTACKS
//


//
// A_Punch
//
void A_Punch (mobj_t* mo, pspdef_t* psp)
{
    angle_t	angle;
    int		damage;
    int		slope;
    int		t;
    player_t*	player;

    damage = (P_Random ()%10+1)<<1;

    if (((player = mo->player) != NULL)
     && (player->powers[pw_strength]))
	damage *= 10;

    angle = mo->angle;
    t = P_Random();	/* remove dependence on order of evaluation */
    angle += (t-P_Random())<<18;
    slope = P_AimLineAttack (mo, angle, MELEERANGE, MF_FRIEND);
    if (!linetarget)
      slope = P_AimLineAttack(mo, angle, MELEERANGE, 0);
    P_LineAttack (mo, angle, MELEERANGE, slope, damage);

    // turn to face target
    if (linetarget)
    {
	S_StartSound (mo, sfx_punch);
	mo->angle = R_PointToAngle2 (mo->x,
					mo->y,
					linetarget->x,
					linetarget->y);
    }
}

//-----------------------------------------------------------------------------
//
// A_Saw
//
void A_Saw (mobj_t* mo, pspdef_t* psp)
{
    angle_t	angle;
    int		damage;
    int		slope;
    int		t;

    damage = 2*(P_Random ()%10+1);
    angle = mo->angle;
    t = P_Random();	/* remove dependence on order of evaluation */
    angle += (t-P_Random())<<18;

    // use meleerange + 1 se the puff doesn't skip the flash
    slope = P_AimLineAttack (mo, angle, MELEERANGE+1, MF_FRIEND);
    if (!linetarget)
      slope = P_AimLineAttack(mo, angle, MELEERANGE+1, 0);
    P_LineAttack (mo, angle, MELEERANGE+1, slope, damage);

    if (!linetarget)
    {
	S_StartSound (mo, sfx_sawful);
	return;
    }
    S_StartSound (mo, sfx_sawhit);

    // turn to face target
    angle = R_PointToAngle2 (mo->x, mo->y,
			     linetarget->x, linetarget->y);
    if (angle - mo->angle > ANG180)
    {
	if (angle - mo->angle < -ANG90/20)
	    mo->angle = angle + ANG90/21;
	else
	    mo->angle -= ANG90/20;
    }
    else
    {
	if (angle - mo->angle > ANG90/20)
	    mo->angle = angle - ANG90/21;
	else
	    mo->angle += ANG90/20;
    }
    mo->flags |= MF_JUSTATTACKED;
}

//-----------------------------------------------------------------------------
//
// A_FireMissile
//
void A_FireMissile (mobj_t* mo, pspdef_t* psp)
{
  player_t*	player;

  if ((player = mo->player) != NULL)
    A_DecrementAmmo (player, 1);

  P_SpawnPlayerMissile (mo, MT_ROCKET);
}

//-----------------------------------------------------------------------------
//
// A_FireBFG
//
void A_FireBFG (mobj_t* mo, pspdef_t* psp)
{
  player_t*	player;

  if ((player = mo->player) != NULL)
    A_DecrementAmmo (player, bfg_cells);

  P_SpawnPlayerMissile (mo, MT_BFG);
}

//-----------------------------------------------------------------------------
//
// A_FireOldBFG
//
// This function emulates Doom's Pre-Beta BFG
// By Lee Killough 6/6/98, 7/11/98, 7/19/98, 8/20/98
//

void A_FireOldBFG (mobj_t* mo, pspdef_t* psp)
{
  player_t *player;
  mobjtype_t type = MT_PLASMA1;


  if ((player = mo->player) == NULL)
    return;

  if (weaponrecoil && !(mo->flags & MF_NOCLIP))
    P_Thrust(player, ANG180 + mo->angle,
	     512*recoil[wp_plasma]);

  A_DecrementAmmo (player, 1);

  player->extralight = 2;

  do
  {
    mobj_t *th;
    angle_t an = mo->angle;
    angle_t an1 = ((P_Random()&127) - 64) * (ANG90/768) + an;
    angle_t an2 = ((P_Random()&127) - 64) * (ANG90/640) + ANG90;
//  extern int autoaim;

//  if (autoaim)
    {
      // killough 8/2/98: make autoaiming prefer enemies
      int mask = MF_FRIEND;
      fixed_t slope;
      do
      {
	slope = P_AimLineAttack(mo, an, 16*64*FRACUNIT, mask);
	if (!linetarget)
	  slope = P_AimLineAttack(mo, an += 1<<26, 16*64*FRACUNIT, mask);
	if (!linetarget)
	  slope = P_AimLineAttack(mo, an -= 2<<26, 16*64*FRACUNIT, mask);
	if (!linetarget)
	  slope = 0, an = mo->angle;
      } while (mask && (mask=0, !linetarget));	// killough 8/2/98
      an1 += an - mo->angle;
      an2 += tantoangle[slope >> DBITS];
    }

    th = P_SpawnMobj(mo->x, mo->y,
		     mo->z + 62*FRACUNIT - player->psprites[ps_weapon].sy,
		     type);
    th->target = mo;
    th->angle = an1;
    th->momx = finecosine[an1>>ANGLETOFINESHIFT] * 25;
    th->momy = finesine[an1>>ANGLETOFINESHIFT] * 25;
    th->momz = finetangent[an2>>ANGLETOFINESHIFT] * 25;
    P_CheckMissileSpawn(th);
    if (type == MT_PLASMA2)
      break;
    type = MT_PLASMA2;
  } while (1);
}

//-----------------------------------------------------------------------------
//
// A_FirePlasma
//
void A_FirePlasma (mobj_t* mo, pspdef_t* psp)
{
  player_t*	player;

  if ((player = mo->player) != NULL)
  {
    A_DecrementAmmo (player, 1);
    A_FireSomething (player, (P_Random ()&1));
  }

  P_SpawnPlayerMissile (mo, MT_PLASMA);
}

//-----------------------------------------------------------------------------
//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//

static fixed_t P_BulletSlope (mobj_t* mo)
{
  int		mask;
  angle_t	an;
  fixed_t	bulletslope;

  // see which target is to be aimed at
  an = mo->angle;

  // killough 8/2/98: make autoaiming prefer enemies
  mask = MF_FRIEND;

  do
  {
    bulletslope = P_AimLineAttack(mo, an, 16*64*FRACUNIT, mask);
    if (!linetarget)
      bulletslope = P_AimLineAttack(mo, an += 1<<26, 16*64*FRACUNIT, mask);
    if (!linetarget)
      bulletslope = P_AimLineAttack(mo, an -= 2<<26, 16*64*FRACUNIT, mask);
  } while (mask && (mask=0, !linetarget));	// killough 8/2/98

  return (bulletslope);
}

//-----------------------------------------------------------------------------
//
// P_GunShot
//
static void P_GunShot (mobj_t* mo, boolean accurate, fixed_t bulletslope)
{
    angle_t	angle;
    int		damage;

    damage = 5*(P_Random ()%3+1);
    angle = mo->angle;

    if (!accurate)
    {
        int t = P_Random();	/* remove dependence on order of evaluation */
	angle += (t-P_Random())<<18;
    }

    P_LineAttack (mo, angle, MISSILERANGE, bulletslope, damage);
}

//-----------------------------------------------------------------------------
//
// A_FirePistol
//
void A_FirePistol (mobj_t* mo, pspdef_t* psp)
{
  player_t* player;
  fixed_t   bulletslope;

  S_StartSound (mo, sfx_pistol);
  P_SetMobjState (mo, S_PLAY_ATK2);

  if ((player = mo->player) != NULL)
  {
    A_DecrementAmmo (player, 1);
    A_FireSomething (player, 0);
    bulletslope = P_BulletSlope (mo);
    P_GunShot (mo, (boolean) (!player->refire), bulletslope);
  }
}

//-----------------------------------------------------------------------------
//
// A_FireShotgun
//
void A_FireShotgun (mobj_t* mo, pspdef_t* psp)
{
  int i;
  player_t* player;
  fixed_t   bulletslope;

  S_StartSound (mo, sfx_shotgn);
  P_SetMobjState (mo, S_PLAY_ATK2);

  if ((player = mo->player) != NULL)
  {
    A_DecrementAmmo (player, 1);
    A_FireSomething (player, 0);
  }

  bulletslope = P_BulletSlope (mo);

  for (i=0 ; i<7 ; i++)
    P_GunShot (mo, false, bulletslope);
}

//-----------------------------------------------------------------------------
//
// A_FireShotgun2
//
void A_FireShotgun2 (mobj_t* mo, pspdef_t* psp)
{
  int	i;
  angle_t angle;
  int	damage;
  player_t* player;
  fixed_t   bulletslope;

  S_StartSound (mo, sfx_dshtgn);
  P_SetMobjState (mo, S_PLAY_ATK2);

  if ((player = mo->player) != NULL)
  {
    A_DecrementAmmo (player, 2);
    A_FireSomething (player, 0);
  }

  bulletslope = P_BulletSlope (mo);

  for (i=0 ; i<20 ; i++)
  {
      int t;
      damage = 5*(P_Random ()%3+1);
      angle = mo->angle;
      t = P_Random();	/* remove dependence on order of evaluation */
      angle += (t-P_Random())<<19;
      t = P_Random();	/* remove dependence on order of evaluation */
      P_LineAttack (mo,
		    angle,
		    MISSILERANGE,
		    bulletslope + ((t-P_Random())<<5), damage);
  }
}

//-----------------------------------------------------------------------------
//
// A_FireCGun
//
void A_FireCGun (mobj_t* mo, pspdef_t* psp)
{
  player_t* player;
  fixed_t   bulletslope;

  if (((player = mo->player) == NULL)
   || (psp == NULL))
    return;

  if (!player->ammo[weaponinfo[player->readyweapon].ammo])
      return;

  S_StartSound (mo, sfx_pistol);
  P_SetMobjState (mo, S_PLAY_ATK2);
  A_DecrementAmmo (player, 1);
  A_FireSomething (player, (psp->state - &states[S_CHAIN1]) & 1);
  bulletslope = P_BulletSlope (mo);
  P_GunShot (mo, (boolean)(!player->refire), bulletslope);
}

//-----------------------------------------------------------------------------
//
// ?
//
void A_Light0 (mobj_t* mo, pspdef_t* psp)
{
  player_t* player;
  if ((player = mo->player) != NULL)
    player->extralight = 0;
}

void A_Light1 (mobj_t* mo, pspdef_t* psp)
{
  player_t* player;
  if ((player = mo->player) != NULL)
    player->extralight = 1;
}

void A_Light2 (mobj_t* mo, pspdef_t* psp)
{
  player_t* player;
  if ((player = mo->player) != NULL)
    player->extralight = 2;
}

//-----------------------------------------------------------------------------
//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray (mobj_t* mo, pspdef_t* psp)
{
    int			i;
    int			j;
    int			damage;
    angle_t		an;

    // offset angles from its attack angle
    for (i=0 ; i<40 ; i++)
    {
	an = mo->angle - ANG90/2 + ANG90/40*i;

	// mo->target is the originator (player)
	//  of the missile
	P_AimLineAttack (mo->target, an, 16*64*FRACUNIT, MF_FRIEND);
	if (!linetarget)
	  P_AimLineAttack(mo->target, an, 16*64*FRACUNIT, 0);

	if (!linetarget)
	    continue;

	P_SpawnMobj (linetarget->x,
		     linetarget->y,
		     linetarget->z + (linetarget->height>>2),
		     MT_EXTRABFG);

	damage = 0;
	for (j=0;j<15;j++)
	    damage += (P_Random()&7) + 1;

	P_DamageMobj (linetarget, mo->target,mo->target, damage);
    }
}

//-----------------------------------------------------------------------------
//
// A_BFGsound
//
void A_BFGsound (mobj_t* mo, pspdef_t* psp)
{
    S_StartSound (mo, sfx_bfg);
}

//-----------------------------------------------------------------------------
//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites (player_t* player)
{
    int	i;

    // remove all psprites
    for (i=0 ; i<NUMPSPRITES ; i++)
	player->psprites[i].state = NULL;

    // spawn the gun
    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon (player);
}

//-----------------------------------------------------------------------------
typedef struct
{
  state_t*	state;
  int		tics;
} pspcdef_t;

//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites (player_t* player)
{
  int		i;
  int		tics;
  pspdef_t*	psp;
  pspcdef_t*	cpsp;
  state_t*	state;
  pspcdef_t	pspcopy [NUMPSPRITES-1];

  /* We need to avoid the case where the first call to P_SetPsprite */
  /* changes the flash sprite and then we immediatly decrement the tic. */
  /* On BlackOps.wad the tic is set to 1 and it is cleared before we */
  /* can see it. */

  psp = &player->psprites[1];
  cpsp = pspcopy;
  for (i=1 ; i<NUMPSPRITES ; i++, psp++, cpsp++)
  {
    cpsp->state = psp->state;
    cpsp->tics  = psp->tics;
  }

  psp = player->psprites;

  if (((state = psp->state) != NULL)	// a null state means not active
   && ((tics = psp->tics) != -1)	// a -1 tic count never changes
   && ((psp->tics = --tics) == 0))	// drop tic count and possibly change state
    P_SetPsprite (player, psp, state->nextstate);

  cpsp = pspcopy;

  for (i=1 ; i<NUMPSPRITES ; i++, cpsp++)
  {
    psp++;
    if (((state = psp->state) != NULL)
     && ((tics = psp->tics) != -1)
     && ((tics > 2)
      || ((state == cpsp->state) && (tics == cpsp->tics)))
     && ((psp->tics = --tics) == 0))
      P_SetPsprite (player, psp, state->nextstate);
  }

  player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
  player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}

//-----------------------------------------------------------------------------
