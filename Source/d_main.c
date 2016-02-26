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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: d_main.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";
#endif

#include "includes.h"

#define IWAD_MAGIC	0x44415749
#define PWAD_MAGIC	0x44415750

/* ----------------------------------------------------------- */

char * dmain_messages_orig [] =
{
		D_DEVSTR,
		D_CDROM,
		 "CD-ROM Version: doomrc from <Choices$Dir>.dsg\n",
		 "                         "
		 "The Ultimate DOOM Startup v%i.%i"
		 "                           ",
		 "                            "
		 "DOOM Shareware Startup v%i.%i"
		 "                           ",
		 "                            "
		 "DOOM Registered Startup v%i.%i"
		 "                           ",
		 "                         "
		 "DOOM 2: Hell on Earth v%i.%i"
		 "                           ",
		 "                   "
		 "DOOM 2: Plutonia Experiment v%i.%i"
		 "                           ",
		 "                     "
		 "DOOM 2: TNT - Evilution v%i.%i"
		 "                           ",
		 "                     "
		 "Public DOOM - v%i.%i"
		 "                           ",
		 "                          "
		 "DOOM System Startup v%i.%i"
		 "                          ",
	    "===========================================================================\n"
	    "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
	    "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
	    "        You will not receive technical support for modified games.\n"
	    "                      press enter to continue\n"
	    "===========================================================================\n",
	    "===========================================================================\n"
	    "                                Shareware!\n"
	    "===========================================================================\n",
	    "===========================================================================\n"
	    "                 Commercial product - do not distribute!\n"
	    "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
	    "===========================================================================\n",
	    "===========================================================================\n"
	    "             This version is NOT SHAREWARE, do not distribute!\n"
	    "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
	    "===========================================================================\n",
	    "===========================================================================\n"
	    "                            Do not distribute!\n"
	    "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
	    "===========================================================================\n",
		NULL
};

char * dmain_messages [ARRAY_SIZE(dmain_messages_orig)];
char * startup_messages [10];

/* ----------------------------------------------------------- */

enum
{
  DN_DEMO1,
  DN_DEMO2,
  DN_DEMO3,
  DN_DEMO4
};

char * demo_names_orig [] =
{
  "DEMO1",
  "DEMO2",
  "DEMO3",
  "DEMO4",
  NULL
};

char * demo_names [ARRAY_SIZE(demo_names_orig)];

/* ----------------------------------------------------------- */
/* Strings from f_finale.c */
extern char*	finale_messages[];
extern char*	finale_messages_orig[];
extern char*	finale_backdrops[];
extern char*	finale_backdrops_orig[];
extern char*	cast_names_copy[];
extern castinfo_t castorder[];

/* Strings from info.c */
extern char * sprnames [];
extern char * sprnames_orig[];

/* Strings from p_inter.c */
extern char * got_messages [];
extern char * got_messages_orig [];

/* Strings from m_menu.c */
extern char * menu_messages [];
extern char * menu_messages_orig [];
extern char * endmsg [];
extern char * endmsg_orig [];

/* Strings from m_misc.c */
extern char * screenshot_messages [];
extern char * screenshot_messages_orig [];

/* Strings from p_doors.c */
extern char * door_messages [];
extern char * door_messages_orig [];

/* Strings from g_game.c */
extern const char enterpic_2 [];
extern char * save_game_messages [];
extern char * save_game_messages_orig [];

/* Strings from am_map.c */
extern char * am_map_messages [];
extern char * am_map_messages_orig [];

/* Strings from hu_stuff.c */
extern char* chat_macros[];
extern char* chat_macros_orig[];
extern char* player_names[];
extern char* player_names_orig[];

/* Strings from st_stuff.c */
extern char * stat_bar_messages [];
extern char * stat_bar_messages_orig [];

/* Strings from p_spec.c */
extern char * special_effects_messages [];
extern char * special_effects_messages_orig [];

/* Strings from sounds.c */
extern char * music_names_copy [];
extern char * sound_names_copy [];

/* Pointers from dh_stuff.c */
extern actionf_t states_ptr_copy [NUMSTATES];

/* ----------------------------------------------------------- */

//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop (void);
void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);


#define	MAXWADFILES 20
static char * wadfiles[MAXWADFILES];


boolean	devparm;	// started game with -devparm
boolean nomonsters1;	// checkparm of -nomonsters
boolean nomonsters2;	// checkparm of -nomonsters
boolean respawnparm;	// checkparm of -respawn
boolean fastparm;	// checkparm of -fast

boolean drone;

boolean	singletics = false; // debug flag to cancel adaptiveness



//extern int soundVolume;
//extern  int	sfxVolume;
//extern  int	musicVolume;

extern boolean	inhelpscreens;
extern boolean	Monsters_Infight2;
extern boolean	Give_Max_Damage;
extern boolean  nobrainspitcheat;
extern boolean	dh_changing_pwad;

skill_t		startskill;
int	     	startepisode;
int		startmap;
boolean		autostart;
FILE*		debugfile;
boolean		advancedemo;
char		basedefaultdir[1024];	// default file directory




//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
event_t	events[MAXEVENTS];
int	eventhead;
int	eventtail;


static int	demosequence;
static int	pagetic;
static char	*pagename;

/* ----------------------------------------------------------- */
//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (event_t* ev)
{
    events[eventhead] = *ev;
    eventhead = (eventhead+1)&(MAXEVENTS-1);
}


/* ----------------------------------------------------------- */
//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
    event_t*	ev;

    // IF STORE DEMO, DO NOT ACCEPT INPUT
    //if ( ( gamemode == commercial )		Removed by JAD 14/11/99
    //	 && (W_CheckNumForName("map01")<0) )
    //  return;

    for ( ; eventtail != eventhead ; eventtail = (eventtail+1)&(MAXEVENTS-1) )
    {
	ev = &events[eventtail];
	if (M_Responder (ev))
	    continue;			// menu ate the event
	G_Responder (ev);
    }
}


//-----------------------------------------------------------------------------


//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t     wipegamestate = GS_DEMOSCREEN;
extern  int	showMessages;

//-----------------------------------------------------------------------------

static void init_a_text_block (char ** dest, char ** src)
{
  char * ptr;

  do
  {
    ptr = *src++;
    *dest++ = ptr;
  } while (ptr);
}

//-----------------------------------------------------------------------------
/*
  Copy all of the text strings from the master copy to the running copy.
  This is mainly so that the DEHACKED can still work after the Free Doom
  wads have changed the messages.
*/

