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
//	Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __P_MOBJ__
#define __P_MOBJ__

// Basics.
#include "tables.h"
#include "m_fixed.h"

// We need the thinker_t stuff.
#include "d_think.h"

// We need the WAD data structure for Map things,
// from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are
//  tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"



#ifdef __GNUG__
#pragma interface
#endif


// killough 11/98:
// Whether an object is "sentient" or not. Used for environmental influences.
#define sentient(mobj)      (mobj->health > 0 && mobj->info->seestate)

//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost allways set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
    // Call P_SpecialThing when touched.
    M_SPECIAL,
    // Blocks.
    M_SOLID,
    // Can be hit.
    M_SHOOTABLE,
    // Don't use the sector links (invisible but touchable).
    M_NOSECTOR,
    // Don't use the blocklinks (inert but displayable)
    M_NOBLOCKMAP,

    // Not to be activated by sound, deaf monster.
    M_AMBUSH,
    // Will try to attack right back.
    M_JUSTHIT,
    // Will take at least one step before attacking.
    M_JUSTATTACKED,
    // On level spawning (initial position),
    //  hang from ceiling instead of stand on floor.
    M_SPAWNCEILING,
    // Don't apply gravity (every tic),
    //  that is, object will float, keeping current height
    //  or changing it actively.
    M_NOGRAVITY,

    // Movement flags.
    // This allows jumps from high places.
    M_DROPOFF,
    // For players, will pick up items.
    M_PICKUP,
    // Player cheat. ???
    M_NOCLIP,
    // Player: keep info about sliding along walls.
    M_SLIDE,
    // Allow moves to any height, no gravity.
    // For active floaters, e.g. cacodemons, pain elementals.
    M_FLOAT,
    // Don't cross lines
    //   ??? or look at heights on teleport.
    M_TELEPORT,
    // Don't hit same species, explode on block.
    // Player missiles as well as fireballs of various kinds.
    M_MISSILE,
    // Dropped by a demon, not level spawned.
    // E.g. ammo clips dropped by dying former humans.
    M_DROPPED,
    // Use fuzzy draw (shadow demons or spectres),
    //  temporary player invisibility powerup.
    M_SHADOW,
    // Flag: don't bleed when shot (use puff),
    //  barrels and shootable furniture shall not bleed.
    M_NOBLOOD,
    // Don't stop moving halfway off a step,
    //  that is, have dead bodies slide down all the way.
    M_CORPSE,
    // Floating to a height for a move, ???
    //  don't auto float to target's height.
    M_INFLOAT,

    // On kill, count this enemy object
    //  towards intermission kill total.
    // Happy gathering.
    M_COUNTKILL,

    // On picking up, count this item object
    //  towards intermission item total.
    M_COUNTITEM,

    // Special handling: skull in flight.
    // Neither a cacodemon nor a missile.
    M_SKULLFLY,

    // Don't spawn this object
    //  in death match mode (e.g. key cards).
    M_NOTDMATCH,

    // Player sprites in multiplayer modes are modified
    //  using an internal color lookup table for re-indexing.
    // If 0x4 0x8 or 0xc,
    //  use a translation table for player colormaps
    M_TRANSLATION_1,
    M_TRANSLATION_2,

    M_TOUCHY,		// killough 11/98: dies when solids touch it
    M_BOUNCES,		// killough 7/11/98: for beta BFG fireballs
    M_FRIEND,		// killough 7/18/98: friendly monsters

    M_TRANSLUCENT	// apply translucency to sprite (BOOM)
} mobjflagnum_t;



