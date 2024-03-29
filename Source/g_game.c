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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: g_game.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";
#endif

#include "includes.h"

/* -------------------------------------------------------------------------------------------- */

/* Variables that DeHackEd can alter. */

char * save_game_messages_orig [] =
{
  SAVEGAMENAME,
  GGSAVED,
  NULL
};

char * save_game_messages [ARRAY_SIZE(save_game_messages_orig)];

unsigned int Initial_Health  = 100;
unsigned int Initial_Bullets = 50;
boolean par_changed = false;
extern boolean dh_changing_pwad;
extern boolean Give_Max_Damage;
extern boolean Monsters_Infight1;
extern boolean Monsters_Infight2;
extern boolean Give_Max_Damage;
extern boolean nobrainspitcheat;

extern char * finale_backdrops [];
extern clusterdefs_t * finale_clusterdefs;

extern unsigned int mtf_mask;

/* -------------------------------------------------------------------------------------------- */

#define SAVESTRINGSIZE	24


boolean	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd (ticcmd_t* cmd);
void	G_WriteDemoTiccmd (ticcmd_t* cmd);
void	G_PlayerReborn (int player);
void	G_InitNew (skill_t skill, int episode, int map);

void	G_DoReborn (int playernum);
//void	G_InitPlayer (int player);
#define G_InitPlayer(p) G_PlayerReborn(p)
static void G_ResetPlayer (player_t* p, int flags);

void	G_DoLoadLevel (void);
void	G_DoNewGame (void);
void	G_DoLoadGame (void);
void	G_DoPlayDemo (void);
void	G_DoCompleted (void);
void	G_DoVictory (void);
void	G_DoWorldDone (void);
void	G_DoSaveGame (void);


gameaction_t	gameaction;
gamestate_t	gamestate;
skill_t		gameskill;
boolean		respawnmonsters;
int		gameepisode;
int		gamemap;
static int	newgametimer = 0;
int		gamecompletedtimer = 0;

boolean		paused;
static boolean	sendpause;		// send a pause event next tic
static boolean	sendsave;		// send a save event next tic
boolean		usergame;		// ok to save / end game

static boolean	timingdemo;		// if true, exit with report on completion
boolean		nodrawers;		// for comparative timing purposes
static boolean	noblit;			// for comparative timing purposes
static int	starttime;		// for comparative timing purposes

boolean		viewactive;

int		deathmatch;		// only if started as net death
boolean		netgame;		// only true if packets are broadcast
boolean		playeringame[MAXPLAYERS];
player_t	players[MAXPLAYERS];

int		consoleplayer;		// player taking events and displaying
int		displayplayer;		// view being displayed
int		gametic;
int		levelstarttic;		// gametic at level start
int		totalkills, totalitems, totalsecret;	// for intermission

static char	demoname[256];
boolean		demorecording;
boolean		demoplayback;
static boolean	netdemo;
static byte*	demobuffer;
static byte*	demo_p;
static byte*	demoend;
boolean		singledemo;		// quit after playing a demo from cmdline

boolean		precache = true;	// if true, load all graphics at start

wbstartstruct_t	wminfo;			// parms for world map / intermission

static short	consistancy[MAXPLAYERS][BACKUPTICS];

unsigned int	secretexit;
extern char*	pagename;

//
// controls (have defaults)
//
keyb_t		keyb;


#define MAXPLMOVE		(forwardmove[1])

#define TURBOTHRESHOLD	0x32

fixed_t		forwardmove[2] = {0x19, 0x32};
fixed_t		sidemove[2] = {0x18, 0x28};
static fixed_t	angleturn[3] = {640, 1280, 320};	// + slow turn

#define SLOWTURNTICS	6

boolean		gamekeydown[NUMKEYS];
static int	turnheld;				// for accelerative turning

static boolean	mousearray[4];
static boolean*	mousebuttons = &mousearray[1];		// allow [-1]

// mouse values are used once
static int	mousex;
static int	mousey;

static int	dclicktime;
static int	dclickstate;
static int	dclicks;
static int	dclicktime2;
static int	dclickstate2;
static int	dclicks2;

// joystick values are repeated
static int	joyxmove;
static int	joyymove;
static boolean	joyarray[5];
static boolean*	joybuttons = &joyarray[1];		// allow [-1]

static int	savegameslot;
static char	savedescription[32];


#define	BODYQUESIZE	32

static mobj_t*	bodyque[BODYQUESIZE];
int		bodyqueslot;

void*		statcopy;				// for statistics driver

/* -------------------------------------------------------------------------------------------- */

typedef struct map_patch_s
{
  struct map_patch_s * next;
  char patch [4];
} map_patch_t;

static map_patch_t * map_patches_head = NULL;

/* -------------------------------------------------------------------------------------------- */
/* Look up tables of level destinations */

const char lumpname_1 [] = "E%dM%d";
const char lumpname_2 [] = "MAP%02d";
const char cwilv [] = "CWILV%2.2d";
const char wilv [] = "WILV%d%d";
const char enterpic_1 [] = "WIMAP%d";
const char enterpic_2 [] = "INTERPIC";
const char borderpatch_1 [] = "FLOOR7_2";	// DOOM border patch.
const char borderpatch_2 [] = "GRNROCK";	// DOOM II border patch.


extramaps_t * extramaps_head = NULL;

/* -------------------------------------------------------------------------------------------- */