static void init_text_messages (void)
{
  unsigned int count;
  char * name;
  char ** c_ptr;
  castinfo_t * ptr_s;
  musicinfo_t * m_ptr;
  sfxinfo_t * s_ptr;
  actionf_t * a_ptr;
  state_t * i_ptr;

  init_a_text_block (sprnames, sprnames_orig);
  init_a_text_block (dmain_messages, dmain_messages_orig);
  init_a_text_block (demo_names, demo_names_orig);
  init_a_text_block (finale_messages, finale_messages_orig);
  init_a_text_block (finale_backdrops, finale_backdrops_orig);
  init_a_text_block (got_messages, got_messages_orig);
  init_a_text_block (menu_messages, menu_messages_orig);
  init_a_text_block (screenshot_messages, screenshot_messages_orig);
  init_a_text_block (endmsg, endmsg_orig);
  init_a_text_block (door_messages, door_messages_orig);
  init_a_text_block (save_game_messages, save_game_messages_orig);
  init_a_text_block (am_map_messages, am_map_messages_orig);
  init_a_text_block (chat_macros, chat_macros_orig);
  init_a_text_block (player_names, player_names_orig);
  init_a_text_block (stat_bar_messages, stat_bar_messages_orig);
  init_a_text_block (special_effects_messages, special_effects_messages_orig);

  ptr_s = castorder;
  c_ptr = cast_names_copy;
  do
  {
    name = ptr_s -> name;
    ptr_s++;
    *c_ptr++ = name;
  } while (name);

  m_ptr = S_music;
  c_ptr = music_names_copy;

  name = m_ptr -> name;		// Do the first one separately
  m_ptr++;
  if (name == NULL)
    name = "e0m0\t";		// Something that cannot be matched!
  *c_ptr++ = name;

  do
  {
    name = m_ptr -> name;
    m_ptr++;
    *c_ptr++ = name;
  } while (name);

  s_ptr = S_sfx;
  c_ptr = sound_names_copy;
  do
  {
    name = s_ptr -> name;
    s_ptr++;
    *c_ptr++ = name;
  } while (name);


  // Initialise extra DeHacked states to 3999
  count = EXTRASTATES;
  i_ptr = &states [EXTRASTATES];
  do
  {
    i_ptr -> sprite = SPR_TNT1;
    i_ptr -> frame = 0;
    i_ptr -> tics = -1;
    i_ptr -> action.acv = NULL;
    i_ptr -> nextstate = (statenum_t) count;
    i_ptr -> misc1 = 0;
    i_ptr -> misc2 = 0;
    i_ptr++;
  } while (++count < NUMSTATES);


  a_ptr = states_ptr_copy;
  i_ptr = states;
  count = NUMSTATES;
  do
  {
    *a_ptr++ = i_ptr -> action;
    i_ptr++;
  } while (--count);
}

/* ----------------------------------------------------------- */

void D_Display (void)
{
    static  boolean		viewactivestate = false;
    static  boolean		menuactivestate = false;
    static  boolean		inhelpscreensstate = false;
    static  boolean		fullscreen = false;
    static  gamestate_t		oldgamestate = (gamestate_t)-1;
    static  int			borderdrawcount;
    unsigned int		nowtime;
    unsigned int		tics;
    unsigned int		wipestart;
    int				y;
    boolean			done;
    boolean			wipe;
    boolean			redrawsbar;

    if (nodrawers)
	return;			// for comparative timing / profiling

    redrawsbar = false;

    // change the view size if needed
    if (setsizeneeded)
    {
	R_ExecuteSetViewSize ();
	oldgamestate = (gamestate_t)-1;	// force background redraw
	borderdrawcount = 3;
    }

    // save the current screen if about to wipe
    if (gamestate != wipegamestate)
    {
	wipe = true;
	wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
    else
	wipe = false;

    if (gamestate == GS_LEVEL && gametic)
	HU_Erase();

    // do buffered drawing
    switch (gamestate)
    {
      case GS_LEVEL:
	if (!gametic)
	    break;
	if (automapactive)
	    AM_Drawer ();
	if (wipe || (viewheight != SCREENHEIGHT && fullscreen) )
	    redrawsbar = true;
	if (inhelpscreensstate && !inhelpscreens)
	    redrawsbar = true;		// just put away the help screen
	ST_Drawer ((boolean)(viewheight == SCREENHEIGHT), redrawsbar );
	fullscreen = (boolean)(viewheight == SCREENHEIGHT);
	break;

      case GS_INTERMISSION:
	WI_Drawer ();
	break;

      case GS_FINALE:
	F_Drawer ();
	break;

      case GS_DEMOSCREEN:
	D_PageDrawer (pagename);
	break;
    }

    // draw buffered stuff to screen
    I_UpdateNoBlit ();

    // draw the view directly
    if (gamestate == GS_LEVEL && !automapactive && gametic)
	R_RenderPlayerView (&players[displayplayer]);

    if (gamestate == GS_LEVEL && gametic)
	HU_Drawer ();

    // clean up border stuff
    if (gamestate != oldgamestate && gamestate != GS_LEVEL)
	I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));

    // see if the border needs to be initially drawn
    if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
	viewactivestate = false;	// view was not active
	R_FillBackScreen ();		// draw the pattern into the back screen
    }

    // see if the border needs to be updated to the screen
    if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != SCREENWIDTH)
    {
	if (menuactive || menuactivestate || !viewactivestate)
	    borderdrawcount = 3;
	if (borderdrawcount)
	{
	    R_DrawViewBorder ();	// erase old menu stuff
	    borderdrawcount--;
	}

    }

    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;

    // draw pause pic
    if (paused)
    {
	if (automapactive)
	    y = 4;
	else
	    y = viewwindowy+4;
	V_DrawPatchDirect(viewwindowx+(scaledviewwidth-68)/2,
			  y,0,W_CacheLumpName ("M_PAUSE", PU_CACHE));
    }


    // menus go directly to the screen
    M_Drawer ();		// menu is drawn even on top of everything
    NetUpdate ();		// send out any new accumulation


    // normal update
    if (!wipe)
    {
	I_FinishUpdate ();	// page flip or blit buffer
	return;
    }

    // wipe update
    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    wipestart = I_GetTime () - 1;

    do
    {
	do
	{
	    nowtime = I_GetTime ();
	    tics = nowtime - wipestart;
	} while (!tics);
	wipestart = nowtime;
	done = (boolean) wipe_ScreenWipe (wipe_Melt , 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
	I_UpdateNoBlit ();
	M_Drawer ();		// menu is drawn even on top of wipes
	I_FinishUpdate ();	// page flip or blit buffer
    } while (!done);
}


//-----------------------------------------------------------------------------
//
//  D_DoomLoop
//
extern  boolean demorecording;

void D_DoomLoop (void)
{
    if (demorecording)
      G_BeginRecording ();

    while (1)
    {
	// frame syncronous IO operations
	I_StartFrame ();

	// process one or more tics
	if (singletics)
	{
	    I_StartTic ();
	    D_ProcessEvents ();
	    G_BuildTiccmd (&netcmds[consoleplayer][maketic%BACKUPTICS]);
	    if (advancedemo)
		D_DoAdvanceDemo ();
	    M_Ticker ();
	    G_Ticker ();
	    gametic++;
	    maketic++;
	}
	else
	{
	    TryRunTics (); // will run at least one tic
	}

	S_UpdateSounds (players[consoleplayer].mo);// move positional sounds

	// Update display, next frame, with current state.
	D_Display ();

#ifndef SNDSERV
	// Sound mixing for the buffer is snychronous.
	I_UpdateSound();
#endif
	// Synchronous sound output is explicitly called.
#ifndef SNDINTR
	// Update sound output.
	I_SubmitSound();
#endif
    }
}

//-----------------------------------------------------------------------------
//
//  DEMO LOOP
//

//-----------------------------------------------------------------------------
//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
  if ((menuactive == false)
   && (--pagetic < 0))
    D_AdvanceDemo ();
}