typedef enum
{
    MF_SPECIAL		= (1U << M_SPECIAL),
    MF_SOLID		= (1U << M_SOLID),
    MF_SHOOTABLE	= (1U << M_SHOOTABLE),
    MF_NOSECTOR		= (1U << M_NOSECTOR),
    MF_NOBLOCKMAP	= (1U << M_NOBLOCKMAP),
    MF_AMBUSH		= (1U << M_AMBUSH),
    MF_JUSTHIT		= (1U << M_JUSTHIT),
    MF_JUSTATTACKED	= (1U << M_JUSTATTACKED),
    MF_SPAWNCEILING	= (1U << M_SPAWNCEILING),
    MF_NOGRAVITY	= (1U << M_NOGRAVITY),
    MF_DROPOFF		= (1U << M_DROPOFF),
    MF_PICKUP		= (1U << M_PICKUP),
    MF_NOCLIP		= (1U << M_NOCLIP),
    MF_SLIDE		= (1U << M_SLIDE),
    MF_FLOAT		= (1U << M_FLOAT),
    MF_TELEPORT		= (1U << M_TELEPORT),
    MF_MISSILE		= (1U << M_MISSILE),
    MF_DROPPED		= (1U << M_DROPPED),
    MF_SHADOW		= (1U << M_SHADOW),
    MF_NOBLOOD		= (1U << M_NOBLOOD),
    MF_CORPSE		= (1U << M_CORPSE),
    MF_INFLOAT		= (1U << M_INFLOAT),
    MF_COUNTKILL	= (1U << M_COUNTKILL),
    MF_COUNTITEM	= (1U << M_COUNTITEM),
    MF_SKULLFLY		= (1U << M_SKULLFLY),
    MF_NOTDMATCH    	= (1U << M_NOTDMATCH),
    MF_TRANSLATION  	= (1U << M_TRANSLATION_1) | (1U << M_TRANSLATION_2),
    MF_TRANSSHIFT	= M_TRANSLATION_1,
    MF_TOUCHY		= (1U << M_TOUCHY),
    MF_BOUNCES		= (1U << M_BOUNCES),
    MF_FRIEND		= (1U << M_FRIEND),
/*  ARM Compiler generates - Error: signed constant overflow: 'enum' */
    MF_TRANSLUCENT	= (signed)(1U << M_TRANSLUCENT)
} mobjflag_t;

typedef enum
{
  M2_PASSMOBJ,
  M2_ONMOBJ,
  M2_MASSACRE		= 28
} mobjflag2num_t;

typedef enum
{
  MF2_PASSMOBJ		= (1U << M2_PASSMOBJ),
  MF2_ONMOBJ		= (1U << M2_ONMOBJ),
  MF2_MASSACRE		= (1U << M2_MASSACRE)
} mobjflag2_t;



typedef enum
{
  MB_LOGRAV,		// 	Lower gravity (1/8)
  MB_SHORTMRANGE,	// 	Short missile range (archvile)
  MB_DMGIGNORED,	// 	Other things ignore its attacks (archvile)
  MB_NORADIUSDMG,	// 	Doesn't take splash damage (cyberdemon, mastermind)
  MB_FORCERADIUSDMG,	// 	Thing causes splash damage even if the target shouldn't
  MB_HIGHERMPROB,	// 	Higher missile attack probability (cyberdemon)
  MB_RANGEHALF,		// 	Use half distance for missile attack probability (cyberdemon, mastermind, revenant, lost soul)
  MB_NOTHRESHOLD,	// 	Has no targeting threshold (archvile)
  MB_LONGMELEE,		// 	Has long melee range (revenant)
  MB_BOSS,		// 	Full volume see / death sound & splash immunity (from heretic)
  MB_MAP07BOSS1,	// 	Tag 666 "boss" on doom 2 map 7 (mancubus)
  MB_MAP07BOSS2,	// 	Tag 667 "boss" on doom 2 map 7 (arachnotron)
  MB_E1M8BOSS,		// 	E1M8 boss (baron)
  MB_E2M8BOSS,		// 	E2M8 boss (cyberdemon)
  MB_E3M8BOSS,		// 	E3M8 boss (mastermind)
  MB_E4M6BOSS,		// 	E4M6 boss (cyberdemon)
  MB_E4M8BOSS,		// 	E4M8 boss (mastermind)
  MB_RIP,		// 	Ripper projectile (does not disappear on impact)
  MB_FULLVOLSOUNDS	// 	Full volume see / death sounds (cyberdemon, mastermind)
} mobjflagmbf21num_t;

