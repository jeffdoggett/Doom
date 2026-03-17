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
//	Items: key cards, artifacts, weapon, ammunition.
//
//-----------------------------------------------------------------------------


#ifndef __D_ITEMS__
#define __D_ITEMS__

#include "doomdef.h"

#ifdef __GNUG__
#pragma interface
#endif

//
// MBF21: haleyjd 09/11/07: weapon flags
//
enum
{
    WPF_NOFLAG         = 0x00000000,    // no flag
    WPF_NOTHRUST       = 0x00000001,    // doesn't thrust Mobj's
    WPF_SILENT         = 0x00000002,    // weapon is silent
    WPF_NOAUTOFIRE     = 0x00000004,    // weapon won't autofire in A_WeaponReady
    WPF_FLEEMELEE      = 0x00000008,    // monsters consider it a melee weapon
    WPF_AUTOSWITCHFROM = 0x00000010,    // can be switched away from when ammo is picked up
    WPF_NOAUTOSWITCHTO = 0x00000020     // cannot be switched to when ammo is picked up
};


// Weapon info: sprite frames, ammunition use.
typedef struct
{
    ammotype_t	ammo;
    int		upstate;
    int		downstate;
    int		readystate;
    int		atkstate;
    int		flashstate;
    int		ammopershot;
    int		mbf21bits;
} weaponinfo_t;

extern  weaponinfo_t    weaponinfo[NUMWEAPONS];

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