//-----------------------------------------------------------------------------
//
// D_PageDrawer
//
void D_PageDrawer (const char * page)
{
  int lump;
  int attempts;
  patch_t * patch;

  attempts = 3;

  do
  {
    lump = W_CheckNumForName (page);

    /* FreeDoom daily build has a couple of corrupt patches */
    /* CREDIT is only 13 bytes and VICTORY2 is only 2538.   */

    if ((lump != -1)
     && (W_LumpLength (lump) >= 4096)
     && (SHORT((patch = W_CacheLumpNum (lump, PU_CACHE))->width) >= 320)
     && (SHORT(patch->height) >= 200))
    {
      V_DrawPatchScaled (0, 0, 0, patch);
      return;
    }

    // printf ("Patch %s does not exist or is too small\n", page);

    if (page == finale_backdrops[BG_TITLEPIC])
      page = enterpic_2;
    else
      page = finale_backdrops[BG_TITLEPIC];

  } while (--attempts);

  /* Cannot find a usable graphic, give up. */
  F_DrawBackgroundFlat (NULL);
}

//-----------------------------------------------------------------------------
//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
    advancedemo = true;
}


//-----------------------------------------------------------------------------
//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
 void D_DoAdvanceDemo (void)
{
    players[consoleplayer].playerstate = PST_LIVE;  // not reborn
    advancedemo = false;
    usergame = false;		// no save / end game here
    paused = false;
    gameaction = ga_nothing;

    if ( gamemode == retail )
      demosequence = (demosequence+1)%7;
    else
      demosequence = (demosequence+1)%6;

    switch (demosequence)
    {
      case 0:
	if ( gamemode == commercial )
	    pagetic = 35 * 11;
	else
	    pagetic = 170;
	gamestate = GS_DEMOSCREEN;
	pagename = finale_backdrops[BG_TITLEPIC];
	if ( gamemode == commercial )
	  S_StartMusic(mus_dm2ttl);
	else
	  S_StartMusic (mus_intro);
	break;
      case 1:
	G_DeferedPlayDemo (demo_names [DN_DEMO1]);
	break;
      case 2:
	pagetic = 200;
	gamestate = GS_DEMOSCREEN;
	pagename = finale_backdrops[BG_CREDIT];
	break;
      case 3:
	G_DeferedPlayDemo (demo_names [DN_DEMO2]);
	break;
      case 4:
	gamestate = GS_DEMOSCREEN;
	if ( gamemode == commercial)
	{
	    pagetic = 35 * 11;
	    pagename = finale_backdrops[BG_TITLEPIC];
	    S_StartMusic(mus_dm2ttl);
	}
	else
	{
	    pagetic = 200;

	    if ( gamemode == retail )
	      pagename = finale_backdrops[BG_CREDIT];
	    else
	      pagename = finale_backdrops[BG_HELP2];
	}
	break;
      case 5:
	G_DeferedPlayDemo (demo_names [DN_DEMO3]);
	break;
	// THE DEFINITIVE DOOM Special Edition demo
      case 6:
	G_DeferedPlayDemo (demo_names [DN_DEMO4]);
	break;
    }
}



//-----------------------------------------------------------------------------
//
// D_StartTitle
//
void D_StartTitle (void)
{
    gameaction = ga_nothing;
    demosequence = -1;
    D_AdvanceDemo ();
}


//-----------------------------------------------------------------------------
//
// D_AddFile
//
void D_AddFile (const char *file)
{
  char *newfile;
  char * q;
  char ** p;

  p = wadfiles;
  while ((q = *p) != NULL)
  {
    if (strcasecmp (q, file) == 0)
      return;				// Already in list
    p++;
  }

  newfile = malloc (strlen(file)+1);

  if (newfile == 0)
  {
    I_Error("Failed to claim memory for WAD file list\n");
  }
  else
  {
    strcpy (newfile, file);
    *p = newfile;
  }
}

//-----------------------------------------------------------------------------

static unsigned int Read_32 (FILE * fin)
{
  unsigned int a,b,c,d;

  a = fgetc (fin);
  b = fgetc (fin);
  c = fgetc (fin);
  d = fgetc (fin);
  return ((d << 24) | (c << 16) | (b << 8) | a);
}

//-----------------------------------------------------------------------------
#if 0
static boolean IsEntryPresent (FILE * fin, char * name,
			unsigned int cat_pos, unsigned int cat_size)
{
  char catname [16];
  unsigned int p;

  fseek (fin, (long int) cat_pos, SEEK_SET);
  do
  {
    p = 0;
    do
    {
      catname [p] = fgetc (fin);
      p++;
    } while (p < 16);

    if (strncasecmp(name, &catname[8], 8) == 0)
      return (true);

    cat_size--;
  } while (cat_size);
  return (false);
}
#endif
//-----------------------------------------------------------------------------
/*
  Try to work out whether this is a WAD that needs doom.wad, doomu.wad or doom2.wad

  Returns: 0 = Not found
  	   1 = doom1.wad
  	   2 = doom.wad
  	   3 = doomu.wad
  	   4 = doom2.wad
*/

static unsigned int IdentifyWad (const char * filename, unsigned int magic)
{
  FILE * fin;
  unsigned int cat_pos;
  unsigned int cat_size;
  unsigned int p;
  unsigned int rc;
  char catname [16];

  rc = 0;
  fin = fopen (filename, "rb");
  if (fin)
  {
    p = Read_32 (fin);
    if (p == magic)
    {
      cat_size = Read_32 (fin);
      cat_pos  = Read_32 (fin);

      /* Now try to identify the WAD from it's contents */

      fseek (fin, (long int) cat_pos, SEEK_SET);
      do
      {
	p = 0;
	do
	{
	  catname [p] = fgetc (fin);
	  p++;
	} while (p < 16);

	if ((catname [0+8] == 'E')
	 && (catname [2+8] == 'M')
	 && (catname [4+8] == 0)
	 && (catname [3+8] >= '0')
	 && (catname [3+8] <= '9')
	 && (catname [1+8] >= '0'))
	{
	  if (rc == 0) rc = 1;
	  if (catname [1+8] > '1')
	  {
	    if (rc < 2) rc = 2;
	    if (catname [1+8] > '3')
	    {
	      rc = 3;
	      break;
	    }
	  }
	}
	else
	if ((strncmp (catname+8, "MAP", 3) == 0)
	 && (catname [5+8] == 0)
	 && (catname [3+8] >= '0')
	 && (catname [3+8] <= '9')
	 && (catname [4+8] >= '0')
	 && (catname [4+8] <= '9'))
	{
	  rc = 4;
	  break;
	}
	cat_size--;
      } while (cat_size);
    }
    fclose (fin);
  }

  // printf ("WAD File %s identified as type %u\n", filename, rc);
  return (rc);
}

//-----------------------------------------------------------------------------

static void IdentifyIwad (const char * name)
{
  switch (IdentifyWad (name, IWAD_MAGIC))
  {
    case 1:
      gamemode = shareware;
      break;

    case 2:
      gamemode = registered;
      break;

    case 3:
      gamemode = retail;
      break;

    case 4:
      gamemode = commercial;
      gamemission = doom2;

      /* Need to differentiate plutonia and tnt here */
      /* For now just use the filename */

      if (dh_instr (name, "plutonia"))
	gamemission = pack_plut;

      if (dh_instr (name, "tnt"))
	gamemission = pack_tnt;
      break;

    default:
      I_Error("File %s is not an IWAD file", name);
      break;
  }

#if 0
  switch (gamemode)
  {
    case shareware:
      printf ("IWAD file is shareware\n");
      break;
    case registered:
      printf ("IWAD file is registered\n");
      break;
    case retail:
      printf ("IWAD file is retail\n");
      break;
    case commercial:
      printf ("IWAD file is commercial\n");
      break;
  }
#endif

  D_AddFile (name);
}

//-----------------------------------------------------------------------------