typedef enum
{
  MBF_LOGRAV		= (1 <<	MB_LOGRAV),
  MBF_SHORTMRANGE	= (1 <<	MB_SHORTMRANGE),
  MBF_DMGIGNORED	= (1 <<	MB_DMGIGNORED),
  MBF_NORADIUSDMG	= (1 <<	MB_NORADIUSDMG),
  MBF_FORCERADIUSDMG	= (1 <<	MB_FORCERADIUSDMG),
  MBF_HIGHERMPROB	= (1 <<	MB_HIGHERMPROB),
  MBF_RANGEHALF		= (1 <<	MB_RANGEHALF),
  MBF_NOTHRESHOLD	= (1 <<	MB_NOTHRESHOLD),
  MBF_LONGMELEE		= (1 <<	MB_LONGMELEE),
  MBF_BOSS		= (1 <<	MB_BOSS),
  MBF_MAP07BOSS1	= (1 <<	MB_MAP07BOSS1),
  MBF_MAP07BOSS2	= (1 <<	MB_MAP07BOSS2),
  MBF_E1M8BOSS		= (1 <<	MB_E1M8BOSS),
  MBF_E2M8BOSS		= (1 <<	MB_E2M8BOSS),
  MBF_E3M8BOSS		= (1 <<	MB_E3M8BOSS),
  MBF_E4M6BOSS		= (1 <<	MB_E4M6BOSS),
  MBF_E4M8BOSS		= (1 <<	MB_E4M8BOSS),
  MBF_RIP		= (1 <<	MB_RIP),
  MBF_FULLVOLSOUNDS	= (1 <<	MB_FULLVOLSOUNDS),
} mobjflagmbf21_t;

// Map Object definition.
typedef struct mobj_s
{
    // List: thinker links.
    thinker_t		thinker;

    // Info for drawing: position.
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

    // More list: links in sector (if needed)
    struct mobj_s*	snext;
    struct mobj_s*	sprev;

    //More drawing info: to determine current sprite.
    angle_t		angle;	// orientation
    spritenum_t		sprite;	// used to find patch_t and flip value
    int			frame;	// might be ORed with FF_FULLBRIGHT

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
    struct mobj_s*	bnext;
    struct mobj_s*	bprev;

    struct subsector_s*	subsector;

    // The closest interval over all contacted Sectors.
    fixed_t		floorz;
    fixed_t		ceilingz;

    // For movement checking.
    fixed_t		radius;
    fixed_t		height;

    // Momentums, used to update position.
    fixed_t		momx;
    fixed_t		momy;
    fixed_t		momz;

    mobjtype_t		type;
    mobjinfo_t*		info;	// &mobjinfo[mobj->type]

    int			tics;	// state tic counter
    state_t*		state;
    int			flags;
    int			flags2;
    int			health;

    // Movement direction, movement generation (zig-zagging).
    int			movedir;	// 0-7
    int			movecount;	// when 0, select a new dir

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
    struct mobj_s*	target;

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int			reactiontime;

    // If >0, the target will be chased
    // no matter what (even if shot)
    int			threshold;

    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
    struct player_s*	player;

    // Player number last looked for.
    int			lastlook;

    // For nightmare respawn.
    mapthing_t		spawnpoint;

    // Thing being chased/attacked for tracers.
    struct mobj_s*	tracer;

    // a linked list of sectors where this object appears
    struct msecnode_s   *touching_sectorlist;   // phares 3/14/98
} mobj_t;


#define STOPSPEED	0x1000
#define WATERFRICTION   0xD500

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
