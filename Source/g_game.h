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
//   Duh.
//
//-----------------------------------------------------------------------------


#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"
#include "info.h"


typedef enum
{
  GG_SAVEGAMENAME,
  GG_GGSAVED
} save_g_msg_texts_t;

//
// GAME
//
typedef struct
{
  unsigned char normal_exit_to_episode;
  unsigned char normal_exit_to_map;
  unsigned char secret_exit_to_episode;
  unsigned char secret_exit_to_map;
  unsigned char this_is_a_secret_level;
  unsigned char reset_kit_etc_on_entering;
  unsigned char intermission_text;
  unsigned char	cluster;
  unsigned char	nointermission;
  unsigned char allow_monster_telefrags;
  unsigned char par_time_5;			// Par time divided by 5
  unsigned char	time_sucks;			// Par time for sucks in minutes
  fixed_t	skydelta;
  char *	mapname;
  char *	sky;
  char *	titlepatch;
  char *	enterpic;
  char *	exitpic;
  char *	bordertexture;
} map_dests_t;

typedef struct
{
  unsigned char  start_episode;
  unsigned char  start_map;
} map_starts_t;

typedef struct item_to_drop_s
{
  mobjtype_t	 just_died;
  mobjtype_t	 mt_spawn;
  struct item_to_drop_s * next;
} item_to_drop_t;

extern boolean par_changed;

void G_DeathMatchSpawnPlayer (int playernum);

void G_InitNew (skill_t skill, int episode, int map);

// Can be called by the startup code or M_Responder.
// A normal game starts at map 1,
// but a warp test can start elsewhere
void G_DeferedInitNew (skill_t skill, int episode, int map);
void G_DeferedInitNewLater (skill_t skill, int episode, int map);

void G_DeferedPlayDemo (char* demo);

// Can be called by the startup code or M_Responder,
// calls P_SetupLevel or W_EnterWorld.
void G_LoadGame (char* name);

void G_DoLoadGame (void);

// Called by M_Responder.
void G_SaveGame (int slot, char* description);
void G_GetSaveGameName (char * name, int i);

// Only called by startup code.
void G_RecordDemo (char* name);

void G_BeginRecording (void);

void G_PlayDemo (char* name);
void G_TimeDemo (char* name);
boolean G_CheckDemoStatus (void);

void G_ExitLevel (void);
void G_SecretExitLevel (void);
void G_ExitLevellater (void);
void G_SecretExitLevellater (void);

void G_WorldDone (void);
void G_WorldDone2 (void);

void G_Ticker (void);
boolean G_Responder (event_t*	ev);

void G_ScreenShot (void);

map_dests_t * G_Access_MapInfoTab (unsigned int episode, unsigned int map);
map_dests_t * G_Access_MapInfoTab_E (unsigned int episode, unsigned int map);
map_starts_t * G_Access_MapStartTab (unsigned int episode);
map_starts_t * G_Access_MapStartTab_E (unsigned int episode);
void G_MapName (char * name, int episode, int map);
int G_MapLump (int episode, int map);
void G_ParseMapSeq (char * filename, FILE * fin, int docheck);
void G_ReadMapSeq (char * filename);
void G_parse_map_seq_wad_file (char * wadfile, boolean do_it);
const char * leafname (const char * path);
void dirname (char * dest, const char * path);
void scan_dir (char * dirname, char * filename, boolean do_it);
void G_Patch_Map (void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