/* See if wadfile is present. Also try chopping
the extension off */
static int wad_present (char * filename)
{
  int rc;
  int st;
  char oc;
  FILE * iwad;
  unsigned int p;

  // printf ("wad_present (%s)\n", filename);

  rc = access (filename, R_OK);
  if (!rc)  return (rc);

  st = (strlen (filename)) - 4;
  if (st > 0)
  {
    if ((strcasecmp (filename+st , "/wad" ) == 0)
     || (strcasecmp (filename+st , ".wad" ) == 0))
    {
      oc = filename [st];
      filename [st] = 0;   /* Chop off "wad" */
      if (!access (filename, R_OK))
      {
	iwad = fopen (filename, "rb");
	if (iwad)
	{
	  p = Read_32 (iwad);
	  fclose (iwad);
	  if (p == IWAD_MAGIC)  return (0);
	}
      }
      filename [st] = oc;  /* Repair the damage */
    }
  }
  return (rc);
}

//-----------------------------------------------------------------------------

static int search_for_iwad (char * wad_filename, const char *doomwaddir, const char * iwadname, unsigned int reqd_pwad)
{
  unsigned int verbose;
  unsigned int this_iwad;
  unsigned int best_iwad;
  DIR * dirp;
  struct dirent *dp;
  struct stat file_stat;
  char buffer [400];

  dirp = opendir (doomwaddir);
  if (dirp == 0)
  {
    // fprintf (stderr,"Couldn't open directory %s for scanning\n", dirname);
  }
  else
  {
    verbose = M_CheckParm ("-showdirsearch");
    if (verbose)
    {
      fprintf (stderr, "Searching directory %s", doomwaddir);
      if (iwadname)
	fprintf (stderr, " for %s", iwadname);
      if (reqd_pwad)
	fprintf (stderr, " for wad type %d", reqd_pwad);
      fprintf (stderr, "\n");
    }
    best_iwad = 0;
    while ((dp = readdir (dirp)) != NULL)
    {
      /* Miss out . & .. */
      if ((strcmp (dp -> d_name, ".") != 0)
       &&  (strcmp (dp -> d_name, "..") != 0))
      {
	if ((iwadname == NULL)
	 || (strcasecmp (iwadname, dp -> d_name) == 0))
	{
	  sprintf (buffer, "%s"DIRSEP"%s", doomwaddir, dp -> d_name);
	  this_iwad = IdentifyWad (buffer, IWAD_MAGIC);
	  if (this_iwad)
	  {
	    if (verbose)
	      printf ("%s is an iwad of type %d\n", dp -> d_name, this_iwad);
	    if (reqd_pwad == 0)			// Just looking
	    {
	      if (this_iwad > best_iwad)
	      {
		best_iwad = this_iwad;
		strcpy (wad_filename, buffer);
	      }
	    }
	    else if (reqd_pwad <= 3)		// Want doom1
	    {
	      if ((this_iwad < 4)
	       && (this_iwad > best_iwad))
	      {
		best_iwad = this_iwad;
		strcpy (wad_filename, buffer);
	      }
	    }
	    else				// Want doom2
	    {
	      if ((this_iwad == 4)
	       && ((this_iwad > best_iwad)
	       || (strncasecmp (dp -> d_name, "doom2", 5) == 0)))
	      {
		best_iwad = this_iwad;
		strcpy (wad_filename, buffer);
	      }
	    }
	  }
	}
      }
    }

    if (best_iwad)
    {
      closedir (dirp);
      return (best_iwad);
    }

    /* Having scanned the directory now try subdirectories */

    rewinddir (dirp);

    while ((dp = readdir (dirp)) != NULL)
    {
      /* Miss out . & .. */
      if ((strcmp (dp -> d_name, ".") != 0)
       &&  (strcmp (dp -> d_name, "..") != 0))
      {
	sprintf (buffer, "%s"DIRSEP"%s", doomwaddir, dp -> d_name);
	if ((stat (buffer, &file_stat) == 0)
	 && (file_stat.st_mode & S_IFDIR))
	{
	  this_iwad = search_for_iwad (wad_filename, buffer, iwadname, reqd_pwad);
	  if (this_iwad)
	  {
	    closedir (dirp);
	    return (this_iwad);
	  }
	}
      }
    }

    closedir (dirp);
  }
  return (0);
}

//-----------------------------------------------------------------------------
/*
  Extract the next element from the path variable.
  Remove any trailing '/' (or . for riscos)
*/

static const char * next_path_element (char * dest, const char * path)
{
  char cc;
  char *ptr;

  ptr = dest;
  do
  {
    cc = *path;
    if ((cc == 0) || (cc == PATHSEPC))
    {
      *ptr = 0;
      while (ptr > (dest+1))
      {
	ptr--;
	if (*ptr != DIRSEPC)
	  break;
	*ptr = 0;
      }
      break;
    }
    else
    {
      *ptr++ = cc;
      path++;
    }
  } while (1);

  return (path);
}

//-----------------------------------------------------------------------------

static int search_for_iwad_in_path (char * wad_filename, const char *doomwadpath, const char * iwadname, unsigned int reqd_pwad)
{
  int iwad;
  char doomwaddir [200];

  if (doomwadpath)
  {
    while (*doomwadpath)
    {
      doomwadpath = next_path_element (doomwaddir, doomwadpath);
      if (M_CheckParm ("-showdirsearch"))
	printf ("Searching path element %s\n", doomwaddir);
      iwad = search_for_iwad (wad_filename, doomwaddir, iwadname, reqd_pwad);
      if (iwad)
	return (iwad);
    }
  }

  return (0);
}

//-----------------------------------------------------------------------------

#ifdef NORMALUNIX
static int is_a_writable_directory (const char * name)
{
  int rc;
  struct stat buf;

  rc = access (name, R_OK | W_OK | X_OK);
  if (rc != 0) return (rc);

  rc = stat (name, &buf);
  if (rc != 0)  return (rc);

  if (S_ISDIR(buf.st_mode)) return (0);
  return (-1);
}

static int is_a_directory (const char * name)
{
  int rc;
  struct stat buf;

  rc = access (name, R_OK | X_OK);
  if (rc != 0) return (rc);

  rc = stat (name, &buf);
  if (rc != 0)  return (rc);

  if (S_ISDIR(buf.st_mode)) return (0);
  return (-1);
}
#endif

//-----------------------------------------------------------------------------

static int already_in_wadlist (const char * filename, char * suffix)
{
  unsigned int p,l;

  l = strlen (filename);
  if ((l > 3)
    && (filename [l - 4] == EXTSEPC)
    && (strcasecmp (filename + l - 3, suffix) == 0))
  {
    l -= 3;
    p = 0;
    while (wadfiles[p])
    {
      if (strncasecmp (wadfiles[p], filename, l) == 0)
      {
	// fprintf (stderr, "Found file %s already in the wad list (%s)(%u)\n", filename, wadfiles[p], l);
	return (1);
      }
      p++;
    }
  }

  return (0);
}

//-----------------------------------------------------------------------------

#define get_wad_filename(d,r,f)  sprintf(d,"%s"DIRSEP"%s"EXTSEP"wad",r,f);

//-----------------------------------------------------------------------------

static int search_for_specific_wad_in_path (const char * path, const char * name)
{
  char doomwaddir [200];
  char wad_filename [200];

  if (path)
  {
    while (*path)
    {
      path = next_path_element (doomwaddir, path);
      get_wad_filename (wad_filename, doomwaddir, name);
      if (M_CheckParm ("-showdirsearch"))
	printf ("Checking for %s\n", wad_filename);
      if (!wad_present (wad_filename))
      {
	IdentifyIwad (wad_filename);
	return (1);
      }
    }
  }

  return (0);
}

