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
// DESCRIPTION:
//
//
//-----------------------------------------------------------------------------


#ifndef __P_ENEMY__
#define __P_ENEMY__


#ifdef __GNUG__
#pragma interface
#endif

typedef void (*actionf2)( void*, unsigned int);
typedef struct
{
  unsigned char episode;
  unsigned char map;
  unsigned int monsterbits;
  unsigned int tag;
  actionf2 func;
  unsigned int action;
} bossdeath_t;

extern void A_Light0 (player_t* player, pspdef_t* psp);
extern void A_WeaponReady (player_t* player, pspdef_t* psp);
extern void A_Lower (player_t* player, pspdef_t* psp);
extern void A_Raise (player_t* player, pspdef_t* psp);
extern void A_Punch (player_t* player, pspdef_t* psp);
extern void A_ReFire (player_t* player, pspdef_t* psp);
extern void A_FirePistol (player_t* player, pspdef_t* psp);
extern void A_Light1 (player_t* player, pspdef_t* psp);
extern void A_FireShotgun (player_t* player, pspdef_t* psp);
extern void A_Light2 (player_t* player, pspdef_t* psp);
extern void A_FireShotgun2 (player_t* player, pspdef_t* psp);
extern void A_CheckReload (player_t* player, pspdef_t* psp);
extern void A_OpenShotgun2 (player_t* player, pspdef_t* psp);
extern void A_LoadShotgun2 (player_t* player, pspdef_t* psp);
extern void A_CloseShotgun2 (player_t* player, pspdef_t* psp);
extern void A_FireCGun (player_t* player, pspdef_t* psp);
extern void A_GunFlash (player_t* player, pspdef_t* psp);
extern void A_FireMissile (player_t* player, pspdef_t* psp);
extern void A_Saw (player_t* player, pspdef_t* psp);
extern void A_FirePlasma (player_t* player, pspdef_t* psp);
extern void A_BFGsound (player_t* player, pspdef_t* psp);
extern void A_FireBFG (player_t* player, pspdef_t* psp);

extern void A_BFGSpray (mobj_t* mo);
extern void A_Explode (mobj_t* mo);
extern void A_Pain (mobj_t* mo);
extern void A_PlayerScream (mobj_t* mo);
extern void A_Fall (mobj_t* mo);
extern void A_XScream (mobj_t* mo);
extern void A_Look (mobj_t* mo);
extern void A_Chase (mobj_t* mo);
extern void A_FaceTarget (mobj_t* mo);
extern void A_PosAttack (mobj_t* mo);
extern void A_Scream (mobj_t* mo);
extern void A_SPosAttack (mobj_t* mo);
extern void A_VileChase (mobj_t* mo);
extern void A_VileStart (mobj_t* mo);
extern void A_VileTarget (mobj_t* mo);
extern void A_VileAttack (mobj_t* mo);
extern void A_StartFire (mobj_t* mo);
extern void A_Fire (mobj_t* mo);
extern void A_FireCrackle (mobj_t* mo);
extern void A_Tracer (mobj_t* mo);
extern void A_SkelWhoosh (mobj_t* mo);
extern void A_SkelFist (mobj_t* mo);
extern void A_SkelMissile (mobj_t* mo);
extern void A_FatRaise (mobj_t* mo);
extern void A_FatAttack1 (mobj_t* mo);
extern void A_FatAttack2 (mobj_t* mo);
extern void A_FatAttack3 (mobj_t* mo);
extern void A_BossDeath (mobj_t* mo);
extern void A_CPosAttack (mobj_t* mo);
extern void A_CPosRefire (mobj_t* mo);
extern void A_TroopAttack (mobj_t* mo);
extern void A_SargAttack (mobj_t* mo);
extern void A_HeadAttack (mobj_t* mo);
extern void A_BruisAttack (mobj_t* mo);
extern void A_SkullAttack (mobj_t* mo);
extern void A_Metal (mobj_t* mo);
extern void A_SpidRefire (mobj_t* mo);
extern void A_BabyMetal (mobj_t* mo);
extern void A_BspiAttack (mobj_t* mo);
extern void A_Hoof (mobj_t* mo);
extern void A_CyberAttack (mobj_t* mo);
extern void A_PainAttack (mobj_t* mo);
extern void A_PainDie (mobj_t* mo);
extern void A_KeenDie (mobj_t* mo);
extern void A_BrainPain (mobj_t* mo);
extern void A_BrainScream (mobj_t* mo);
extern void A_BrainDie (mobj_t* mo);
extern void A_BrainAwake (mobj_t* mo);
extern void A_BrainSpit (mobj_t* mo);
extern void A_SpawnSound (mobj_t* mo);
extern void A_SpawnFly (mobj_t* mo);
extern void A_BrainExplode (mobj_t* mo);
extern void A_Detonate (mobj_t* mo);
extern void A_Mushroom (mobj_t* mo);
extern void A_Die (mobj_t* mo);
extern void A_Spawn (mobj_t* mo);
extern void A_Turn (mobj_t* mo);
extern void A_Face (mobj_t* mo);
extern void A_Scratch (mobj_t* mo);
extern void A_PlaySound (mobj_t* mo);
extern void A_RandomJump (mobj_t* mo);
extern void A_LineEffect (mobj_t* mo);

extern void P_Massacre (void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
