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
//  Internally used data structures for virtually everything,
//   key definitions, lots of other stuff.
//
//-----------------------------------------------------------------------------

#ifndef __DOOMDEF__
#define __DOOMDEF__

#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define USE_BOOM_P_ChangeSector

//
// Global parameters/defines.
//
// DOOM version
enum { SAVE_GAME_VERSION = 110 };	// Will never change!!!
enum { DEMO_VERSION = 110 };
enum { NET_PROTOCOL_VERSION = 110 };
enum { GAME_ENGINE_VERSION = 111 };


// Game mode handling - identify IWAD version
//  to handle IWAD dependend animations etc.
typedef enum
{
  shareware,	// DOOM 1 shareware, E1, M9
  registered,	// DOOM 1 registered, E3, M27
  commercial,	// DOOM 2 retail, E1 M34
  // DOOM 2 german edition not handled
  retail,	// DOOM 1 retail, E4, M36
  indetermined	// Well, no IWAD found.

} GameMode_t;


// Mission packs - might be useful for TC stuff?
typedef enum
{
  doom,		// DOOM 1
  doom2,	// DOOM 2
  pack_tnt,	// TNT mission pack
  pack_plut,	// Plutonia pack
  pack_hacx,
  none

} GameMission_t;


// Identify language to use, software localization.
typedef enum
{
  english,
  french,
  german,
  unknown

} Language_t;


// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
#define RANGECHECK

// Do or do not use external soundserver.
// The sndserver binary to be run separately
//  has been introduced by Dave Taylor.
// The integrated sound support is experimental,
//  and unfinished. Default is synchronous.
// Experimental asynchronous timer based is
//  handled by SNDINTR.
#define SNDSERV  1
//#define SNDINTR  1


// This one switches between MIT SHM (no proper mouse)
// and XFree86 DGA (mickey sampling). The original
// linuxdoom used SHM, which is default.
//#define X11_DGA		1


//
// For resize of screen, at start of game.
// It will not work dynamically, see visplanes.
//
//#define	BASE_WIDTH		320

// It is educational but futile to change this
//  scaling e.g. to 2. Drawing of status bar,
//  menues etc. is tied to the scale implied
//  by the graphics.
#define	SCREEN_MUL		1
#define	INV_ASPECT_RATIO	0.625 // 0.75, ideally

// Defines suck. C sucks.
// C++ might sucks for OOP, but it sure is a better C.
// So there.
//(int)(SCREEN_MUL*BASE_WIDTH*INV_ASPECT_RATIO) //200

extern unsigned int SCREENWIDTH;
extern unsigned int SCREENHEIGHT;

extern unsigned int ORIGSCREENWIDTH;	// 320 in original Doom
extern unsigned int ORIGSCREENHEIGHT;	// 200


// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS		4

// State updates, number of tics / second.
#define TICRATE		35

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo.
typedef enum
{
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN
} gamestate_t;

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define	MTF_EASY		1
#define	MTF_NORMAL		2
#define	MTF_HARD		4

// Deaf monsters/do not react to sound.
#define	MTF_AMBUSH		8

// killough 11/98
#define MTF_NOTSINGLE		16
#define MTF_NOTDM		32
#define MTF_NOTCOOP		64
#define MTF_FRIEND		128


typedef enum
{
#ifdef BOOM
    sk_none=-1,
#endif
    sk_baby = 0,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare
} skill_t;




//
// Key cards.
//
typedef enum
{
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,
    NUMCARDS
} card_t;

typedef enum
{
  P_PD_BLUEO,
  P_PD_REDO,
  P_PD_YELLOWO,
  P_PD_BLUEK,
  P_PD_REDK,
  P_PD_YELLOWK,
  P_PD_BLUEC,
  P_PD_REDC,
  P_PD_YELLOWC,
  P_PD_BLUES,
  P_PD_REDS,
  P_PD_YELLOWS,
  P_PD_BLUECO,
  P_PD_REDCO,
  P_PD_YELLOWCO,
  P_PD_BLUESO,
  P_PD_REDSO,
  P_PD_YELLOWSO,
  P_PD_ANY,
  P_PD_ANYOBJ,
  P_PD_ALL3,
  P_PD_ALL3O,
  P_PD_ALL6,
  P_PD_ALL6O,
  P_PD_ALLKEYS,
  P_PD_QTY
} door_keys_t;


// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    NUMWEAPONS,

    // No pending weapon change.
    wp_nochange

} weapontype_t;


// Ammunition types defined.
typedef enum
{
    am_clip,	// Pistol / chaingun ammo.
    am_shell,	// Shotgun / double barreled shotgun.
    am_cell,	// Plasma rifle, BFG.
    am_misl,	// Missile launcher.
    NUMAMMO,
    am_noammo	// Unlimited for chainsaw / fist.

} ammotype_t;


// Power up artifacts.
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    NUMPOWERS

} powertype_t;



//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
    INVULNTICS	= (30*TICRATE),
    INVISTICS	= (60*TICRATE),
    INFRATICS	= (120*TICRATE),
    IRONTICS	= (60*TICRATE)

} powerduration_t;




//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (lowercased).
//
#define NUMKEYS		256
#define KEY_RIGHTARROW	0xae
#define KEY_LEFTARROW	0xac
#define KEY_UPARROW	0xad
#define KEY_DOWNARROW	0xaf
#define KEY_ESCAPE	27
#define KEY_ENTER	13
#define KEY_TAB		9
#define KEY_F1		(0x80+0x3b)
#define KEY_F2		(0x80+0x3c)
#define KEY_F3		(0x80+0x3d)
#define KEY_F4		(0x80+0x3e)
#define KEY_F5		(0x80+0x3f)
#define KEY_F6		(0x80+0x40)
#define KEY_F7		(0x80+0x41)
#define KEY_F8		(0x80+0x42)
#define KEY_F9		(0x80+0x43)
#define KEY_F10		(0x80+0x44)
#define KEY_F11		(0x80+0x57)
#define KEY_F12		(0x80+0x58)
#define KEY_PRINTSCRN	(0x80+0x59)

#define KEY_BACKSPACE	127
#define KEY_PAUSE	0xff

#define KEY_EQUALS	0x3d
#define KEY_MINUS	0x2d
#define KEY_PLUS	0x2b

#define KEY_RSHIFT	(0x80+0x36)
#define KEY_RCTRL	(0x80+0x1d)
#define KEY_RALT	(0x80+0x38)

#define KEY_LSHIFT	KEY_RSHIFT
#define KEY_LCTRL	KEY_RCTRL
#define KEY_LALT	KEY_RALT



#ifdef BOOM

#define DEN_PLAYER5 4001
#define DEN_PLAYER6 4002
#define DEN_PLAYER7 4003
#define DEN_PLAYER8 4004

#define MORE_FRICTION_MOMENTUM 15000       /* mud factor based on momentum */
#define ORIG_FRICTION          0xE800      /* original value */
#define ORIG_FRICTION_FACTOR   2048        /* original value */

#endif

// DOOM basic types (boolean),
//  and max/min values.
//#include "doomtype.h"

// Fixed point.
//#include "m_fixed.h"

// Endianess handling.
//#include "m_swap.h"


// Binary Angles, sine/cosine/atan lookups.
//#include "tables.h"

// Event type.
//#include "d_event.h"

// Game function, skills.
//#include "g_game.h"

// All external data is defined here.
//#include "doomdata.h"

// All important printed strings.
// Language selection (message strings).
//#include "dstrings.h"

// Player is a special actor.
//struct player_s;


//#include "d_items.h"
//#include "d_player.h"
//#include "p_mobj.h"
//#include "d_net.h"

// PLAY
//#include "p_tick.h"




// Header, generated by sound utility.
// The utility was written by Dave Taylor.
//#include "sounds.h"




#endif          // __DOOMDEF__
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