//-----------------------------------------------------------------------------

#if 0
#define search_for_specific_wad(p1,p2,n)		\
	   ((search_for_specific_wad_in_path (p1,n))	\
	|| (search_for_specific_wad_in_path (p2,n)))

#endif

#ifndef search_for_specific_wad
static int search_for_specific_wad (const char * path1, const char * path2, const char * name)
{
  int rc;

  rc = search_for_specific_wad_in_path (path1, name);

  if (rc == 0)
    rc = search_for_specific_wad_in_path (path2, name);

  return (rc);
}
#endif
//-----------------------------------------------------------------------------
//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
static void IdentifyVersion (void)
{
  unsigned int p;
  unsigned int q;
  unsigned int reqd_pwad;
  char * f;
  const char *doomwaddir;
  const char *doomwadpath;
  char wad_filename [200];
  char wad_dirname [200];

#ifdef NORMALUNIX

  char cc;
  char *home;
  char *choicespath;

  p = M_CheckParm ("-iwad_dir");	// Have we been given a directory on the command line?
  if (p)
  {
    doomwaddir = myargv[p+1];
  }
  else
  {
    doomwaddir = getenv("DOOMWADDIR");
    if (!doomwaddir)
    {
      dirname (wad_dirname, myargv [0]);
      doomwaddir = wad_dirname;
      // printf ("doomwaddir = %s\n", doomwaddir);
    }
  }

  p = M_CheckParm ("-iwad_path");	// Have we been given a directory on the command line?
  if (p)
  {
    doomwadpath = myargv[p+1];
  }
  else
  {
    doomwadpath = getenv("DOOMWADPATH");
  }

  basedefaultdir [0] = 0;

  choicespath = getenv ("CHOICESPATH");			// Are we running under ROX?
  if (choicespath)
  {							// Yes. Search for the first
    p = 0;						// writable directory.

    do
    {
      cc = *choicespath++;
      basedefaultdir [p++] = cc;

      if ((cc == 0) || (cc == ':'))
      {
	basedefaultdir [p-1] = 0;
	if (is_a_writable_directory (basedefaultdir) == 0)
	{
	  strcat (basedefaultdir, "/dsg");
	  break;
	}
	basedefaultdir [0] = 0;
	p = 0;
      }
    } while (cc);
  }

  if (basedefaultdir [0] == 0)				// Did we find a writable directory?
  {
    home = getenv("HOME");
    if (!home)
      I_Error ("Please set $HOME to your home directory");

    sprintf (basedefaultdir, "%s/.Choices", home);	// Let's see if there's a Choices dir.
    if (is_a_writable_directory (basedefaultdir) != 0)
    {
      sprintf (basedefaultdir, "%s/Choices", home);
      if (is_a_writable_directory (basedefaultdir) != 0)
      {
	strcpy (basedefaultdir, home);
      }
    }
    if (strcmp (basedefaultdir, home) == 0)
      strcat (basedefaultdir, "/.dsg");
    else
      strcat (basedefaultdir, "/dsg");
  }

#endif

#ifdef __riscos

  f = getenv ("DoomSavDir");
  if (f)
    strcpy (basedefaultdir, "<DoomSavDir>");
  else
    strcpy (basedefaultdir, "<Choices$Dir>.dsg");

  /* Have to be careful with the getenv call here as we cannot call it */
  /* twice without taking a copy, as it corrupts the workspace next time. */

  p = M_CheckParm ("-iwad_dir");	// Have we been given a directory on the command line?
  if (p)
  {
    doomwaddir = myargv[p+1];
  }
  else
  {
    doomwaddir = getenv ("DOOMWADDIR");
    if (doomwaddir)
    {
      strcpy (wad_dirname, doomwaddir);
    }
    else
    {
      dirname (wad_dirname, myargv [0]);
      strcat (wad_dirname, DIRSEP"Iwads");
      // printf ("doomwaddir = %s\n", doomwaddir);
    }
    doomwaddir = wad_dirname;
  }

  p = M_CheckParm ("-iwad_path");	// Have we been given a directory on the command line?
  if (p)
  {
    doomwadpath = myargv[p+1];
  }
  else
  {
    doomwadpath = getenv ("DOOMWADPATH");
  }

#endif

  mkdir (basedefaultdir, 0755);


#if 0
  if (M_CheckParm ("-shdev"))
  {
      gamemode = shareware;
      devparm = true;
      D_AddFile (DEVDATA"doom1.wad");
      D_AddFile (DEVMAPS"data_se/texture1.lmp");
      D_AddFile (DEVMAPS"data_se/pnames.lmp");
      strcpy (basedefaultdir,DEVDATA"default.cfg");
      return;
  }

  if (M_CheckParm ("-regdev"))
  {
      gamemode = registered;
      devparm = true;
      D_AddFile (DEVDATA"doom.wad");
      D_AddFile (DEVMAPS"data_se/texture1.lmp");
      D_AddFile (DEVMAPS"data_se/texture2.lmp");
      D_AddFile (DEVMAPS"data_se/pnames.lmp");
      strcpy (basedefaultdir,DEVDATA"default.cfg");
      return;
  }

  if (M_CheckParm ("-comdev"))
  {
      gamemode = commercial;
      devparm = true;
      /* I don't bother
      if(plutonia)
	  D_AddFile (DEVDATA"plutonia.wad");
      else if(tnt)
	  D_AddFile (DEVDATA"tnt.wad");
      else*/
	  D_AddFile (DEVDATA"doom2.wad");

      D_AddFile (DEVMAPS"cdata/texture1.lmp");
      D_AddFile (DEVMAPS"cdata/pnames.lmp");
      strcpy (basedefaultdir,DEVDATA"default.cfg");
      return;
  }
#endif

  p = M_CheckParm ("-iwad");
  if (p)
  {
    strcpy (wad_filename, myargv[p+1]);
    if (!wad_present (wad_filename))
    {
      IdentifyIwad (wad_filename);
      return;
    }

    // Allow the user to specify -iwad doom.wad without the full path

    if (search_for_iwad (wad_filename, doomwaddir, myargv[p+1], 0))
    {
      IdentifyIwad (wad_filename);
      return;
    }

    if (search_for_iwad_in_path (wad_filename, doomwadpath, myargv[p+1], 0))
    {
      IdentifyIwad (wad_filename);
      return;
    }

    /* Try the same directory as the executable */
    dirname (wad_dirname, myargv [0]);
    if ((strcmp (wad_dirname, doomwaddir))
     && (search_for_iwad (wad_filename, wad_dirname, myargv[p+1], 0)))
    {
      IdentifyIwad (wad_filename);
      return;
    }

#ifdef NORMALUNIX
    /* Try .. */
    dirname (wad_dirname, wad_dirname);
    if ((search_for_iwad (wad_filename, wad_dirname, myargv[p+1], 0))

    /* Try a couple of obvious places in unix */
     || (search_for_iwad (wad_filename, "/usr/local", myargv[p+1], 0))
     || (search_for_iwad (wad_filename, "/usr/share", myargv[p+1], 0)))
    {
      IdentifyIwad (wad_filename);
      return;
    }
#endif

    fprintf (stderr, "Failed to find Iwad file %s\n", myargv[p+1]);
    exit (0);
  }

  /* Haven't been given a specific IWAD, but have we got a PWAD with a map? */
  reqd_pwad = 0;

  p = 1;
  while ((p = M_CheckParm_N ("-file", p)) != 0)
  {
    q = M_CheckParm ("-file_dir");
    while (++p < myargc && myargv[p][0] != '-')
    {
      f = myargv[p];
      if (q)
      {
	sprintf (wad_filename, "%s"DIRSEP"%s", myargv[q+1],f);
	f = wad_filename;
      }
      reqd_pwad = IdentifyWad (f, PWAD_MAGIC);
      if (reqd_pwad)
      {
	// printf ("Required WAD for %s is type %u\n", f, reqd_pwad);
	break;
      }
      if (IdentifyWad (f, IWAD_MAGIC))
      {					// Ooops, the user has specified an IWAD in the -file
	IdentifyIwad (f);
	return;
      }
    }
  }

  if ((reqd_pwad == 0) || (reqd_pwad >= 4))
  {
    if (search_for_specific_wad (doomwaddir, doomwadpath, "doom2f"))
    {
      // C'est ridicule!
      // Let's handle languages in config files, okay?
      language = french;
      printf("French version\n");
      return;
    }

    if (search_for_specific_wad (doomwaddir, doomwadpath, "doom2"))
    {
      return;
    }
  }

  if (reqd_pwad == 0)
  {
    if (search_for_specific_wad (doomwaddir, doomwadpath, "plutonia"))
    {
      return;
    }

    if (search_for_specific_wad (doomwaddir, doomwadpath, "tnt"))
    {
      return;
    }
  }

  if (reqd_pwad < 4)
  {
    if (search_for_specific_wad (doomwaddir, doomwadpath, "doomu"))
    {
      return;
    }

    if (search_for_specific_wad (doomwaddir, doomwadpath, "doom"))
    {
      return;
    }
  }

  if ((reqd_pwad == 0) || (reqd_pwad >= 4))
  {
    if (search_for_specific_wad (doomwaddir, doomwadpath, "freedoom2"))
    {
      return;
    }
  }

  if (search_for_specific_wad (doomwaddir, doomwadpath, "freedoom1"))
  {
    return;
  }

  if (search_for_specific_wad (doomwaddir, doomwadpath, "doom1"))
  {
    return;
  }

  if (search_for_iwad (wad_filename, doomwaddir, NULL, reqd_pwad))
  {
    IdentifyIwad (wad_filename);
    return;
  }

  /* Try the same directory as the executable */
  dirname (wad_dirname, myargv [0]);
  if ((strcmp (wad_dirname, doomwaddir))
   && (search_for_iwad (wad_filename, wad_dirname, NULL, reqd_pwad)))
  {
    IdentifyIwad (wad_filename);
    return;
  }

#ifdef NORMALUNIX
  /* Try .. */
  dirname (wad_dirname, wad_dirname);
  if ((search_for_iwad (wad_filename, wad_dirname, NULL, reqd_pwad))

  /* Try a couple of obvious places in unix */
   || (search_for_iwad (wad_filename, "/usr/local", NULL, reqd_pwad))
   || (search_for_iwad (wad_filename, "/usr/share", NULL, reqd_pwad)))
  {
    IdentifyIwad (wad_filename);
    return;
  }
#endif

  printf("Game mode indeterminate.\n");
  gamemode = indetermined;

  // We don't abort. Let's see what the PWAD contains.
  //exit(1);
  //I_Error ("Game mode indeterminate\n");
}