static map_dests_t next_level_tab_1 [QTY_EPISODES][QTY_MAPS_PER_EPISODE] =
{  // E  M  E  M  S  R  I  C  N  T   Par  Suk  skydelta Name Sky
  {
    { 1, 2, 1, 9, 0, 0, 0, 0, 0, 0,  30/5, 60, 0, HUSTR_E1M1, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M1
    { 1, 3, 1, 9, 0, 0, 0, 0, 0, 0,  75/5, 60, 0, HUSTR_E1M2, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M2
    { 1, 4, 1, 9, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_E1M3, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M3
    { 1, 5, 1, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E1M4, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M4
    { 1, 6, 1, 9, 0, 0, 0, 0, 0, 0, 165/5, 60, 0, HUSTR_E1M5, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M5
    { 1, 7, 1, 9, 0, 0, 0, 0, 0, 0, 180/5, 60, 0, HUSTR_E1M6, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M6
    { 1, 8, 1, 9, 0, 0, 0, 0, 0, 0, 180/5, 60, 0, HUSTR_E1M7, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M7
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 165/5, 60, 0, HUSTR_E1M8, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E1M8
    { 1, 4, 1, 4, 1, 0, 0, 0, 0, 0, 165/5, 60, 0, HUSTR_E1M9, (char*) lumpname_1, (char*) sky_1, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL }   // E1M9
  },
  {
    { 2, 2, 2, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E2M1, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M1
    { 2, 3, 2, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E2M2, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M2
    { 2, 4, 2, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E2M3, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M3
    { 2, 5, 2, 9, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_E2M4, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M4
    { 2, 6, 2, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E2M5, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M5
    { 2, 7, 2, 9, 0, 0, 0, 0, 0, 0, 360/5, 60, 0, HUSTR_E2M6, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M6
    { 2, 8, 2, 9, 0, 0, 0, 0, 0, 0, 240/5, 60, 0, HUSTR_E2M7, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M7
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 135/5, 60, 0, HUSTR_E2M8, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E2M8
    { 2, 6, 2, 6, 1, 0, 0, 0, 0, 0, 170/5, 60, 0, HUSTR_E2M9, (char*) lumpname_1, (char*) sky_2, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL }   // E2M9
  },
  {
    { 3, 2, 3, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E3M1, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M1
    { 3, 3, 3, 9, 0, 0, 0, 0, 0, 0,  45/5, 60, 0, HUSTR_E3M2, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M2
    { 3, 4, 3, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E3M3, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M3
    { 3, 5, 3, 9, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_E3M4, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M4
    { 3, 6, 3, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E3M5, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M5
    { 3, 7, 3, 9, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_E3M6, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M6
    { 3, 8, 3, 9, 0, 0, 0, 0, 0, 0, 165/5, 60, 0, HUSTR_E3M7, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M7
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 105/5, 60, 0, HUSTR_E3M8, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL },  // E3M8
    { 3, 7, 3, 7, 1, 0, 0, 0, 0, 0, 135/5, 60, 0, HUSTR_E3M9, (char*) lumpname_1, (char*) sky_3, (char*) wilv, (char*) enterpic_1, (char*) enterpic_1, (char*) borderpatch_1, NULL }   // E3M9
  },
  {
    { 4, 2, 4, 9, 0, 0, 0, 0, 0, 0, 165/5, 60, 0, HUSTR_E4M1, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M1
    { 4, 3, 4, 9, 0, 0, 0, 0, 0, 0, 255/5, 60, 0, HUSTR_E4M2, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M2
    { 4, 4, 4, 9, 0, 0, 0, 0, 0, 0, 135/5, 60, 0, HUSTR_E4M3, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M3
    { 4, 5, 4, 9, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_E4M4, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M4
    { 4, 6, 4, 9, 0, 0, 0, 0, 0, 0, 180/5, 60, 0, HUSTR_E4M5, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M5
    { 4, 7, 4, 9, 0, 0, 0, 0, 0, 0, 390/5, 60, 0, HUSTR_E4M6, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M6
    { 4, 8, 4, 9, 0, 0, 0, 0, 0, 0, 135/5, 60, 0, HUSTR_E4M7, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M7
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 360/5, 60, 0, HUSTR_E4M8, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL },  // E4M8
    { 4, 3, 4, 3, 1, 0, 0, 0, 0, 0, 180/5, 60, 0, HUSTR_E4M9, (char*) lumpname_1, (char*) sky_4, (char*) wilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_1, NULL }   // E4M9
  }
};


static map_dests_t next_level_tab_2 [] =
{//  x   M   x    M   S  R  I  C  N  T   Par  Suk  Name, Sky
  { 255,  2, 255, 31, 0, 0, 0, 0, 0, 0,  30/5, 60, 0, HUSTR_1,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 01
  { 255,  3, 255, 31, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_2,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 02
  { 255,  4, 255, 31, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_3,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 03
  { 255,  5, 255, 31, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_4,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 04
  { 255,  6, 255, 31, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_5,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 05
  { 255,  7, 255, 31, 0, 0, 1, 0, 0, 0, 150/5, 60, 0, HUSTR_6,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 06
  { 255,  8, 255, 31, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_7,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 07
  { 255,  9, 255, 31, 0, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_8,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 08
  { 255, 10, 255, 31, 0, 0, 0, 0, 0, 0, 270/5, 60, 0, HUSTR_9,  (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 09
  { 255, 11, 255, 31, 0, 0, 0, 0, 0, 0,  90/5, 60, 0, HUSTR_10, (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 10
  { 255, 12, 255, 31, 0, 0, 2, 0, 0, 0, 210/5, 60, 0, HUSTR_11, (char*) lumpname_2, (char*) sky_1, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 11
  { 255, 13, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_12, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 12
  { 255, 14, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_13, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 13
  { 255, 15, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_14, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 14
  { 255, 16, 255, 31, 0, 0, 0, 0, 0, 0, 210/5, 60, 0, HUSTR_15, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 15
  { 255, 17, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_16, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 16
  { 255, 18, 255, 31, 0, 0, 0, 0, 0, 0, 420/5, 60, 0, HUSTR_17, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 17
  { 255, 19, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_18, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 18
  { 255, 20, 255, 31, 0, 0, 0, 0, 0, 0, 210/5, 60, 0, HUSTR_19, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 19
  { 255, 21, 255, 31, 0, 0, 3, 0, 0, 0, 150/5, 60, 0, HUSTR_20, (char*) lumpname_2, (char*) sky_2, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 20
  { 255, 22, 255, 31, 0, 0, 0, 0, 0, 0, 240/5, 60, 0, HUSTR_21, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 21
  { 255, 23, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_22, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 22
  { 255, 24, 255, 31, 0, 0, 0, 0, 0, 0, 180/5, 60, 0, HUSTR_23, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 23
  { 255, 25, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_24, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 24
  { 255, 26, 255, 31, 0, 0, 0, 0, 0, 0, 150/5, 60, 0, HUSTR_25, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 25
  { 255, 27, 255, 31, 0, 0, 0, 0, 0, 0, 300/5, 60, 0, HUSTR_26, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 26
  { 255, 28, 255, 31, 0, 0, 0, 0, 0, 0, 330/5, 60, 0, HUSTR_27, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 27
  { 255, 29, 255, 31, 0, 0, 0, 0, 0, 0, 420/5, 60, 0, HUSTR_28, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 28
  { 255, 30, 255, 31, 0, 0, 0, 0, 0, 0, 300/5, 60, 0, HUSTR_29, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 29
  { 255,  0, 255,  0, 0, 0, 4, 0, 0, 0, 180/5, 60, 0, HUSTR_30, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 30
  { 255, 16, 255, 32, 1, 0, 0, 0, 0, 0, 120/5, 60, 0, HUSTR_31, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL },	// Map 31
  { 255, 16, 255, 16, 2, 0, 0, 0, 0, 0,  30/5, 60, 0, HUSTR_32, (char*) lumpname_2, (char*) sky_3, (char*) cwilv, (char*) enterpic_2, (char*) enterpic_2, (char*) borderpatch_2, NULL } 	// Map 32
};

/* -------------------------------------------------------------------------------------------- */

static char* mapnamesp[] =	// Plutonia WAD map names.
{
  PHUSTR_1,
  PHUSTR_2,
  PHUSTR_3,
  PHUSTR_4,
  PHUSTR_5,
  PHUSTR_6,
  PHUSTR_7,
  PHUSTR_8,
  PHUSTR_9,
  PHUSTR_10,
  PHUSTR_11,
  PHUSTR_12,
  PHUSTR_13,
  PHUSTR_14,
  PHUSTR_15,
  PHUSTR_16,
  PHUSTR_17,
  PHUSTR_18,
  PHUSTR_19,
  PHUSTR_20,
  PHUSTR_21,
  PHUSTR_22,
  PHUSTR_23,
  PHUSTR_24,
  PHUSTR_25,
  PHUSTR_26,
  PHUSTR_27,
  PHUSTR_28,
  PHUSTR_29,
  PHUSTR_30,
  PHUSTR_31,
  PHUSTR_32
};


static char *mapnamest[] =	// TNT WAD map names.
{
  THUSTR_1,
  THUSTR_2,
  THUSTR_3,
  THUSTR_4,
  THUSTR_5,
  THUSTR_6,
  THUSTR_7,
  THUSTR_8,
  THUSTR_9,
  THUSTR_10,
  THUSTR_11,
  THUSTR_12,
  THUSTR_13,
  THUSTR_14,
  THUSTR_15,
  THUSTR_16,
  THUSTR_17,
  THUSTR_18,
  THUSTR_19,
  THUSTR_20,
  THUSTR_21,
  THUSTR_22,
  THUSTR_23,
  THUSTR_24,
  THUSTR_25,
  THUSTR_26,
  THUSTR_27,
  THUSTR_28,
  THUSTR_29,
  THUSTR_30,
  THUSTR_31,
  THUSTR_32
};

/* -------------------------------------------------------------------------------------------- */

static map_starts_t start_doom1_tab [] =
{
  { 0, 1 },
  { 1, 1 },
  { 2, 1 },
  { 3, 1 },
  { 4, 1 },
  { 5, 1 },
  { 6, 1 },
  { 7, 1 },
  { 8, 1 },
  { 9, 1 }
};

static map_starts_t start_doom2_tab [] =
{
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 },
  { 255, 1 }
};

/* -------------------------------------------------------------------------------------------- */
/*
   We need to copy the mapnames for plutonia and tnt.
*/

void G_InitMapNames (void)
{
  unsigned int i;
  char ** mapnames;

  if (gamemode != commercial)
    return;

  mapnames = NULL;

  switch (gamemission)
  {
    case pack_plut:
      mapnames = mapnamesp;
      break;

    case pack_tnt:
      mapnames = mapnamest;
      break;

    default:
      return;
  }

  i = 1;
  do
  {
    *(G_AccessMapname_E (255, i)) = *mapnames;
    ++mapnames;
  } while (++i <= 32);
}

/* -------------------------------------------------------------------------------------------- */

map_dests_t * G_Find_Mapdef (unsigned int episode, unsigned int map)
{
  extramaps_t * extra_maps;

  if (episode == 255)
  {
    if ((map >= 1) && (map <= ARRAY_SIZE(next_level_tab_2)))
      return (&next_level_tab_2 [map-1]);
  }
  else
  {
    if ((episode >=1)
     && (episode <= QTY_EPISODES)
     && (map >=1)
     && (map <= QTY_MAPS_PER_EPISODE))
      return (&next_level_tab_1 [episode-1][map-1]);
  }

  extra_maps = extramaps_head;
  while (extra_maps)
  {
    if ((extra_maps->episode == episode)
     && (extra_maps->map == map))
      return (&extra_maps->mapdef);
    extra_maps = extra_maps->next;
  }

  return (NULL);
}

/* -------------------------------------------------------------------------------------------- */

map_dests_t * G_Access_MapInfoTab (unsigned int episode, unsigned int map)
{
  if (gamemode == commercial)
    episode = 255;
  return (G_Access_MapInfoTab_E (episode, map));
}

/* -------------------------------------------------------------------------------------------- */

map_dests_t * G_Access_MapInfoTab_E (unsigned int episode, unsigned int map)
{
  unsigned int s;
  map_dests_t * ptr;
  extramaps_t * extra_map;

  ptr = G_Find_Mapdef (episode, map);
  if (ptr)
    return (ptr);

  if (M_CheckParm ("-showmaptables"))
    printf ("Making new map entry for %u %u\n", episode, map);

  extra_map = calloc (1,sizeof(extramaps_t));
  if (extra_map == NULL)
    I_Error ("Could not claim memory for extra map");

  extra_map->next = extramaps_head;
  extramaps_head = extra_map;

  extra_map->episode = episode;
  extra_map->map = map;

  ptr = &extra_map->mapdef;

  ptr->normal_exit_to_episode =
  ptr->secret_exit_to_episode = episode;
  ptr->normal_exit_to_map =
  ptr->secret_exit_to_map = map + 1;
  ptr->time_sucks = 60;

  if (episode != 255)
  {
    ptr->lumpname = (char*)lumpname_1;
    s = episode;
    if (s >= ARRAY_SIZE(skies))
      s = ARRAY_SIZE(skies) - 1;
    ptr->sky = (char*)skies[s];
    ptr->titlepatch = (char*)wilv;
    ptr->enterpic = (char*)enterpic_2;
    ptr->exitpic = (char*)enterpic_2;
    ptr->bordertexture = (char*)borderpatch_1;
  }
  else
  {
    ptr->lumpname = (char*)lumpname_2;
    ptr->sky = (char*)sky_4;
    ptr->titlepatch = (char*)cwilv;
    ptr->enterpic = (char*)enterpic_2;
    ptr->exitpic = (char*)enterpic_2;
    ptr->bordertexture = (char*)borderpatch_2;
  }

  return (ptr);
}

/* ---------------------------------------------------------------------------- */

map_starts_t * G_Access_MapStartTab (unsigned int episode)
{
  if (gamemode == commercial)
  {
    if (episode > 9) episode = 0;
    return (&start_doom2_tab [episode]);
  }
  else
  {
    if (episode > 9)  episode = 9;
    return (&start_doom1_tab [episode]);
  }
}

/* -------------------------------------------------------------------------------------------- */

map_starts_t * G_Access_MapStartTab_E (unsigned int episode)
{
  if (episode == 255)
  {
    return (&start_doom2_tab [0]);
  }
  else
  {
    if (episode > 9)  episode = 9;
    return (&start_doom1_tab [episode]);
  }
}

/* -------------------------------------------------------------------------------------------- */

void G_MapName (char * name, int episode, int map)
{
  map_dests_t * mptr;

  if (gamemode == commercial)
    episode = 255;

  mptr = G_Find_Mapdef (episode, map);
  if (mptr == NULL)
  {
    if (gamemode == commercial)
      sprintf (name, lumpname_2, map);
    else
      sprintf (name, lumpname_1, episode, map);
    return;
  }

  if (gamemode == commercial)
    sprintf (name, mptr -> lumpname, map);
  else
    sprintf (name, mptr -> lumpname, episode, map);
}

/* -------------------------------------------------------------------------------------------- */

void G_MapName_E (char * name, int episode, int map)
{
  map_dests_t * mptr;

  mptr = G_Access_MapInfoTab_E (episode, map);

  if (gamemode == commercial)
    sprintf (name, mptr -> lumpname, map);
  else
    sprintf (name, mptr -> lumpname, episode, map);
}

/* -------------------------------------------------------------------------------------------- */

int G_MapLump (int episode, int map)
{
  char name[10];

  G_MapName (name, episode, map);
  return (W_CheckNumForName (name));
}

/* -------------------------------------------------------------------------------------------- */

char ** G_AccessMapname (unsigned int episode, unsigned int map)
{
  if (gamemode == commercial)
    episode = 255;
  return (G_AccessMapname_E (episode, map));
}

/* -------------------------------------------------------------------------------------------- */

char ** G_AccessMapname_E (unsigned int episode, unsigned int map)
{
  map_dests_t * ptr;

  switch (episode)
  {
    case 254:			// Plutonia
      if ((map >= 1) && (map <= 32))
      {
	if ((gamemode != commercial)
	 || (gamemission != pack_plut))
	  return (&mapnamesp [map-1]);
      }
      break;

    case 253:			// Tnt
      if ((map >= 1) && (map <= 32))
      {
	if ((gamemode != commercial)
	 || (gamemission != pack_tnt))
	  return (&mapnamest [map-1]);
      }
      break;
  }

  ptr = G_Access_MapInfoTab_E (episode, map);
  return (&ptr -> name);
}

/* -------------------------------------------------------------------------------------------- */

static item_to_drop_t item_4 =
{
  MT_CHAINGUY, MT_CHAINGUN, NULL
};

static item_to_drop_t item_3 =
{
  MT_SHOTGUY, MT_SHOTGUN, &item_4
};

static item_to_drop_t item_2 =
{
  MT_POSSESSED, MT_CLIP, &item_3
};

static item_to_drop_t item_1 =
{
  MT_WOLFSS, MT_CLIP, &item_2
};

item_to_drop_t * item_drop_head = &item_1;

/* -------------------------------------------------------------------------------------------- */

#if 0
int G_CmdChecksum (ticcmd_t* cmd)
{
  int		i;
  int		sum = 0;

  for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++)
      sum += ((int *)cmd)[i];

  return sum;
}
#endif

/* -------------------------------------------------------------------------------------------- */
//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd (ticcmd_t* cmd)
{
  int		i;
  boolean	strafe;
  boolean	bstrafe;
  int		speed;
  int		tspeed;
  int		forward;
  int		side;

  ticcmd_t*	base;

  base = I_BaseTiccmd ();		// empty, or external driver
  memcpy (cmd,base,sizeof(*cmd));

  cmd->argnum = i = maketic%BACKUPTICS;
  cmd->consistancy = consistancy[consoleplayer][i];
  cmd->spare1 = 0;
  cmd->spare2 = 0;

  switch (i)
  {
    case 0:
      cmd->value = Give_Max_Damage;
      break;

    case 1:
      cmd->value = Monsters_Infight1;
      break;

    case 2:
      cmd->value = Monsters_Infight2;
      break;

    case 3:
      cmd->value = nobrainspitcheat;
      break;

    default:
      cmd->value = 0;
  }

  strafe = (boolean) (gamekeydown[keyb.strafe] || mousebuttons[keyb.mousebstrafe]
      || joybuttons[keyb.joybstrafe]);
  speed = (gamekeydown[keyb.speed] || joybuttons[keyb.joybspeed]) ^ (keyb.always_run & 1);

  forward = side = 0;

  // use two stage accelerative turning
  // on the keyboard and joystick
  if (joyxmove < 0
      || joyxmove > 0
      || gamekeydown[keyb.right_1]
      || gamekeydown[keyb.right_2]
      || gamekeydown[keyb.left_1]
      || gamekeydown[keyb.left_2])
      turnheld += ticdup;
  else
      turnheld = 0;

  if (turnheld < SLOWTURNTICS)
      tspeed = 2;			// slow turn
  else
      tspeed = speed;

  // let movement keys cancel each other out
  if (strafe)
  {
      if ((gamekeydown[keyb.right_1]) || (gamekeydown[keyb.right_2]))
      {
	  // fprintf(stderr, "strafe right\n");
	  side += sidemove[speed];
      }
      if ((gamekeydown[keyb.left_1]) || (gamekeydown[keyb.left_2]))
      {
	  //	fprintf(stderr, "strafe left\n");
	  side -= sidemove[speed];
      }
      if (joyxmove > 0)
	  side += sidemove[speed];
      if (joyxmove < 0)
	  side -= sidemove[speed];
  }
  else
  {
      if ((gamekeydown[keyb.right_1]) || (gamekeydown[keyb.right_2]))
	  cmd->angleturn -= angleturn[tspeed];
      if ((gamekeydown[keyb.left_1]) || (gamekeydown[keyb.left_2]))
	  cmd->angleturn += angleturn[tspeed];
      if (joyxmove > 0)
	  cmd->angleturn -= angleturn[tspeed];
      if (joyxmove < 0)
	  cmd->angleturn += angleturn[tspeed];
  }

  if ((gamekeydown[keyb.up_1]) || (gamekeydown[keyb.up_2]))
  {
      // fprintf(stderr, "up\n");
      forward += forwardmove[speed];
  }
  if ((gamekeydown[keyb.down_1]) || (gamekeydown[keyb.down_2]))
  {
      // fprintf(stderr, "down\n");
      forward -= forwardmove[speed];
  }
  if (joyymove < 0)
      forward += forwardmove[speed];
  if (joyymove > 0)
      forward -= forwardmove[speed];
  if (gamekeydown[keyb.straferight])
      side += sidemove[speed];
  if (gamekeydown[keyb.strafeleft])
      side -= sidemove[speed];

  // buttons
  cmd->chatchar = HU_dequeueChatChar();

  if (gamekeydown[keyb.fire] || mousebuttons[keyb.mousebfire]
      || joybuttons[keyb.joybfire])
      cmd->buttons |= BT_ATTACK;

  if (gamekeydown[keyb.use] || joybuttons[keyb.joybuse] )
  {
      cmd->buttons |= BT_USE;
      // clear double clicks if hit use button
      dclicks = 0;
  }

  // chainsaw overrides
  for (i=0 ; i<NUMWEAPONS-1 ; i++)
      if (gamekeydown['1'+i])
      {
	  cmd->buttons |= BT_CHANGE;
	  cmd->buttons |= i<<BT_WEAPONSHIFT;
	  break;
      }

  // mouse
  if (mousebuttons[keyb.mousebforward])
      forward += forwardmove[speed];

  // forward double click
  if (mousebuttons[keyb.mousebforward] != dclickstate && dclicktime > 1 )
  {
      dclickstate = mousebuttons[keyb.mousebforward];
      if (dclickstate)
	  dclicks++;
      if (dclicks == 2)
      {
	  cmd->buttons |= BT_USE;
	  dclicks = 0;
      }
      else
	  dclicktime = 0;
  }
  else
  {
      dclicktime += ticdup;
      if (dclicktime > 20)
      {
	  dclicks = 0;
	  dclickstate = 0;
      }
  }

  // strafe double click
  bstrafe = (boolean)	(mousebuttons[keyb.mousebstrafe] || joybuttons[keyb.joybstrafe]);
  if (bstrafe != dclickstate2 && dclicktime2 > 1 )
  {
      dclickstate2 = bstrafe;
      if (dclickstate2)
	  dclicks2++;
      if (dclicks2 == 2)
      {
	  cmd->buttons |= BT_USE;
	  dclicks2 = 0;
      }
      else
	  dclicktime2 = 0;
  }
  else
  {
      dclicktime2 += ticdup;
      if (dclicktime2 > 20)
      {
	  dclicks2 = 0;
	  dclickstate2 = 0;
      }
  }

  forward += mousey;
  if (strafe)
      side += mousex*2;
  else
      cmd->angleturn -= mousex*0x8;

  mousex = mousey = 0;

  if (forward > MAXPLMOVE)
      forward = MAXPLMOVE;
  else if (forward < -MAXPLMOVE)
      forward = -MAXPLMOVE;
  if (side > MAXPLMOVE)
      side = MAXPLMOVE;
  else if (side < -MAXPLMOVE)
      side = -MAXPLMOVE;

  cmd->forwardmove += forward;
  cmd->sidemove += side;

  // special buttons
  if (sendpause)
  {
      sendpause = false;
      cmd->buttons = BT_SPECIAL | BTS_PAUSE;
  }

  if (sendsave)
  {
      sendsave = false;
      cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT);
  }
}

/* -------------------------------------------------------------------------------------------- */

//
// G_DoLoadLevel
//
extern  gamestate_t     wipegamestate;

void G_DoLoadLevel (void)
{
  int i;
  player_t*	p;
  map_dests_t * map_info_p;

  if (!demoplayback)
    M_SetEpiSel (gameepisode);

  map_info_p = G_Access_MapInfoTab (gameepisode, gamemap);

  R_InitSkyMap (map_info_p);

  levelstarttic = gametic;		// for time calculation

  if (wipegamestate == GS_LEVEL)
      wipegamestate = (gamestate_t) -1;		// force a wipe

  gamestate = GS_LEVEL;

  i = 0;
  p = &players[0];
  do
  {
    if (playeringame[i] && p->playerstate == PST_DEAD)
      p->playerstate = PST_REBORN;

    memset (p->frags,0,sizeof(p->frags));
    G_ResetPlayer (p, map_info_p -> reset_kit_etc_on_entering);
    p++;
  } while (++i < MAXPLAYERS);

  sector_list = NULL;

  P_SetupLevel (gameepisode, gamemap, 0, gameskill);
  displayplayer = consoleplayer;		// view the guy you are playing
  starttime = I_GetTime ();
  gameaction = ga_nothing;

  // clear cmd building stuff
  memset (gamekeydown, 0, sizeof(gamekeydown));
  joyxmove = joyymove = 0;
  mousex = mousey = 0;
  sendpause = sendsave = paused = false;
  memset (mousearray, 0, sizeof(mousearray));
  memset (joyarray, 0, sizeof(joyarray));
  M_RemoveDisc ();
}


/* -------------------------------------------------------------------------------------------- */
//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
boolean G_Responder (event_t* ev)
{
  // allow spy mode changes even during the demo
  if (gamestate == GS_LEVEL && ev->type == ev_keydown
      && ev->data1 == KEY_F12 && (singledemo || !deathmatch) )
  {
      // spy mode
      do
      {
	  displayplayer++;
	  if (displayplayer == MAXPLAYERS)
	      displayplayer = 0;
      } while (!playeringame[displayplayer] && displayplayer != consoleplayer);
      return true;
  }

  // any other key pops up menu if in demos
  if (gameaction == ga_nothing && !singledemo &&
      (demoplayback || gamestate == GS_DEMOSCREEN)
      )
  {
      if (ev->type == ev_keydown ||
	  (ev->type == ev_mouse && ev->data1) ||
	  (ev->type == ev_joystick && ev->data1) )
      {
	  M_StartControlPanel ();
	  return true;
      }
      return false;
  }

  if (gamestate == GS_LEVEL)
  {
#if 0
      if (devparm && ev->type == ev_keydown && ev->data1 == ';')
      {
	  G_DeathMatchSpawnPlayer (0);
	  return true;
      }
#endif
      if (HU_Responder (ev))
	  return true;	// chat ate the event
      if (ST_Responder (ev))
	  return true;	// status window ate it
      if (AM_Responder (ev))
	  return true;	// automap ate it
  }

  if (gamestate == GS_FINALE)
  {
      if (F_Responder (ev))
	  return true;	// finale ate the event
  }

  switch (ev->type)
  {
    case ev_keydown:
      if (ev->data1 == KEY_PAUSE)
      {
	  sendpause = true;
	  return true;
      }
      if (ev->data1 <NUMKEYS)
	  gamekeydown[ev->data1] = true;
      return true;    // eat key down events

    case ev_keyup:
      if (ev->data1 <NUMKEYS)
	  gamekeydown[ev->data1] = false;
      return false;   // always let key up events filter down

    case ev_mouse:
      mousebuttons[0] = (boolean) (ev->data1 & 1);
      mousebuttons[1] = (boolean) (ev->data1 & 2);
      mousebuttons[2] = (boolean) (ev->data1 & 4);
      mousex = ev->data2*(mouseSensitivity+5)/10;
      mousey = ev->data3*(mouseSensitivity+5)/10;
      return true;    // eat events

    case ev_joystick:
      joybuttons[0] = (boolean) (ev->data1 & 1);
      joybuttons[1] = (boolean) (ev->data1 & 2);
      joybuttons[2] = (boolean) (ev->data1 & 4);
      joybuttons[3] = (boolean) (ev->data1 & 8);
      joyxmove = ev->data2;
      joyymove = ev->data3;
      return true;    // eat events

    default:
      break;
  }

  return false;
}

/* -------------------------------------------------------------------------------------------- */
//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void)
{
  int		i,j;
  int		buf;
  ticcmd_t*	cmd;

  // do player reborns if needed
  i = 0;
  do
  {
    if (playeringame[i] && players[i].playerstate == PST_REBORN)
      G_DoReborn (i);
  } while (++i < MAXPLAYERS);

  if ((newgametimer)
   && (--newgametimer == 0))
    gameaction = ga_newgame;

  if ((gamecompletedtimer)
   && (--gamecompletedtimer == 0))
    gameaction = ga_completed;

  // do things to change the game state
  // while (gameaction != ga_nothing)		// JAD - Removed loop as it makes sequencing easier
  {						// because the screen will get updated between tics.
      switch (gameaction)
      {
	case ga_loadlevel:
	  G_DoLoadLevel ();
	  break;

	case ga_newgame:
	  M_DrawDisc ();
	  gameaction = ga_newgame2;
	  break;

	case ga_newgame2:
	  if (M_ScreenUpdated ())
	    G_DoNewGame ();
	  break;

	case ga_newgamelater:
	  if ((newgametimer == 0) && (gamecompletedtimer == 0))
	    newgametimer = 40;
	  gameaction = ga_nothing;
	  break;

	case ga_loadgame:
	  G_DoLoadGame ();
	  break;

	case ga_savegame:
	  G_DoSaveGame ();
	  break;

	case ga_playdemo:
	  G_DoPlayDemo ();
	  break;

	case ga_completed:
	  G_DoCompleted ();
	  break;

	case ga_completedlater:
	  if ((newgametimer == 0) && (gamecompletedtimer == 0))
	    gamecompletedtimer = 10;
	  gameaction = ga_nothing;
	  break;

	case ga_victory:
	  if (!demoplayback)
	    M_SetNextEpiSel (gameepisode);
	  F_StartFinale (1);
	  break;

	case ga_worlddone:
	  if (M_ScreenUpdated ())
	    G_DoWorldDone ();
	  break;

	case ga_screenshot:
	  M_ScreenShot ();
	  gameaction = ga_nothing;
	  break;

	case ga_nothing:
	  break;

	default:			// Just in case
	  gameaction = ga_nothing;
      }
  }

  // get commands, check consistancy,
  // and build new consistancy check
  buf = (gametic/ticdup)%BACKUPTICS;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
      if (playeringame[i])
      {
	  cmd = &players[i].cmd;

	  memcpy (cmd, &netcmds[i][buf], sizeof(ticcmd_t));

	  if (demoplayback)
	      G_ReadDemoTiccmd (cmd);
	  if (demorecording)
	      G_WriteDemoTiccmd (cmd);

	  // check for turbo cheats
	  j = cmd->forwardmove;
	  /* Bug in ARM compiler fails to sign extend chars to ints */
	  if (j > 0x7F) j |= ~0xFF;
	  if (j > TURBOTHRESHOLD
	      && !(gametic&31) && ((gametic>>5)&3) == i )
	  {
	      extern char *player_names[4];
	      players[consoleplayer].message =
		      HU_printf ("%s is turbo!",player_names[i]);
	  }

	  if (netgame && !netdemo && !(gametic%ticdup) )
	  {
	      if (gametic > BACKUPTICS
		  && consistancy[i][buf] != cmd->consistancy)
	      {
#ifdef NORMALUNIX
		  printf ("consistency failure %u (%i should be %i)\n",
			   i, cmd->consistancy, consistancy[i][buf]);
#endif
	      }
	      if (players[i].mo)
		  consistancy[i][buf] = players[i].mo->x;
	      else
		  consistancy[i][buf] = rndindex;

	      /* Try to prevent consistancy errors by copying */
	      /* some of the cheats across */
	      if (i)
	      {
		switch (cmd->argnum)
		{
		  case 0:
		    Give_Max_Damage = (boolean) cmd->value;
		    break;

		  case 1:
		    Monsters_Infight1 = (boolean) cmd->value;
		    break;

		  case 2:
		    Monsters_Infight2 = (boolean) cmd->value;
		    break;

		  case 3:
		    nobrainspitcheat = (boolean) cmd->value;
		    break;
		}
	      }
	  }
      }
  }

  // check for special buttons
  for (i=0 ; i<MAXPLAYERS ; i++)
  {
      if (playeringame[i])
      {
	  if (players[i].cmd.buttons & BT_SPECIAL)
	  {
	      switch (players[i].cmd.buttons & BT_SPECIALMASK)
	      {
		case BTS_PAUSE:
		  paused = (boolean) (paused ^ 1);//paused ^= 1;
		  if (paused)
		      S_PauseSound ();
		  else
		      S_ResumeSound ();
		  break;

		case BTS_SAVEGAME:
		  if (!savedescription[0])
		      strcpy (savedescription, "NET GAME");
		  savegameslot =
		      (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
		  gameaction = ga_savegame;
		  break;
	      }
	  }
      }
  }

  // do main actions
  switch (gamestate)
  {
    case GS_LEVEL:
      P_Ticker ();
      ST_Ticker ();
      AM_Ticker ();
      HU_Ticker ();
      break;

    case GS_INTERMISSION:
      WI_Ticker ();
      break;

    case GS_FINALE:
      F_Ticker ();
      break;

    case GS_DEMOSCREEN:
      D_PageTicker ();
      break;
  }
}

/* -------------------------------------------------------------------------------------------- */
//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer
// Called at the start.
// Called by the game initialization functions.
//
#if 0
void G_InitPlayer (int player)
{
//  player_t*	p;

// set up the saved info
//  p = &players[player];

  // clear everything else to defaults
  G_PlayerReborn (player);
}
#endif
/* -------------------------------------------------------------------------------------------- */
//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (int player)
{
  player_t*	p;

  p = &players[player];

  memset (p->powers, 0, sizeof (p->powers));
  memset (p->cards, 0, sizeof (p->cards));
  p->mo->flags &= ~MF_SHADOW;		// cancel invisibility
  p->extralight = 0;			// cancel gun flashes
  p->fixedcolormap = 0;			// cancel ir gogles
  p->damagecount = 0;			// no palette changes
  p->bonuscount = 0;

  /*
  If the player has a chainsaw and a berserk power-up, and ends a level
  with their fists selected, they start the next level with their fists.
  This shouldn't happen, because the berserk power-up has been removed and
  they have the chainsaw.
  */
  if ((p->readyweapon == wp_fist)
   && (p->weaponowned[wp_chainsaw]))
    p->readyweapon = wp_chainsaw;
}


/* -------------------------------------------------------------------------------------------- */
/* Usually we start with the pistol, but if the
   initial_bullets has been set to zero by a DeHack patch
   and the weapon actually requires ammo, then switch to the fists.
*/
static void G_SetInitialWeapon (player_t* p)
{
  int i;

  p->weaponowned[wp_fist] = true;
  p->weaponowned[wp_pistol] = true;

  if (((p->ammo[am_clip] = Initial_Bullets) == 0)
   && (weaponinfo[wp_pistol].ammo != am_noammo))
    p->readyweapon = p->pendingweapon = wp_fist;
  else
    p->readyweapon = p->pendingweapon = wp_pistol;

  for (i=0 ; i<NUMAMMO ; i++)
      p->maxammo[i] = maxammo[i];
}

/* -------------------------------------------------------------------------------------------- */
//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn (int player)
{
  player_t*	p;
  int		frags[MAXPLAYERS];
  int		killcount;
  int		itemcount;
  int		secretcount;

  memcpy (frags,players[player].frags,sizeof(frags));
  killcount = players[player].killcount;
  itemcount = players[player].itemcount;
  secretcount = players[player].secretcount;

  p = &players[player];
  memset (p, 0, sizeof(*p));

  memcpy (players[player].frags, frags, sizeof(players[player].frags));
  players[player].killcount = killcount;
  players[player].itemcount = itemcount;
  players[player].secretcount = secretcount;

  p->usedown = p->attackdown = true;	// don't do anything immediately
  p->playerstate = PST_LIVE;
  p->health = Initial_Health;
  G_SetInitialWeapon (p);
}

/* -------------------------------------------------------------------------------------------- */
//
// G_ResetPlayer
// Resets player inventory at start of level.
//
static void G_ResetPlayer (player_t* p, int flags)
{
  if (flags & 1)				// reset health?
  {
    p->health = Initial_Health;
  }

  if (flags & 2)				// reset inventory?
  {
    memset (p->weaponowned, 0, sizeof(p->weaponowned));
    memset (p->ammo, 0, sizeof(p->ammo));

    p->backpack = false;
    p->armourpoints = 0;
    p->armourtype = NOARMOUR;
    G_SetInitialWeapon (p);
  }
}

/* -------------------------------------------------------------------------------------------- */
//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing_t spot
// because something is occupying it
//
void P_SpawnPlayer (mapthing_t* mthing);

boolean
G_CheckSpot
( int		playernum,
  mapthing_t*	mthing )
{
  fixed_t		x;
  fixed_t		y;
  subsector_t*	ss;
  unsigned		an;
  mobj_t*		mo;
  int			i;

  if (!players[playernum].mo)
  {
      // first spawn of level, before corpses
      for (i=0 ; i<playernum ; i++)
	  if (players[i].mo->x == mthing->x << FRACBITS
	      && players[i].mo->y == mthing->y << FRACBITS)
	      return false;
      return true;
  }

  x = mthing->x << FRACBITS;
  y = mthing->y << FRACBITS;

  if (!P_CheckPosition (players[playernum].mo, x, y) )
      return false;

  // flush an old corpse if needed
  if (bodyqueslot >= BODYQUESIZE)
      P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]);
  bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo;
  bodyqueslot++;

  // spawn a teleport fog
  ss = R_PointInSubsector (x,y);
  an = ( ANG45 * ((unsigned)mthing->angle/45) ) >> ANGLETOFINESHIFT;

  mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an]
		    , ss->sector->floorheight
		    , MT_TFOG);

  if (players[consoleplayer].viewz != 1)
      S_StartSound (mo, sfx_telept);	// don't start sound on first frame

  return true;
}

/* -------------------------------------------------------------------------------------------- */
//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//
void G_DeathMatchSpawnPlayer (int playernum)
{
  int i,j;
  int selections;
  dmstart_t* dmthis;

  selections = 0;
  for (dmthis = deathmatchstartlist ; dmthis != NULL ; dmthis = dmthis->next)
  {
    dmthis -> tried = 0;
    selections++;
  }

  j = 0;
  while (j<selections)
  {
    i = (selections - 1) - (P_Random() % selections);	// Reverse the linked list here..
    dmthis = deathmatchstartlist;
    while (i)
    {
      dmthis = dmthis->next;
      i--;
    }

    if (dmthis -> tried == 0)			// Have we tried this spot yet?
    {
      dmthis -> tried = 1;			// No.
      j++;

      if (G_CheckSpot (playernum, &dmthis->dmstart))
      {
	dmthis->dmstart.type = playernum+1;
	P_SpawnPlayer (&dmthis->dmstart);
	return;
      }
    }
  }

  // no good spot, so the player will probably get stuck
  P_SpawnPlayer (&playerstarts[playernum]);
}

/* -------------------------------------------------------------------------------------------- */

static mobjtype_t G_SprNum (spritenum_t sprite)
{
  mobjinfo_t *	ptr;
  state_t*	st;
  unsigned int	count;

  count = 0;
  ptr = mobjinfo;

  do
  {
    if (ptr->flags & MF_SPECIAL)
    {
      st = &states[ptr->spawnstate];
      if (st->sprite == sprite)
	return ((mobjtype_t)count);
    }
    ptr++;
  } while (++count < NUMMOBJTYPES);

  return (MT_NULL);
}

/* -------------------------------------------------------------------------------------------- */
/*
   We have to find the object of our desire in the table
   in case there is a Dehack patch that has changed things...
*/

static void G_SpawnObject (fixed_t x, fixed_t y, spritenum_t sprite)
{
  fixed_t    z;
  mobjtype_t num;

  num = G_SprNum (sprite);
  if (num != MT_NULL)
  {
    if (mobjinfo[num].flags & MF_SPAWNCEILING)
      z = ONCEILINGZ;
    else
      z = ONFLOORZ;
    P_SpawnMobj (x, y, z, num);
  }
}

/* -------------------------------------------------------------------------------------------- */
/*
   When we cheat and respawn in single player mode, we need to drop
   the keys etc so that we can get them back.
*/

static void G_DropAllKit (player_t* player)
{
  fixed_t x,y;

  if (netgame)
    return;

  x = player -> mo -> x;
  y = player -> mo -> y;

  if (player->cards[it_bluecard])
    G_SpawnObject (x, y, SPR_BKEY);

  if (player->cards[it_redcard])
    G_SpawnObject (x, y, SPR_RKEY);

  if (player->cards[it_yellowcard])
    G_SpawnObject (x, y, SPR_YKEY);

  if (player->cards[it_yellowskull])
    G_SpawnObject (x, y, SPR_YSKU);

  if (player->cards[it_redskull])
    G_SpawnObject (x, y, SPR_RSKU);

  if (player->cards[it_blueskull])
    G_SpawnObject (x, y, SPR_BSKU);

  if (player->backpack)
    G_SpawnObject (x, y, SPR_BPAK);
}

/* -------------------------------------------------------------------------------------------- */
//
// G_DoReborn
//
void G_DoReborn (int playernum)
{
  int i;

  if ((!netgame) && (!gamekeydown['`']))
  {
    // reload the level from scratch
    gameaction = ga_loadlevel;
  }
  else
  {
    G_DropAllKit (&players[playernum]);
    // respawn at the start

    // first dissasociate the corpse
    players[playernum].mo->player = NULL;

    // spawn at random spot if in death match
    if (deathmatch)
    {
      G_DeathMatchSpawnPlayer (playernum);
      return;
    }

    if (G_CheckSpot (playernum, &playerstarts[playernum]) )
    {
      P_SpawnPlayer (&playerstarts[playernum]);
      return;
    }

    // try to spawn at one of the other players spots
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
      if (G_CheckSpot (playernum, &playerstarts[i]) )
      {
	playerstarts[i].type = playernum+1;	// fake as other player
	P_SpawnPlayer (&playerstarts[i]);
	playerstarts[i].type = i+1;		// restore
	return;
      }
	// he's going to be inside something.  Too bad.
    }
    P_SpawnPlayer (&playerstarts[playernum]);
  }
}

/* -------------------------------------------------------------------------------------------- */

void G_ScreenShot (void)
{
  gameaction = ga_screenshot;
}


/* -------------------------------------------------------------------------------------------- */
/*
 User has pressed the exit switch.
 Allow a few more frames of animation so that the switch
 can be seen to travel across before the screen wipes.
*/

void G_ExitLevellater (void)
{
  secretexit = 0;
  gameaction = ga_completedlater;
}

void G_ExitLevel (void)
{
  secretexit = 0;
  gameaction = ga_completed;
}

/* -------------------------------------------------------------------------------------------- */
// Here's for the german edition. (ie no wolf3d!)
void G_SecretExitLevel (void)
{
  unsigned int s;

  map_dests_t *   map_info_pc;  // Info for current map
  map_dests_t *   map_info_pn;  // Info for next map

  map_info_pc = G_Access_MapInfoTab (gameepisode, gamemap);
  map_info_pn = G_Find_Mapdef  (map_info_pc -> secret_exit_to_episode,
				map_info_pc -> secret_exit_to_map);

  if ((map_info_pn)
   && ((s = map_info_pn -> this_is_a_secret_level) == 0))
  {
    s = 3;				// 3 = no intermission message!
    map_info_pn -> this_is_a_secret_level = s;
  }

  secretexit = s;

  // IF NO WOLF3D LEVELS, NO SECRET EXIT!
  if ((gamemode == commercial)
   && (map_info_pc -> secret_exit_to_map == 32)
   && (G_MapLump (255, map_info_pc -> secret_exit_to_map) < 0))
  {
    secretexit = 0;
  }

  gameaction = ga_completed;
}

void G_SecretExitLevellater (void)
{
  G_SecretExitLevel ();
  gameaction = ga_completedlater;
}

/* -------------------------------------------------------------------------------------------- */
//
// G_DoCompleted
//

void G_DoCompleted (void)
{
  int i;
  int j;
  map_dests_t * map_info_p;

  gameaction = ga_nothing;

  i = 0;
  do
  {
    if (playeringame[i])
      G_PlayerFinishLevel (i);		// take away cards and stuff
  } while (++i < MAXPLAYERS);

  if (automapactive)
      AM_Stop ();

  map_info_p = G_Access_MapInfoTab (gameepisode, gamemap);

  if (map_info_p -> this_is_a_secret_level)
  {
    i = 0;
    do
    {
      players[i].didsecret = true;
    } while (++i < MAXPLAYERS);
  }

  wminfo.didsecret = players[consoleplayer].didsecret;

  if (secretexit)
  {
    i = map_info_p -> secret_exit_to_episode;
    j = map_info_p -> secret_exit_to_map;
  }
  else
  {
    i = map_info_p -> normal_exit_to_episode;
    j = map_info_p -> normal_exit_to_map;
  }

  wminfo.next_episode = i;
  wminfo.next_map = j;

  if (G_MapLump (i, j) == -1)
  {
    gameaction = ga_victory;
    return;
  }

  if (i == 255)
    i = gameepisode;

  // wminfo.next is 0 biased, unlike gamemap so we have to subtract 1
  wminfo.epsd = i - 1;
  wminfo.next = j - 1;
  wminfo.last = gamemap - 1;

  if (map_info_p -> nointermission)
  {
    G_WorldDone ();
    return;
  }

  wminfo.maxkills = totalkills;
  wminfo.maxitems = totalitems;
  wminfo.maxsecret = totalsecret;
  wminfo.maxfrags = 0;
  wminfo.partime = map_info_p -> par_time_5 * (35*5);
  wminfo.timesucks = map_info_p -> time_sucks * 60;
  wminfo.pnum = consoleplayer;

  // If the map that we just played was loaded from a Pwad and the par times haven't been
  // modified then don't show it.

  if ((modifiedgame == true) && (par_changed == false))
  {
    i = G_MapLump (gameepisode,gamemap);
    if ((i != -1)			// Cannot happen!
     && (lumpinfo[i].handle != lumpinfo[0].handle))
	wminfo.partime = 0;
  }

  i = 0;
  do
  {
    wminfo.plyr[i].in = playeringame[i];
    wminfo.plyr[i].skills = players[i].killcount;
    wminfo.plyr[i].sitems = players[i].itemcount;
    wminfo.plyr[i].ssecret = players[i].secretcount;
    wminfo.plyr[i].stime = leveltime;
    memcpy (wminfo.plyr[i].frags, players[i].frags,
	      sizeof(wminfo.plyr[i].frags));
  } while (++i < MAXPLAYERS);

  gamestate = GS_INTERMISSION;
  viewactive = false;
  automapactive = false;

  if (statcopy)
      memcpy (statcopy, &wminfo, sizeof(wminfo));

  WI_Start (&wminfo);
}


/* -------------------------------------------------------------------------------------------- */
//
// G_WorldDone
//
void G_WorldDone (void)
{
  gameaction = ga_worlddone;

  if (secretexit)
      players[consoleplayer].didsecret = true;

  if (gamemode == commercial)
      F_StartFinale (0);

  if (gameaction == ga_worlddone)
    M_DrawDisc ();
}

void G_WorldDone2 (void)
{
  gameaction = ga_worlddone;
  M_DrawDisc ();
}

/* -------------------------------------------------------------------------------------------- */

void G_DoWorldDone (void)
{
  gamestate = GS_LEVEL;
  gameepisode = wminfo.next_episode;
  gamemap = wminfo.next_map;
  G_DoLoadLevel ();
  gameaction = ga_nothing;
  viewactive = true;
}

/* -------------------------------------------------------------------------------------------- */
//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
static char	savename[256];

void G_LoadGame (char* name)
{
  strcpy (savename, name);
  gameaction = ga_loadgame;
}

/* -------------------------------------------------------------------------------------------- */

#define VERSIONSIZE		16


void G_DoLoadGame (void)
{
  int		i;
  int		a,b,c;
  unsigned int	length;
  skill_t	l_skill;
  int		l_episode;
  int		l_map;
  byte*		loadbuffer;
  byte*		save_p;
  char		vcheck[VERSIONSIZE];

  gameaction = ga_nothing;

  length = M_ReadFile (savename, &loadbuffer);
  if (length < SAVESTRINGSIZE)
    return;
  save_p = loadbuffer + SAVESTRINGSIZE;

  // skip the description field
  memset (vcheck,0,sizeof(vcheck));
  sprintf (vcheck,"version %i",SAVE_GAME_VERSION);
  if (strcmp ((const char*)save_p, (const char *)vcheck))
      return;				// bad version
  save_p += VERSIONSIZE;

  l_skill = (skill_t) *save_p++;	// Using local variable instead of "gameskill" Fixes the demon speed bug.
				      // http://doomwiki.org/wiki/Demon_speed_bug
  l_episode = *save_p++;
  l_map = *save_p++;
  for (i=0 ; i<MAXPLAYERS ; i++)
      playeringame[i] = (boolean) *save_p++;

  D_LoadCheats ();
  // load a base level
  G_InitNew (l_skill, l_episode, l_map);

  // get the times
  a = *save_p++;
  b = *save_p++;
  c = *save_p++;
  leveltime = (a<<16) + (b<<8) + c;

  // dearchive all the modifications
  save_p = P_UnArchivePlayers (save_p);
  save_p = P_UnArchiveWorld (save_p);
  save_p = P_UnArchiveThinkers (save_p);
  save_p = P_UnArchiveSpecials (save_p);
  BOOMSTATEMENT(save_p = P_UnArchiveMap(save_p);)
  P_RestoreTargets ();

  if (*save_p != 0x1d)
      I_Error ("Bad savegame");

  // done
  Z_Free (loadbuffer);

  if (setsizeneeded)
      R_ExecuteSetViewSize ();

  // draw the pattern into the back screen
  R_FillBackScreen ();
  M_RemoveDisc ();
}

/* -------------------------------------------------------------------------------------------- */
//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void
G_SaveGame
( int	slot,
char*	description )
{
  savegameslot = slot;
  strcpy (savedescription, description);
  sendsave = true;
}

/* -------------------------------------------------------------------------------------------- */

void G_GetSaveGameName (char * name, int i)
{
#ifdef __riscos
  if (M_CheckParm("-cdrom"))
    sprintf(name,"<Choices$Dir>.dsg.%s%d/dsg",	save_game_messages [GG_SAVEGAMENAME], i);
  else
    sprintf(name,"%s.%s%d/dsg", basedefaultdir, save_game_messages [GG_SAVEGAMENAME], i);
#else
  if (M_CheckParm("-cdrom"))
    sprintf(name,"c:\\doomdata\\%s%d.dsg",	save_game_messages [GG_SAVEGAMENAME], i);
  else
    sprintf(name,"%s/%s%d.dsg", basedefaultdir, save_game_messages [GG_SAVEGAMENAME], i);
#endif
}

/* ----------------------------------------------------------------------- */
/*
   A quick and dirty function to get an approximate
   size of buffer required for the save game.
*/

static unsigned int G_GetSaveBufferSize (void)
{
  unsigned int	length;
  thinker_t*	th;

  length = 512;				// Overheads.
  length += numsectors * 14;		// 14 bytes per sector
  length += numlines * 26;		// 26 bytes for a two sided line

  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    length += 200;			// Just a guess!
  }

  return (length);
}

/* ----------------------------------------------------------------------- */

void G_DoSaveGame (void)
{
  int		i;
  unsigned int	length;
  unsigned int	savebuffersize;
  byte*		save_p;
  byte*		savebuffer;
  char		name [100];
  char		name2 [VERSIONSIZE];

  G_GetSaveGameName (name, savegameslot);

  savebuffersize = G_GetSaveBufferSize ();

  save_p = savebuffer = Z_Malloc (savebuffersize, PU_STATIC, NULL);

  memcpy (save_p, savedescription, SAVESTRINGSIZE);
  save_p += SAVESTRINGSIZE;
  memset (name2,0,sizeof(name2));
  sprintf (name2,"version %i",SAVE_GAME_VERSION);
  memcpy (save_p, name2, VERSIONSIZE);
  save_p += VERSIONSIZE;

  *save_p++ = gameskill;
  *save_p++ = gameepisode;
  *save_p++ = gamemap;
  for (i=0 ; i<MAXPLAYERS ; i++)
      *save_p++ = playeringame[i];
  *save_p++ = leveltime>>16;
  *save_p++ = leveltime>>8;
  *save_p++ = leveltime;

  save_p = P_ArchivePlayers (save_p);
  BOOMSTATEMENT(save_p = P_ThinkerToIndex(save_p);)
  save_p = P_ArchiveWorld (save_p);
  save_p = P_ArchiveThinkers (save_p);
  BOOMSTATEMENT(save_p = P_IndexToThinker(save_p);)
  save_p = P_ArchiveSpecials (save_p);
  BOOMSTATEMENT(save_p = P_ArchiveMap(save_p);)

  *save_p++ = 0x1d;		// consistancy marker

  for (i = 1;i<myargc;i++)
  {
    save_p += sprintf ((char*)save_p, " %s", myargv[i]);
  }

  length = save_p - savebuffer;
  if (length > savebuffersize)
    I_Error ("Savegame buffer overrun (%u > %u)", length, savebuffersize);

  // printf ("Writing %X bytes (buffer = %X)\n", length, savebuffersize);

  if (M_WriteFile (name, savebuffer, length) == false)
    players[consoleplayer].message = "SAVE FAILED!!";
  else
    players[consoleplayer].message = save_game_messages [GG_GGSAVED];

  gameaction = ga_nothing;
  savedescription[0] = 0;

  Z_Free (savebuffer);

  // draw the pattern into the back screen
  R_FillBackScreen ();
}

/* -------------------------------------------------------------------------------------------- */
//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set.
//
skill_t	d_skill;
int     d_episode;
int     d_map;

void
G_DeferedInitNew
( skill_t	skill,
  int		episode,
  int		map)
{
  d_skill = skill;
  d_episode = episode;
  d_map = map;
  gameaction = ga_newgame;
}

/* -------------------------------------------------------------------------------------------- */

void
G_DeferedInitNewLater
( skill_t	skill,
  int		episode,
  int		map)
{
  d_skill = skill;
  d_episode = episode;
  d_map = map;
  gameaction = ga_newgamelater;
}

/* -------------------------------------------------------------------------------------------- */

void G_DoNewGame (void)
{
  D_LoadCheats ();
  demoplayback = false;
  netdemo = false;
  netgame = false;
  deathmatch = 0;
  playeringame[1] = playeringame[2] = playeringame[3] = false;
  respawnparm = false;
  fastparm = false;
  nomonsters2 = false;
  consoleplayer = 0;
  G_InitNew (d_skill, d_episode, d_map);
  gameaction = ga_nothing;
}

/* -------------------------------------------------------------------------------------------- */

void
G_InitNew
( skill_t	skill,
  int		episode,
  int		map )
{
  int  i;

  R_InitColormaps (0);
  R_InitLightTables ();

  if (paused)
  {
      paused = false;
      S_ResumeSound ();
  }

  if (skill > sk_nightmare)
      skill = sk_nightmare;

  // This was quite messy with SPECIAL and commented parts.
  // Supposedly hacks to make the latest edition work.
  // It might not work properly.

  if (G_MapLump (episode,map) == -1)
  {
    if (map < 1)
      map = 1;

    if (gamemode == commercial)
    {
      while ((map > 1) && (G_MapLump (episode,map) == -1))
	map--;
    }
    else
    {
      do
      {
	if (map <= 1)
	{
	  if (episode <= 1)
	    break;
	  map = 9;
	  episode--;
	}
	else
	{
	  map--;
	}
      } while (G_MapLump (episode,map) == -1);
    }
  }

  M_ClearRandom ();

  if (skill == sk_nightmare || respawnparm )
      respawnmonsters = true;
  else
      respawnmonsters = false;

#if 0
  /* Would be nice to be able to remove this as it causes problems... */

  if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare) )
  {
      for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	  states[i].tics >>= 1;
      mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
      mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
      mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
  }
  else if (skill != sk_nightmare && gameskill == sk_nightmare)
  {
      for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
	  states[i].tics <<= 1;
      mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
      mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
      mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
  }
#endif

  // force players to be initialized upon first level load
  for (i=0 ; i<MAXPLAYERS ; i++)
      players[i].playerstate = PST_REBORN;

  usergame = true;		// will be set false if a demo
  paused = false;
  demoplayback = false;
  automapactive = false;
  viewactive = true;
  gameepisode = episode;
  gamemap = map;
  gameskill = skill;
  viewactive = true;
  G_DoLoadLevel ();
}

/* -------------------------------------------------------------------------------------------- */
//
// DEMO RECORDING
//
#define DEMOMARKER		0x80


void G_ReadDemoTiccmd (ticcmd_t* cmd)
{
  byte*	p;

  p = demo_p;

  /* modified: allow 'q' to abort demo playback */
  if ((*p == DEMOMARKER) || (gamekeydown['q']))
  {
    // end of demo data stream
    G_CheckDemoStatus ();
  }
  else
  {
    cmd->forwardmove = ((signed char)*p++);
    cmd->sidemove = ((signed char)*p++);
    cmd->angleturn = ((signed char)*p++)<<8;
    cmd->buttons = (unsigned char)*p++;
    demo_p = p;
  }
}

/* -------------------------------------------------------------------------------------------- */

void G_WriteDemoTiccmd (ticcmd_t* cmd)
{
  byte*	p;

  if (gamekeydown['q'])	   // press q to end demo recording
    G_CheckDemoStatus ();

  p = demo_p;
  p[0] = cmd->forwardmove;
  p[1] = cmd->sidemove;
  p[2] = (cmd->angleturn+128)>>8;
  p[3] = cmd->buttons;

  if (p > (demoend - 16))
  {
    // no more space
    G_CheckDemoStatus ();
  }
  else
  {
    G_ReadDemoTiccmd (cmd);	 // make SURE it is exactly the same
  }
}

/* -------------------------------------------------------------------------------------------- */
//
// G_RecordDemo
//
void G_RecordDemo (char* name)
{
  int	     i;
  int				maxsize;

  usergame = false;
  strcpy (demoname, name);
#ifdef __riscos
  /// strcat (demoname, "/lmp");  /* We don't bother with extensions on RiscOS */
#else
  strcat (demoname, ".lmp");
#endif
  maxsize = 0x20000;
  i = M_CheckParm ("-maxdemo");
  if (i && i<myargc-1)
      maxsize = atoi(myargv[i+1])*1024;
  demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL);
  demoend = demobuffer + maxsize;

  demorecording = true;
}

/* -------------------------------------------------------------------------------------------- */

void G_BeginRecording (void)
{
  int i;
  byte*	p;

  p = demobuffer;
  *p++ = DEMO_VERSION;
  *p++ = gameskill;
  *p++ = gameepisode;
  *p++ = gamemap;
  *p++ = deathmatch;
  *p++ = respawnparm;
  *p++ = fastparm;
  *p++ = nomonsters1 | nomonsters2;
  *p++ = consoleplayer;

  for (i=0 ; i<MAXPLAYERS ; i++)
    *p++ = playeringame[i];

  demo_p = p;
}


/* -------------------------------------------------------------------------------------------- */
//
// G_PlayDemo
//

char*	defdemoname;

void G_DeferedPlayDemo (char* name)
{
  defdemoname = name;
  gameaction = ga_playdemo;
}

/* -------------------------------------------------------------------------------------------- */

void G_DoPlayDemo (void)
{
  int i, episode, map;
  int dlump,mlump;
  skill_t skill;
  byte * p;
  const char * lumpname;
  char longpath [250];

  gameaction = ga_nothing;
  lumpname = leafname (defdemoname);

  dlump = -1;
  if (strcmp (lumpname, defdemoname) == 0)
  {
    dlump = W_CheckNumForName (lumpname);
    if (dlump != -1)
    {
      if (W_LumpLength (dlump) < 20)
	return;
      demobuffer = demo_p = W_CacheLumpNum (dlump, PU_STATIC);
    }
  }

  if (dlump == -1)
  {
    lumpname = defdemoname;
    if (access (defdemoname, R_OK))
    {
      sprintf (longpath, "%s"EXTSEP"lmp", defdemoname);
      lumpname = longpath;
    }

    i = M_ReadFile (lumpname, &demobuffer);
    if (i < 20)
    {
      printf ("Cannot open demo file %s\n", defdemoname);
      return;
    }

    demo_p = demobuffer;
  }

  D_ClearCheats ();

  p = demo_p;
  i = *p++;			/* Version number of program that made the recording */
  if ((i / 100) != (DEMO_VERSION / 100))  /* Assume that we can have a stab at */
  {					  /* playing this if the major number agrees */
#ifdef NORMALUNIX
    fprintf( stderr, "Demo is from a different game version! (%d)\n", i);
#endif
    if (M_CheckParm ("-forcedemo") == 0)
    {
      // gameaction = ga_nothing;
      return;
    }
  }

  if (dlump != -1)
  {
    // Do not play a demo if the map is in a pwad
    mlump = G_MapLump (p[1], p[2]);
    if ((mlump == -1)
     || (W_SameWadfile (mlump, dlump) == 0))
    {
      Z_ChangeTag (demobuffer, PU_CACHE);
      return;
    }
  }

  skill = (skill_t) *p++;
  episode = *p++;
  map = *p++;
  deathmatch = *p++;
  respawnparm = (boolean) *p++;
  fastparm = (boolean) *p++;
  nomonsters2 = (boolean) *p++;
  consoleplayer = *p++;

  for (i=0 ; i<MAXPLAYERS ; i++)
      playeringame[i] = (boolean) *p++;

  demo_p = p;

  if (playeringame[1])
  {
      netgame = true;
      netdemo = true;
  }

  // don't spend a lot of time in loadlevel
  precache = false;
  G_InitNew (skill, episode, map);
  demoplayback = true;
  precache = true;
  usergame = false;
}

/* -------------------------------------------------------------------------------------------- */
//
// G_TimeDemo
//
void G_TimeDemo (char* name)
{
  nodrawers = (boolean) M_CheckParm ("-nodraw");
  noblit = (boolean) M_CheckParm ("-noblit");
  timingdemo = true;
  singletics = true;

  defdemoname = name;
  gameaction = ga_playdemo;
}


/* -------------------------------------------------------------------------------------------- */
/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

boolean G_CheckDemoStatus (void)
{
  int	     endtime;

  if (timingdemo)
  {
      endtime = I_GetTime ();
      I_Error ("timed %i gametics in %i realtics",gametic
	       , endtime-starttime);
  }

  if (demoplayback)
  {
      if (singledemo)
	  I_Quit ();

      Z_ChangeTag (demobuffer, PU_CACHE);
      demoplayback = false;
      netdemo = false;
      netgame = false;
      deathmatch = 0;
      playeringame[1] = playeringame[2] = playeringame[3] = false;
      respawnparm = false;
      fastparm = false;
      nomonsters2 = false;
      consoleplayer = 0;
      D_AdvanceDemo ();
      return true;
  }

  if (demorecording)
  {
      *demo_p++ = DEMOMARKER;
      M_WriteFile (demoname, demobuffer, demo_p - demobuffer);
      Z_Free (demobuffer);
      demorecording = false;
      I_Error ("Demo %s recorded",demoname);
  }

  return false;
}

/* -------------------------------------------------------------------------------------------- */

static const char * const cast_mtconv [] =
{
  "PLAYER", "POSSESSED", "SHOTGUY", "VILE", "FIRE",
  "UNDEAD", "TRACER", "SMOKE", "FATSO", "FATSHOT",
  "CHAINGUY", "TROOP", "SERGEANT", "SHADOWS", "HEAD",
  "BRUISER", "BRUISERSHOT", "KNIGHT", "SKULL", "SPIDER",
  "BABY", "CYBORG", "PAIN", "WOLFSS", "KEEN", "BOSSBRAIN",
  "BOSSSPIT", "BOSSTARGET", "SPAWNSHOT", "SPAWNFIRE",
  "BARREL", "TROOPSHOT", "HEADSHOT", "ROCKET", "PLASMA",
  "BFG", "ARACHPLAZ", "PUFF", "BLOOD", "TFOG", "IFOG",
  "TELEPORTMAN", "EXTRABFG", "MISC0", "MISC1", "MISC2",
  "MISC3", "MISC4", "MISC5", "MISC6", "MISC7", "MISC8",
  "MISC9", "MISC10", "MISC11", "MISC12", "INV", "MISC13",
  "INS", "MISC14", "MISC15", "MISC16", "MEGA", "CLIP",
  "MISC17", "MISC18", "MISC19", "MISC20", "MISC21", "MISC22",
  "MISC23", "MISC24", "MISC25", "CHAINGUN", "MISC26", "MISC27",
  "MISC28", "SHOTGUN", "SUPERSHOTGUN", "MISC29", "MISC30",
  "MISC31", "MISC32", "MISC33", "MISC34", "MISC35", "MISC36",
  "MISC37", "MISC38", "MISC39", "MISC40", "MISC41", "MISC42",
  "MISC43", "MISC44", "MISC45", "MISC46", "MISC47", "MISC48",
  "MISC49", "MISC50", "MISC51", "MISC52", "MISC53", "MISC54",
  "MISC55", "MISC56", "MISC57", "MISC58", "MISC59", "MISC60",
  "MISC61", "MISC62", "MISC63", "MISC64", "MISC65", "MISC66",
  "MISC67", "MISC68", "MISC69", "MISC70", "MISC71", "MISC72",
  "MISC73", "MISC74", "MISC75", "MISC76", "MISC77", "MISC78",
  "MISC79", "MISC80", "MISC81", "MISC82", "MISC83", "MISC84",
  "MISC85", "MISC86", "PUSH", "PULL", "DOGS", "PLASMA1",
  "PLASMA2", "SCEPTRE", "BIBLE", "MUSICSOURCE", "GIBDTH",
  "BLUEBLOOD", "GREENBLOOD", "FUZZYBLOOD", "TRAIL",
  NULL
};

/* -------------------------------------------------------------------------------------------- */

static mobjtype_t get_mobj_num (char * mobj_name, char * filename, unsigned int line_number)
{
  char buffer [32];
  int i;
  char j;
  int counter;
  mobjtype_t num;
  char * src;
  const char * const * cmp;

  i = 0;
  do
  {
    j = mobj_name [i];
    if ((j == ':') || (i > 30))
      j = 0;

    buffer [i++] = j;
  } while (j);


  src = buffer;

  if (strncasecmp (buffer, "SPR_", 4) == 0)
  {
    src += 4;
    cmp = (const char * const *) sprnames;
    counter = 0;
    do
    {
      if ((cmp [counter] != NULL)
       && (strcasecmp (src, cmp [counter]) == 0))
      {
	num = G_SprNum ((spritenum_t) counter);
	if (num != MT_NULL)
	  return (num);
      }
    } while (++counter < NUMSPRITES);
  }
  else
  {
    if (strncasecmp (buffer, "MT_", 3) == 0)
    {
      src += 3;
      counter = 0;
      cmp = cast_mtconv;
      do
      {
        if (strcasecmp (src, cmp [counter]) == 0)
	  return ((mobjtype_t)counter);
        counter++;
      } while (cmp[counter]);
    }
    else
    {
      counter = 0;
      do
      {
        if (strcasecmp (src, mobjinfo [counter].names[0]) == 0)
	  return ((mobjtype_t)counter);
        counter++;
      } while (counter < NUMMOBJTYPES);
      counter = 0;
      do
      {
        if (strcasecmp (src, mobjinfo [counter].names[1]) == 0)
	  return ((mobjtype_t)counter);
        counter++;
      } while (counter < NUMMOBJTYPES);
    }
  }

  fprintf (stderr, "Failed to match %s at line %d of file %s\n",
	    		buffer, line_number, filename);

  return (MT_NULL);
}

/* -------------------------------------------------------------------------------------------- */

static void set_mapname (char * name, char ** ptr)
{
  char * f;
  do
  {
  } while ((*name) && (*name++ != ':'));

  if (*name)
  {
    f = malloc (strlen(name)+1);
    if (f)
    {
      strcpy (f, name);
      *ptr = f;
    }
  }

  mapnameschanged = (boolean)((int)mapnameschanged|(int)dh_changing_pwad);
}

/* -------------------------------------------------------------------------------------------- */

static void G_process_hustr_line (char * a_line)
{
  unsigned int i,j;

  if (strncasecmp (a_line, "HUSTR_E", 7) == 0)
  {
    if (a_line [8] == 'M')
    {
      i = a_line [7] - '0';
      j = a_line [9] - '0';
      if ((i < 10)
       && (j < 10))
      {
	set_mapname (a_line, G_AccessMapname_E (i,j));
      }
    }
  }
  else if (strncasecmp (a_line, "HUSTR_", 6) == 0)
  {
    j = atoi (&a_line [6]);
    set_mapname (a_line, G_AccessMapname_E (255,j));
  }
  else if (strncasecmp (a_line, "PHUSTR_", 7) == 0)
  {
    j = atoi (&a_line [7]);
    set_mapname (a_line, G_AccessMapname_E (254,j));
  }
  else if (strncasecmp (a_line, "THUSTR_", 7) == 0)
  {
    j = atoi (&a_line [7]);
    set_mapname (a_line, G_AccessMapname_E (253,j));
  }
}

/* -------------------------------------------------------------------------------------------- */

void G_ParseMapSeq (char * filename, FILE * fin, int docheck)
{
  char a_line [256];
  unsigned int line_number;
  unsigned int i,j;
  unsigned int pos;
  map_dests_t *   map_info_p;
  item_to_drop_t * drop_info_p;

  if (fin)
  {
    line_number = 0;

    if (docheck)
    {
      /* First line just tells us it's a map sequence file */
      dh_fgets (a_line, 250, fin);
      line_number++;

      if (dh_strcmp (a_line,"# Map sequence file"))
      {
	fprintf (stderr, "File %s is an invalid map sequence file\n", filename);
	return;
      }
    }


    do
    {
      dh_fgets (a_line, 250, fin);
      line_number++;

      map_info_p = 0;
      drop_info_p = 0;

      switch (a_line [0])
      {
	case 'E':
	case 'e':
	  switch (a_line [2])
	  {
	    case 'M':
	    case 'm':
	      i = a_line [1] - '0'; // Episode
	      j = a_line [3];	    // Map
	      if ((j == 'S') || (j == 's'))	// Start link?
	      {
		map_info_p = (map_dests_t*) G_Access_MapStartTab_E (i);
	      }
	      else
	      {
		j -= '0';
		map_info_p = G_Access_MapInfoTab_E (i, j);
	      }
	  }
	  break;

	case 'M':
	case 'm':
	  if (strcasecmp (a_line, "MAPINFO") == 0)
	  {
	    Change_To_Mapinfo (fin);
	  }
	  else
	  {
	    if ((a_line [1] == 'S') || (a_line [1] == 's'))
	    {
	      map_info_p = (map_dests_t*) G_Access_MapStartTab_E (255);
	    }
	    else
	    {
	      j = atoi (&a_line [1]);
	      map_info_p = G_Access_MapInfoTab_E (255, j);
	    }
	  }
	  break;

	case 'U':
	case 'u':
	  if (strcasecmp (a_line, "UMAPINFO") == 0)
	  {
	    Change_To_UMapinfo (fin);
	  }
	  break;

	case 'D':
	case 'd':
	  if (strcasecmp(a_line, "DEHACKED") == 0)
	  {
	    DH_parse_hacker_file_f (filename, fin, ~0);
	    break;
	  }
	  i = get_mobj_num (&a_line [1], filename, line_number);
	  if (i != -1)
	  {
	    pos = 0;		/* If there's a - sign after the colon  */
	    do			/* then I need to find an existing slot */
	    {			/* to clear it, otherwise the user is   */
	      j = a_line [pos];	/* adding a new entry */
	      if (j == ':')
		j = 0;
	      pos++;
	    } while (j);

	    if (a_line [pos] == '-')
	    {
    	      drop_info_p = item_drop_head;
	      while (drop_info_p)
	      {
		if (drop_info_p -> just_died == i)
		  break;
		drop_info_p = drop_info_p -> next;
	      }
	    }
	    else
	    {
    	      drop_info_p = item_drop_head;
	      while (drop_info_p)
	      {
		if (drop_info_p -> just_died == (mobjtype_t) -2)
		{
		  drop_info_p -> just_died = (mobjtype_t) i;
		  break;
		}
		drop_info_p = drop_info_p -> next;
	      }
	      if (drop_info_p == NULL)
	      {
		drop_info_p = malloc (sizeof (item_to_drop_t));
		if (drop_info_p)
		{
		  drop_info_p -> next = item_drop_head;
		  item_drop_head = drop_info_p;
		  drop_info_p -> just_died = (mobjtype_t) i;
		}
	      }
	    }
	  }
	  break;

	  case 'P':
	  case 'p':
	    if (strncasecmp (a_line, "PATCH ", 6) == 0)
	    {
	      if (strncasecmp (a_line+6, "SWITCHES ", 9) == 0)
	      {
		P_PatchSwitchList (a_line + 15);
	      }
	      else if (strncasecmp (a_line+6, "SKY ", 4) == 0)
	      {
		R_PatchSky (a_line + 10);
	      }
	      else if (strncasecmp (a_line+6, "ORIGSCREENWIDTH ", 16) == 0)
	      {
		ORIGSCREENWIDTH = atoi (a_line+6+16);
	      }
	      else if (strncasecmp (a_line+6, "ORIGSCREENHEIGHT ", 17) == 0)
	      {
		ORIGSCREENHEIGHT = atoi (a_line+6+17);
	      }
	      else if (strncasecmp (a_line+6, "MTF_MASK ", 9) == 0)
	      {
		mtf_mask = (unsigned int) strtol (a_line+6+9, NULL, 0);
	      }
	      else
	      {
		map_patch_t * ptr;

		ptr = malloc (sizeof (map_patch_t) + strlen (a_line));
		if (ptr)
		{
		  ptr -> next = map_patches_head;
		  map_patches_head = ptr;
		  strcpy (ptr -> patch, a_line+6);
		}
	      }
	      break;
	    }

	  case 'H':
	  case 'h':
	  case 'T':
	  case 't':
	    if (dh_instr (a_line, "HUSTR_"))
	      G_process_hustr_line (a_line);
      }


      if (map_info_p)
      {
	pos = 0;
	do
	{
	  pos++;
	  while ((a_line [pos]) && (a_line [pos++] != ':'));

	  switch (a_line [pos])
	  {
	    case 'E':
	    case 'e':
	      switch (a_line [pos+2])
	      {
		case 'M':
		case 'm':
		  i = a_line [pos+1] - '0';
		  j = a_line [pos+3] - '0';
		  map_info_p -> normal_exit_to_episode = i;
		  map_info_p -> normal_exit_to_map = j;
		  break;
	      }
	      break;

	    case 'M':
	    case 'm':
	      j = atoi (&a_line [pos+1]);
	      map_info_p -> normal_exit_to_map = j;
	      break;

	    case 'I':
	    case 'i':
	      j = a_line [pos+1] - '0';
	      map_info_p -> intermission_text = j;
	      break;

	    case 'S':
	    case 's':
	      switch (a_line [pos+2])
	      {
		case 'M':
		case 'm':
		  i = a_line [pos+1] - '0';
		  j = a_line [pos+3] - '0';
		  map_info_p -> secret_exit_to_episode = i;
		  map_info_p -> secret_exit_to_map = j;
		  break;

		default:
		  j = atoi (&a_line [pos+1]);
		  map_info_p -> secret_exit_to_map = j;
	      }
	      break;

	    case 'P':
	    case 'p':
	      j = atoi (&a_line [pos+1]);
	      j = (j+4)/5;
	      if (j > 255) j = 255;
	      map_info_p -> par_time_5 = j;
	      par_changed = (boolean)((int)par_changed|(int)dh_changing_pwad);
	      break;

	    case 'R':
	    case 'r':
	      if (isdigit(a_line [pos+1]))
		map_info_p -> reset_kit_etc_on_entering = atoi (&a_line [pos+1]);
	      else
		map_info_p -> reset_kit_etc_on_entering = 3;
	      break;

	    case '0':
	    case '1':
	    case '2':
	      i = a_line [pos] - '0';
	      map_info_p -> this_is_a_secret_level = i;
	      break;
	  }

	} while (a_line [pos]);
      }


      if (drop_info_p)
      {
	pos = 0;
	do
	{
	  j = a_line [pos];
	  if (j == ':')
	    j = 0;
	  pos++;
	} while (j);

	switch (a_line [pos])
	{
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    i = atoi (&a_line [pos]);
	    break;

	  case '-':
	    i = -1;
	    break;

	  default:
	    i = get_mobj_num (&a_line [pos], filename, line_number);
	}

	drop_info_p -> mt_spawn = (mobjtype_t) i;
	if ((int)i < 0)  /* Release the storage if doing nothing */
	  drop_info_p -> just_died = (mobjtype_t) -2;
      }

    } while (!feof(fin));
  }

// Testing code

#if 0
  printf ("# Map sequence file\n");

  i = 1;

  map_info_p = G_Access_MapInfoTab_E (1,0);
  do
  {
    j = 0;
    do
    {
      printf ("E%dM%d:E%dM%d:S%dM%d:P%d:R%d:I%d:%d:%d:%d\n",
	i,j,
	map_info_p -> normal_exit_to_episode,
	map_info_p -> normal_exit_to_map,
	map_info_p -> secret_exit_to_episode,
	map_info_p -> secret_exit_to_map,
	map_info_p -> par_time_5,
	map_info_p -> reset_kit_etc_on_entering,
	map_info_p -> intermission_text,
	map_info_p -> this_is_a_secret_level,
	map_info_p -> cluster,
	map_info_p -> flags);
      map_info_p++;
      j++;
    } while (j < 10);
    i++;
  } while (i < 6);

  map_info_p = G_Access_MapInfoTab_E (255,0);
  i = 0;
  do
  {
    if ((map_info_p -> normal_exit_to_episode == 255)
     && (map_info_p -> secret_exit_to_episode == 255))
    {
      printf ("M%d:M%d:S%d:P%d:R%d:I%d:%d:%d\n",
	i,
	map_info_p -> normal_exit_to_map,
	map_info_p -> secret_exit_to_map,
	map_info_p -> par_time_5,
	map_info_p -> reset_kit_etc_on_entering,
	map_info_p -> intermission_text,
	map_info_p -> this_is_a_secret_level,
	map_info_p -> cluster);
    }
    else
    {
      printf ("M%d:M%d,%d:S%d,%d:P%d:R%d:I%d:%d:%d\n",
	i,
	map_info_p -> normal_exit_to_episode,
	map_info_p -> normal_exit_to_map,
	map_info_p -> secret_exit_to_episode,
	map_info_p -> secret_exit_to_map,
	map_info_p -> par_time_5,
	map_info_p -> reset_kit_etc_on_entering,
	map_info_p -> intermission_text,
	map_info_p -> this_is_a_secret_level,
	map_info_p -> cluster);
    }
    map_info_p++;
    i++;
  } while (i < ARRAY_SIZE(next_level_tab_2));

  printf ("item_drop_head = %X\n", item_drop_head);
  drop_info_p = item_drop_head;
  while (drop_info_p)
  {
    if ((drop_info_p -> just_died) == -2)
    {
      printf ("D:Unused slot\n");
    }
    else
    {
      printf ("DMT_%s:MT_%s\n",	cast_mtconv [drop_info_p -> just_died],
      				cast_mtconv [drop_info_p -> mt_spawn]);
    }
    drop_info_p = drop_info_p -> next;
  }

  i = 0;
  do
  {
    printf ("Thing %u (%s) (%s)\n", i, cast_mtconv[i], thing_names[i]);
  } while (++i < NUMMOBJTYPES);
#endif
}

/* -------------------------------------------------------------------------------------------- */

void G_ReadHstFile (char * filename)
{
  FILE * fin;

  fin = fopen (filename, "r");
  if (fin == 0)
  {
    fprintf (stderr, "Couldn't open file %s for reading\n", filename);
    return;
  }

  G_ParseMapSeq (filename, fin, 0);
  fclose (fin);
}

/* -------------------------------------------------------------------------------------------- */

void G_ReadMapSeq (char * filename)
{
  FILE * fin;

  fin = fopen (filename, "r");
  if (fin == 0)
  {
    fprintf (stderr, "Couldn't open file %s for reading\n", filename);
    return;
  }

  G_ParseMapSeq (filename, fin, 1);
  fclose (fin);
}

/* -------------------------------------------------------------------------------------------- */

const char * leafname (const char * path)
{
  char cc;
  const char * ptr;

  ptr = path;
  do
  {
    cc = *ptr++;
    if (cc == DIRSEPC)
      path = ptr;
  } while (cc);

  return (path);
}

/* -------------------------------------------------------------------------------------------- */

void dirname (char * dest, const char * path)
{
  char cc;
  char * ptr_sep;
  char * ptr_dest;

  ptr_sep = NULL;
  ptr_dest = dest;

  do
  {
    cc = *path++;
    if (cc == DIRSEPC)
      ptr_sep = ptr_dest;
    *ptr_dest++ = cc;
  } while (cc);

  if (ptr_sep == NULL)
  {
    ptr_sep = dest;
#ifdef __riscos
    *ptr_sep++ = '@';
#else
    *ptr_sep++ = '.';
#endif
  }
  *ptr_sep = 0;
}

/* -------------------------------------------------------------------------------------------- */
/*
   We use ".." in the filename to represent a "match to here" marker.
*/

static boolean scan_filematch (const char * filename, const char * searchname)
{
  unsigned int dotpos;
  unsigned int extpos1;
  unsigned int extpos2;

  if (strcasecmp (filename, searchname) == 0)
    return (true);

  dotpos = dh_rinchar (filename, EXTSEPC);

  if ((dotpos == 0)
   || (filename [dotpos-2] != EXTSEPC))
    return (false);

  if (strncasecmp (filename, searchname, dotpos-2) != 0)
    return (false);

  /* Filename part matches, need to check the extension */

  extpos1 = dh_rinchar (filename, EXTSEPC);
  if (extpos1 == 0)
    return (false);

  extpos2 = dh_rinchar (searchname, EXTSEPC);
  if (extpos2 == 0)
    return (false);

  if (strcasecmp (&filename[extpos1-1], &searchname[extpos2-1]) == 0)
    return (true);

  return (false);
}

/* -------------------------------------------------------------------------------------------- */

unsigned int scan_dir (char * dirname, char * filename, boolean do_it, boolean allow_recurse)
{
  unsigned int qty_done;
  unsigned char last;
  unsigned int null_pos;
  FILE * fin;
  DIR * dirp;
  struct dirent *dp;
  struct stat file_stat;

  null_pos = strlen (filename);
  last = filename [null_pos-1];
  qty_done = 0;

  dirp = opendir (dirname);
  if (dirp == 0)
  {
    // fprintf (stderr,"Couldn't open directory %s for scanning\n", dirname);
  }
  else
  {
    null_pos = strlen (dirname);	/* Remember where the null is */

    while ((dp = readdir (dirp)) != NULL)
    {
      /* Miss out . & .. */
      if ((strcmp (dp -> d_name, ".") != 0)
       &&  (strcmp (dp -> d_name, "..") != 0))
      {
	dirname [null_pos] = DIRSEPC;
	strcpy (dirname+null_pos+1, dp -> d_name);

	if (stat (dirname, &file_stat) == 0)
	{
	  if (file_stat.st_mode & S_IFDIR)
	  {
	    if (allow_recurse)
	      qty_done += scan_dir (dirname, filename, do_it, allow_recurse);
	  }
	  else if (scan_filematch (dp -> d_name, filename) == true)
	  {
	    fin = fopen (dirname, "r");
	    if (fin)
	    {
	      if (do_it == true)
	      {
		switch (last)
		{
		  case 'q':				// msq
		    G_ParseMapSeq (dirname, fin, 1);
		    break;
		  case 't':				// hst
		    G_ParseMapSeq (dirname, fin, 0);
		    break;
		  case 'x':				// bex
		  case 'h':				// deh
		    DH_parse_hacker_file_f (dirname, fin, ~0);
		    break;
		  default:
		    fprintf (stderr, "Unknown file extension - %s\n", dirname);
		}
	      }
	      else
	      {
		printf (" adding %s\n", dirname);
	      }
	      fclose (fin);
	      qty_done++;
	    }
	  }
	}
      }
    }

    dirname [null_pos] = 0;		/* And chop it off again */
    closedir (dirp);
  }

  return (qty_done);
}

/* -------------------------------------------------------------------------------------------- */

// #define WIPE_UNTESTED_DIR

#ifdef WIPE_UNTESTED_DIR

static void G_wipe_untested_dir (char * dirname)
{
  unsigned int null_pos;
  DIR * dirp;
  struct dirent *dp;
  struct stat file_stat;

  dirp = opendir (dirname);
  if (dirp == 0)
  {
    // fprintf (stderr,"Couldn't open directory %s for scanning\n", dirname);
  }
  else
  {
    null_pos = strlen (dirname);	/* Remember where the null is */

    while ((dp = readdir (dirp)) != NULL)
    {
      /* Miss out . & .. */
      if ((strcmp (dp -> d_name, ".") != 0)
       &&  (strcmp (dp -> d_name, "..") != 0))
      {
	dirname [null_pos] = DIRSEPC;
	strcpy (dirname+null_pos+1, dp -> d_name);

	if (stat (dirname, &file_stat) == 0)
	{
	  if (file_stat.st_mode & S_IFDIR)
	  {
	    G_wipe_untested_dir (dirname);
	  }
	  else
	  {
	    remove (dirname);
	  }
	}
      }
    }

    dirname [null_pos] = 0;		/* And chop it off again */
    closedir (dirp);
    remove (dirname);
  }
}

#endif

/* -------------------------------------------------------------------------------------------- */

void G_parse_map_seq_wad_file (const char * wadfile, const char * extension, boolean do_it)
{
  char aline [48];
  char msqname [250];

  /* Change /WAD to /MSQ */

  dirname (msqname, wadfile);
  DH_replace_file_extension (aline, leafname(wadfile), extension);

  if (scan_dir (msqname, aline, do_it, false) == 0)
  {
    dirname (msqname, myargv [0]);
    strcat (msqname, DIRSEP"pwads"DIRSEP"Database");
#ifdef WIPE_UNTESTED_DIR
    {
      unsigned int null_pos;

      null_pos = strlen (msqname);
      strcat (msqname, DIRSEP"Untested");
      G_wipe_untested_dir (msqname);
      msqname [null_pos] = 0;
    }
#endif
    scan_dir (msqname, aline, do_it, true);
  }
}

/* -------------------------------------------------------------------------------------------- */
/*
  Read patches from the .msq file.

  Examples:
    Patch E1M5 Sector 14 Tag=12
    Patch Map01 Line 1555 Special=10
    Patch Map01 GenDoors Tag=0
*/

void G_Patch_Map (void)
{
  unsigned int pos;
  unsigned int index;
  unsigned int patch;
  map_patch_t * ptr;
  char * pp;
  char * pq;
  line_t * line;
  sector_t * sector;

  ptr = map_patches_head;
  if (ptr)
  {
    do
    {
      pp = ptr -> patch;

      if (gamemode == commercial)
      {
	if (strncasecmp (pp, "map", 3))
	  continue;
	if (atoi (pp+3) != gamemap)
	  continue;
	pp += 5;
      }
      else
      {
	if ((pp [0] != 'E')
	 || (pp [2] != 'M')
	 || (pp [4] != ' ')
	 || ((pp [1] - '0') != gameepisode)
	 || ((pp [3] - '0') != gamemap))
	  continue;
	pp += 4;
      }

      pos = dh_inchar (pp, '=');
      if (pos == 0)
	continue;

      pq = pp + pos;
      while (*pq == ' ') pq++;
      patch = (unsigned int) strtol (pq, NULL, 0);
      while (*pp == ' ') pp++;
      if (strncasecmp (pp, "LINE ",5) == 0)
      {
	pp += 5;
	index = atoi (pp);
	if (index < numlines)
	{
	  while (*pp && (*pp != ' ')) pp++;
	  while (*pp == ' ') pp++;
	  line = &lines [index];
	  if (strncasecmp (pp, "TAG", 3) == 0)
	  {
//	    printf ("Line %u Tag set to %u\n", index, patch);
	    line -> tag = patch;
	  }
	  else if (strncasecmp (pp, "FLAGS", 5) == 0)
	  {
	    line -> flags = patch;
	  }
	  else if (strncasecmp (pp, "SPECIAL", 7) == 0)
	  {
	    line -> special = patch;
	  }
	}
      }

      if (strncasecmp (pp, "SECTOR ", 7) == 0)
      {
	pp += 7;
	index = atoi (pp);
	if (index < numsectors)
	{
	  while (*pp && (*pp != ' ')) pp++;
	  while (*pp == ' ') pp++;
	  sector = &sectors [index];
	  if (strncasecmp (pp, "TAG", 3) == 0)
	  {
	    sector -> tag = patch;
	  }
	  else if (strncasecmp (pp, "SPECIAL", 7) == 0)
	  {
	    sector -> special = patch;
	  }
	  else if (strncasecmp (pp, "CEILING", 7) == 0)
	  {
	    sector -> ceilingheight = patch << FRACBITS;
	  }
	  else if (strncasecmp (pp, "FLOOR", 5) == 0)
	  {
	    sector -> floorheight = patch << FRACBITS;
	  }
	}
      }

      if (strncasecmp (pp, "GENDOORS ",9) == 0)
      {
	while (*pp && (*pp != ' ')) pp++;
	while (*pp == ' ') pp++;
	index = 0;
	line = &lines [0];
	do
	{
	  if ((line -> special >= GenLockedBase)
	   && (line -> special < GenCeilingBase))
	  {
	    if (strncasecmp (pp, "TAG", 3) == 0)
	    {
//	      printf ("Line %u Tag set to %u from %u\n", index, patch, line->tag);
	      line -> tag = patch;
	    }
	    else if (strncasecmp (pp, "SPECIAL", 7) == 0)
	    {
	      line -> special = patch;
	    }
	  }
	  line++;
	} while (++index < numlines);
      }


    } while ((ptr = ptr -> next) != NULL);
  }
}

/* -------------------------------------------------------------------------------------------- */
/*
  Read patches from the .msq file.

  Examples:
    Patch E1M5 Thing 14 options=12
*/

void G_Patch_Map_Things (int thingnumber, mapthing_t * mt)
{
  unsigned int pos;
  unsigned int index;
  unsigned int patch;
  map_patch_t * ptr;
  char * pp;
  char * pq;

  ptr = map_patches_head;
  if (ptr)
  {
    do
    {
      pp = ptr -> patch;

      if (gamemode == commercial)
      {
	if (strncasecmp (pp, "map", 3))
	  continue;
	if (atoi (pp+3) != gamemap)
	  continue;
	pp += 5;
      }
      else
      {
	if ((pp [0] != 'E')
	 || (pp [2] != 'M')
	 || (pp [4] != ' ')
	 || ((pp [1] - '0') != gameepisode)
	 || ((pp [3] - '0') != gamemap))
	  continue;
	pp += 4;
      }

      while (*pp == ' ') pp++;
      if (strncasecmp (pp, "THING ",6) == 0)
      {
	pp += 6;
	index = atoi (pp);
	if (index == thingnumber)
	{
	  pos = dh_inchar (pp, '=');
	  if (pos == 0)
	    continue;

	  pq = pp + pos;
	  while (*pq == ' ') pq++;
	  patch = (unsigned int) strtol (pq, NULL, 0);

	  while (*pp && (*pp != ' ')) pp++;
	  while (*pp == ' ') pp++;

	  if (strncasecmp (pp, "X", 1) == 0)
	  {
	    mt -> x = patch;
	  }
	  else if (strncasecmp (pp, "Y", 1) == 0)
	  {
	    mt -> y = patch;
	  }
	  else if (strncasecmp (pp, "ANGLE", 5) == 0)
	  {
	    mt -> angle = patch;
	  }
	  else if (strncasecmp (pp, "TYPE", 4) == 0)
	  {
	    mt -> type = patch;
	  }
	  else if (strncasecmp (pp, "OPTIONS", 7) == 0)
	  {
	    mt -> options = patch;
	  }
	}
      }
    } while ((ptr = ptr -> next) != NULL);
  }
}