//-----------------------------------------------------------------------------

static void find_language_in_wad (const char * wadname)
{
  unsigned int cat_pos;
  unsigned int cat_size;
  unsigned int file_pos;
  unsigned int file_size;
  unsigned int p;
  FILE * fin;
  char catname [9];

  fin = fopen (wadname, "rb");
  if (fin)
  {
    p = Read_32 (fin);
    if ((p == IWAD_MAGIC)
     || (p == PWAD_MAGIC))
    {
      cat_size = Read_32 (fin);
      cat_pos  = Read_32 (fin);
      fseek (fin, (long int) cat_pos, SEEK_SET);
      catname [8] = 0;
      do
      {
	file_pos  = Read_32 (fin);
	file_size = Read_32 (fin);
	p = 0;
	do
	{
	  catname [p] = fgetc (fin);
	} while (++p < 8);

	if ((strcasecmp(catname, "LANGUAGE") == 0)
	 || (strcasecmp(catname, "GAMEINFO") == 0))
	{
	  fseek (fin, (long int) file_pos, SEEK_SET);
	  DH_parse_language_file_f (wadname, fin, file_pos+file_size);
	  break;
	}
      } while (--cat_size);
    }
    fclose (fin);
  }
}

//-----------------------------------------------------------------------------

static void find_dehacked_in_wad (const char * wadname)
{
  unsigned int cat_pos;
  unsigned int cat_size;
  unsigned int file_pos;
  unsigned int file_size;
  unsigned int p;
  FILE * fin;
  char catname [9];

  fin = fopen (wadname, "rb");
  if (fin)
  {
    p = Read_32 (fin);
    if ((p == IWAD_MAGIC)
     || (p == PWAD_MAGIC))
    {
      cat_size = Read_32 (fin);
      cat_pos  = Read_32 (fin);
      fseek (fin, (long int) cat_pos, SEEK_SET);
      catname [8] = 0;
      do
      {
	file_pos  = Read_32 (fin);
	file_size = Read_32 (fin);
	p = 0;
	do
	{
	  catname [p] = fgetc (fin);
	} while (++p < 8);

	if (strcasecmp(catname, "DEHACKED") == 0)
	{
	  fseek (fin, (long int) file_pos, SEEK_SET);
	  DH_parse_hacker_file_f (wadname, fin, file_pos+file_size);
	  break;
	}
      } while (--cat_size);
    }
    fclose (fin);
  }
}

//-----------------------------------------------------------------------------

unsigned int D_Startup_msg_number (void)
{
  unsigned int p;

  switch (gamemode)
  {
    case retail:
      p = D_ULTIMATE;
      break;

    case shareware:
      p = D_SHAREWARE;
      break;

    case registered:
      p = D_REGISTERED;
      break;

    case commercial:
      switch (gamemission)
      {
	case pack_plut:
	  p = D_PLUTONIA;
	  break;

	case pack_tnt:
	  p = D_TNT;
	  break;

	default:
	  p = D_HELL_ON_EARTH;
      }
      break;


    default:
      p = D_PUBLIC;
      break;
  }

  return (p);
}

//-----------------------------------------------------------------------------

void D_LoadCheats (void)
{
  nomonsters1 = nomonsters2 = (boolean) M_CheckParm ("-nomonsters");
  Monsters_Infight2 = (boolean) M_CheckParm ("-monstersinfight");
  Give_Max_Damage = (boolean) M_CheckParm ("-maxdamage");
  nobrainspitcheat = (boolean) M_CheckParm ("-nobrainspit");
}

/* We clear the cheats when running a demo.... */

void D_ClearCheats (void)
{
  nomonsters1 = nomonsters2 = false;
  Monsters_Infight2 = false;
  Give_Max_Damage = false;
  nobrainspitcheat = false;
}

//-----------------------------------------------------------------------------

static void D_SetDebugOut (void)
{
  unsigned int	p;
  char *	f;
  char		file[256];

  p = M_CheckParm ("-debugfile");
  if (p)
  {
    p++;
    if ((p < myargc) && (myargv[p][0] != '-'))
    {
      f = myargv[p];
    }
    else
    {
      f = file;
      sprintf (file,"debug%i"EXTSEP"txt",consoleplayer);
    }
    printf ("debug output to: %s\n",f);
    debugfile = fopen (f,"w");
  }
}

//-----------------------------------------------------------------------------

static void D_LoadWads (void)
{
  unsigned int	p,q;
  char *	f;
  char		file[256];

  // add any files specified on the command line with -file wadfile
  // to the wad list
  //
  // convenience hack to allow -wart e m to add a wad file
  // prepend a tilde to the filename so wadfile will be reloadable
  p = M_CheckParm ("-wart");
  if (p)
  {
    // myargv[p][4] = 'p';     // big hack, change to -warp  DONE LATER (JAD)

    // Map name handling.
    switch (gamemode )
    {
      case shareware:
      case retail:
      case registered:
	sprintf (file,"~"DEVMAPS"E%cM%c.wad",
		 myargv[p+1][0], myargv[p+2][0]);
	break;

      case commercial:
      default:
	p = atoi (myargv[p+1]);
	if (p<10)
	  sprintf (file,"~"DEVMAPS"cdata/map0%i.wad", p);
	else
	  sprintf (file,"~"DEVMAPS"cdata/map%i.wad", p);
	break;
    }
    D_AddFile (file);
  }

  p = 1;
  while ((p = M_CheckParm_N ("-file", p)) != 0)
  {
    // the parms after p are wadfile/lump names,
    // until end of parms or another - preceded parm
    modifiedgame = true;	// homebrew levels
    q = M_CheckParm ("-file_dir");

    while (++p < myargc && myargv[p][0] != '-')
    {
      f = myargv[p];
      if (q)
      {
	sprintf (file, "%s"DIRSEP"%s", myargv[q+1],f);
	f = file;
      }
#ifdef __riscos
      if (RiscOs_expand_path (f) == 0)
#endif
      D_AddFile (f);
    }
  }
}

//-----------------------------------------------------------------------------

static void D_ShowStartupMessage (void)
{
  unsigned int	p,q;
  unsigned int	len;
  char *	p1;
  char *	p2;
  char *	linebuf;

  q = 0;
  p = 0;
  do
  {
    if (startup_messages [p] && startup_messages[p][0])
    {
      len = strlen (startup_messages [p]);
      linebuf = malloc (len+10);
      if (linebuf == NULL)
	break;
      p1 = linebuf;
      sprintf (p1, startup_messages[p], GAME_ENGINE_VERSION/100,GAME_ENGINE_VERSION%100);
      do
      {
	p2 = strchr (p1, '\n');
	if (p2) *p2 = 0;
	if (*p1 != ' ')
	{
	  len = strlen (p1);
	  if (len < 80)
	  {
	    len = (80 - len) / 2;
	    while (len)
	    {
	      putchar (' ');
	      len--;
	    }
	  }
	}
	printf ("%s\n", p1);
	if (p2 == NULL)
	  break;
	putchar ('\n');		// Write the LF that we clobbered.
	p1 = p2 + 1;
      } while (*p1);
      free (linebuf);
      q++;
    }
  } while (++p < ARRAY_SIZE(startup_messages));

  if (q == 0)
  {
    p = D_Startup_msg_number ();
    printf (dmain_messages[p], GAME_ENGINE_VERSION/100,GAME_ENGINE_VERSION%100);
  }

  printf ("\n\nScreen resolution is %d x %d\n", SCREENWIDTH, SCREENHEIGHT);
}

//-----------------------------------------------------------------------------

static void D_LoadGameFile (void)
{
  unsigned int	p;
  char		file[256];

  p = M_CheckParm ("-loadgame");
  if (p && p < myargc-1)
  {
    if ((M_CheckParm("-cdrom") == 0)
     && (access (myargv[p+1], R_OK) == 0))
    {
      strcpy (file, myargv[p+1]);
    }
    else
    {
      G_GetSaveGameName (file, myargv[p+1][0]);
    }
    G_LoadGame (file);
  }
}

//-----------------------------------------------------------------------------
//
// D_DoomMain
//
void D_DoomMain (void)
{
  unsigned int	p;
  map_starts_t *  map_info_p;

  D_SetDebugOut ();
  init_text_messages ();
  I_SetScreenSize ();
  IdentifyVersion ();

  setbuf (stdout, NULL);
  modifiedgame = false;
  D_LoadCheats ();

  respawnparm = (boolean) M_CheckParm ("-respawn");
  fastparm = (boolean) M_CheckParm ("-fast");
  devparm = (boolean) M_CheckParm ("-devparm");
  if (M_CheckParm ("-altdeath"))
    deathmatch = (boolean) 2;
  else if (M_CheckParm ("-deathmatch"))
    deathmatch = (boolean) 1;

  D_LoadWads ();

  dh_changing_pwad = false;

  p = M_CheckParm ("-nowadlang");
  if (p == 0)
  {
    while (wadfiles[p])
    {
      find_language_in_wad (wadfiles[p]);
      dh_changing_pwad = true;			// Subsequent files are Pwads...
      p++;
    }
  }

  p = M_CheckParm ("-nowaddeh");
  if (p == 0)
  {
    dh_changing_pwad = false;
    while (wadfiles[p])
    {
      find_dehacked_in_wad (wadfiles[p]);
      dh_changing_pwad = true;			// Subsequent files are Pwads...
      p++;
    }
  }

  dh_changing_pwad = true;

  p = M_CheckParm ("-noautodeh");
  if (p == 0)
  {
    while (wadfiles[p])
    {
      DH_parse_hacker_wad_file (wadfiles[p], true);
      p++;
    }
  }

  p = M_CheckParm ("-dehack");
  if (p == 0)
    p = M_CheckParm ("-deh");		/* Allow alternative */
  if (p)
  {
      // the parms after p are dehack script names,
      // until end of parms or another - preceded parm
      while (++p < myargc && myargv[p][0] != '-')
      {
	if (already_in_wadlist (myargv[p], "deh") == 0)
	  DH_parse_hacker_file (myargv[p]);
      }
  }

  printf ("\n\n");
  D_ShowStartupMessage ();

  if (devparm)
      printf(dmain_messages[D_D_DEVSTR]);

  if (M_CheckParm("-cdrom"))
  {
#ifdef __riscos
      printf(dmain_messages[D_D_CDROM_RO]);
      mkdir ("<Choices$Dir>.dsg",0755);
      strcpy (basedefaultdir,"<Choices$Dir>.dsg");
#else
      printf(dmain_messages[D_D_CDROM]);
      mkdir ("c:\\doomdata",0755);
      strcpy (basedefaultdir,"c:/doomdata");
#endif
  }

  // turbo option
  p = M_CheckParm ("-turbo");
  if (p)
  {
      int     scale = 200;
      extern int forwardmove[2];
      extern int sidemove[2];

      if (p<myargc-1)
	  scale = atoi (myargv[p+1]);
      if (scale < 10)
	  scale = 10;
      if (scale > 400)
	  scale = 400;
      printf ("turbo scale: %i%%\n",scale);
      forwardmove[0] = forwardmove[0]*scale/100;
      forwardmove[1] = forwardmove[1]*scale/100;
      sidemove[0] = sidemove[0]*scale/100;
      sidemove[1] = sidemove[1]*scale/100;
  }


  // convenience hack to allow -wart e m to add a wad file
  p = M_CheckParm ("-wart");
  if (p)
  {
      myargv[p][4] = 'p';     // big hack, change to -warp

      // Map name handling.
      switch (gamemode )
      {
	case shareware:
	case retail:
	case registered:
	  printf("Warping to Episode %s, Map %s.\n",
		 myargv[p+1],myargv[p+2]);
	  break;

      }
  }


#if 0
  p = M_CheckParm ("-playdemo");

  if (!p)
      p = M_CheckParm ("-timedemo");

  if (p && p < myargc-1)
  {
#ifdef __riscos
    D_AddFile (myargv[p+1]);
    printf("Playing demo %s.\n",myargv[p+1]);
#else
    sprintf (file,"%s.lmp", myargv[p+1]);
    D_AddFile (file);
    printf("Playing demo %s.lmp.\n",myargv[p+1]);
#endif
  }
#endif

  // get skill / episode / map from parms
  startskill = sk_medium;
  startepisode = 1;
  startmap = 0;
  autostart = false;

  map_info_p = G_Access_MapStartTab (1);
  startmap     = map_info_p -> start_map;
  if (gamemode != commercial)
    startepisode = map_info_p -> start_episode;

  p = M_CheckParm ("-skill");
  if (p && p < myargc-1)
  {
      startskill = (skill_t) (myargv[p+1][0]-'1');
      autostart = true;
  }

  p = M_CheckParm ("-episode");
  if (p && p < myargc-1)
  {
      startepisode = myargv[p+1][0]-'0';
      startmap = 1;
      autostart = true;
  }

  p = M_CheckParm ("-timer");
  if (p && p < myargc-1 && deathmatch)
  {
      int     time;
      time = atoi(myargv[p+1]);
      printf("Levels will end after %d minute",time);
      if (time>1)
	  printf("s");
      printf(".\n");
  }

  p = M_CheckParm ("-avg");
  if (p && p < myargc-1 && deathmatch)
      printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");

  p = M_CheckParm ("-warp");
  if (p && p < myargc-1)
  {
      if (gamemode == commercial)
	  startmap = atoi (myargv[p+1]);
      else
      {
	  startepisode = myargv[p+1][0]-'0';
	  startmap = myargv[p+2][0]-'0';
      }
      autostart = true;
  }

  // init subsystems
  // printf ("V_Init: allocate screens.\n");
  V_Init ();

  // printf ("M_LoadDefaults: Load system defaults.\n");
  M_LoadDefaults ();		// load before initing other systems

  // printf ("Z_Init: Init zone memory allocation daemon. \n");
  Z_Init ();

  //printf ("W_Init: Init WADfiles.\n");
  printf ("Using Wadfiles:\n");
  W_InitMultipleFiles (wadfiles);


  /* This simply tells the user that the files were/will be actioned */
  p = 0;
  while (wadfiles[p])
  {
    HU_parse_map_name_file (wadfiles[p], false);
    if (M_CheckParm ("-noautodeh") == 0)
      DH_parse_hacker_wad_file (wadfiles[p], false);
    G_parse_map_seq_wad_file (wadfiles[p], false);
    p++;
  }


  // Check for -file in shareware
  if (modifiedgame)
  {
      // These are the lumps that will be checked in IWAD,
      // if any one is not present, execution will be aborted.
      char name[23][9]=
      {
	  "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
	  "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
	  "dphoof","bfgga0","heada1","cybra1","spida1d1"
      };
      int i;

      if ( gamemode == shareware)
	  I_Error("\nYou cannot -file with the shareware "
		  "version. Register!");

      // Check for fake IWAD with right name,
      // but w/o all the lumps of the registered version.
      if (gamemode == registered)
	  for (i = 0;i < 23; i++)
	      if (W_CheckNumForName (name[i])<0)
		  I_Error("\nThis is not the registered version.");

      // Iff additonal PWAD files are used, print modified banner
      printf (dmain_messages [D_MODIFIED]);
      // getchar (); Removed by JAD 'cos it's a bloody nuisence!
  }


  // Check and print which version is executed.
  switch ( gamemode )
  {
    case shareware:
    case indetermined:
      printf (dmain_messages [D_SHAREWARE_2]);
      break;
    case registered:
    case retail:
    case commercial:
//    if (W_CheckNumForName ("FREEDOOM") == -1)		// Not needed anymore as freedoom
//							// now sets this up in DEHACKED
	printf (dmain_messages [D_COMMERCIAL]);
      break;

    default:
      // Ouch.
      break;
  }

  // printf ("R_Init: Init DOOM refresh daemon - ");
  R_Init ();

  // printf ("\nP_Init: Init Playloop state.\n");
  P_Init ();

  // printf ("I_Init: Setting up machine state.\n");
  I_Init ();

  // printf ("D_CheckNetGame: Checking network game status.\n");
  D_CheckNetGame ();

  W_RemoveDuplicates ();
  W_QuicksortLumps (0, numlumps-1);

  /* We do these down here so that they can override a MAPINFO segment */
  p = 0;
  while (wadfiles[p])
  {
    HU_parse_map_name_file (wadfiles[p], true);
    G_parse_map_seq_wad_file (wadfiles[p], true);
    p++;
  }
  p = M_CheckParm ("-mapseq");
  if (p)
  {
      // the parms after p are map sequence file names,
      // until end of parms or another - preceded parm
      while (++p < myargc && myargv[p][0] != '-')
      {
	if (already_in_wadlist (myargv[p], "msq") == 0)
	  G_ReadMapSeq (myargv[p]);
      }
  }

  DH_remove_duplicate_mapinfos ();

  // printf ("M_Init: Init miscellaneous info.\n");
  M_Init ();					// Must be after the MAPINFO readers...
  // printf ("S_Init: Setting up sound.\n");
  S_Init (snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );
  // printf ("HU_Init: Setting up heads up display.\n");
  HU_Init ();
  // printf ("ST_Init: Init status bar.\n");
  ST_Init ();
  V_LoadFonts ();
  I_InitGraphics ();


  // check for a driver that wants intermission stats
  p = M_CheckParm ("-statcopy");
  if (p && p<myargc-1)
  {
      // for statistics driver
      extern  void*	statcopy;

      statcopy = (void*)((uintptr_t)atoi(myargv[p+1]));
#ifdef NORMALUNIX
      printf ("External statistics registered.\n");
#endif
  }

  // start the apropriate game based on parms
  p = M_CheckParm ("-record");
  if (p && p < myargc-1)
  {
      G_RecordDemo (myargv[p+1]);
      autostart = true;
  }

  p = M_CheckParm ("-playdemo");
  if (p && p < myargc-1)
  {
#ifdef NORMALUNIX
      printf ("Playing demo %s.\n",myargv[p+1]);
#endif
      singledemo = true;		// quit after one demo
      G_DeferedPlayDemo (myargv[p+1]);
      D_DoomLoop ();			// never returns
      return;
  }

  p = M_CheckParm ("-timedemo");
  if (p && p < myargc-1)
  {
#ifdef NORMALUNIX
      printf ("Playing demo %s.\n",myargv[p+1]);
#endif
      G_TimeDemo (myargv[p+1]);
      D_DoomLoop ();			// never returns
      return;
  }

  D_LoadGameFile ();

  if ( gameaction != ga_loadgame )
  {
      if (autostart || netgame)
	  G_InitNew (startskill, startepisode, startmap);
      else
	  D_StartTitle ();		// start up intro loop
  }

  D_DoomLoop ();  // never returns
}
