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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: m_menu.c,v 1.7 1997/02/03 22:45:10 b1 Exp $";
#endif

#include "includes.h"

extern boolean	st_firsttime;
/* ----------------------------------------------------------------------- */

char * menu_messages_orig [] =
{
  LOADNET,
  QLOADNET,
  QSAVESPOT,
  SAVEDEAD,
  QSPROMPT,
  QLPROMPT,
  NEWGAME,
  NIGHTMARE,
  SWSTRING,
  MSGOFF,
  MSGON,
  NETEND,
  ENDGAME,
  DOSY,
  DETAILHI,
  DETAILLO,
  GAMMALVL0,
  GAMMALVL1,
  GAMMALVL2,
  GAMMALVL3,
  GAMMALVL4,
  EMPTYSTRING,
  NULL
};

char * menu_messages [ARRAY_SIZE(menu_messages_orig)];


typedef enum
{
  MM_LOADNET,
  MM_QLOADNET,
  MM_QSAVESPOT,
  MM_SAVEDEAD,
  MM_QSPROMPT,
  MM_QLPROMPT,
  MM_NEWGAME,
  MM_NIGHTMARE,
  MM_SWSTRING,
  MM_MSGOFF,
  MM_MSGON,
  MM_NETEND,
  MM_ENDGAME,
  MM_DOSY,
  MM_DETAILHI,
  MM_DETAILLO,
  MM_GAMMALVL0,
  MM_GAMMALVL1,
  MM_GAMMALVL2,
  MM_GAMMALVL3,
  MM_GAMMALVL4,
  MM_EMPTYSTRING
} menu_texts_t;

/* ----------------------------------------------------------------------- */

char * menu_lump_names_orig [] =
{
  "",
  "M_DOOM",
  "M_NEWG",
  "M_SKILL",
  "M_EPISOD",
  "M_SKULL1",
  "M_SKULL2",
  "M_NGAME",
  "M_OPTION",
  "M_LOADG",
  "M_SAVEG",
  "M_LGTTL",
  "M_SGTTL",
  "M_RDTHIS",
  "M_QUITG",
  "M_EPI0",
  "M_EPI1",
  "M_EPI2",
  "M_EPI3",
  "M_EPI4",
  "M_EPI5",
  "M_EPI6",
  "M_EPI7",
  "M_EPI8",
  "M_EPI9",
  "M_JKILL",
  "M_ROUGH",
  "M_HURT",
  "M_ULTRA",
  "M_NMARE",
  "M_ENDGAM",
  "M_MESSG",
  "M_DETAIL",
  "M_SCRNSZ",
  "M_MSENS",
  "M_SVOL",
  "M_SFXVOL",
  "M_MUSVOL",
  "M_LSLEFT",
  "M_LSCNTR",
  "M_LSRGHT",
  "M_GDHIGH",
  "M_GDLOW",
  "M_MSGOFF",
  "M_MSGON",
  "M_OPTTTL",
  "M_THERML",
  "M_THERMM",
  "M_THERMR",
  "M_THERMO",
  "M_CELL1",
  "M_CELL2",
  "STDISK",
  "STCDROM",
  NULL
};


char * menu_lump_names [ARRAY_SIZE(menu_lump_names_orig)];


typedef enum
{
  ML_NULL,
  ML_DOOM,
  ML_NEWG,
  ML_SKILL,
  ML_EPISOD,
  ML_SKULL1,
  ML_SKULL2,
  ML_NGAME,
  ML_OPTION,
  ML_LOADG,
  ML_SAVEG,
  ML_LGTTL,
  ML_SGTTL,
  ML_RDTHIS,
  ML_QUITG,
  ML_EPI0,
  ML_EPI1,
  ML_EPI2,
  ML_EPI3,
  ML_EPI4,
  ML_EPI5,
  ML_EPI6,
  ML_EPI7,
  ML_EPI8,
  ML_EPI9,
  ML_JKILL,
  ML_ROUGH,
  ML_HURT,
  ML_ULTRA,
  ML_NMARE,
  ML_ENDGAM,
  ML_MESSG,
  ML_DETAIL,
  ML_SCRNSZ,
  ML_MSENS,
  ML_SVOL,
  ML_SFXVOL,
  ML_MUSVOL,
  ML_LSLEFT,
  ML_LSCNTR,
  ML_LSRGHT,
  ML_GDHIGH,
  ML_GDLOW,
  ML_MSGOFF,
  ML_MSGON,
  ML_OPTTTL,
  ML_THERML,
  ML_THERMM,
  ML_THERMR,
  ML_THERMO,
  ML_CELL1,
  ML_CELL2,
  ML_STDISC,
  ML_STCDROM
} menu_lumps_t;

/* ----------------------------------------------------------------------- */

char* endmsg_orig []=
{
  // DOOM1
  QUITMSG,
  "please don't leave, there's more\ndemons to toast!",
  "let's beat it -- this is turning\ninto a bloodbath!",
  "i wouldn't leave if i were you.\ndos is much worse.",
  "I wouldn't leave if I were you.\nWindows is much worse.",
  "you're trying to say you like dos\nbetter than me, right?",
  "So you're trying to say you like\nWindows better than me?",
  "don't leave yet -- there's a\ndemon around that corner!",
  "Don't leave yet - there's a\ndemon 'round that corner!",
  "ya know, next time you come in here\ni'm gonna toast ya.",
  "go ahead and leave. see if i care.",

  // QuitDOOM II messages
  "you want to quit?\nthen, thou hast lost an eighth!",
  "don't go now, there's a \ndimensional shambler waiting\nat the dos prompt!",
  "Don't go now! There's a dimensional\nshambler waiting on the desktop!",
  "get outta here and go back\nto your boring programs.",
  "Get outta here and go back\nto your precious internet.",
  "if i were your boss, i'd \n deathmatch ya in a minute!",
  "look, bud. you leave now\nand you forfeit your body count!",
  "just leave. when you come\nback, i'll be waiting with a bat.",
  "you're lucky i don't smack\nyou for thinking about leaving.",
#if 0
  // FinalDOOM?
  "fuck you, pussy!\nget the fuck out!",
  "you quit and i'll jizz\nin your cystholes!",
  "if you leave, i'll make\nthe lord drink my jizz.",
  "hey, ron! can we say\n'fuck' in the game?",
  "i'd leave: this is just\nmore monsters and levels.\nwhat a load.",
  "suck it down, asshole!\nyou're a fucking wimp!",
  "don't quit now! we're \nstill spending your money!",

  // Internal debug. Different style, too.
  "THIS IS NO MESSAGE!\nPage intentionally left blank.",
#endif
  0
};

char* endmsg [30];	// Allow a few extra for DEHACK

int qty_endmsg_nums = ARRAY_SIZE(endmsg_orig)-1;

/* ----------------------------------------------------------------------- */

extern patch_t*		hu_font[HU_FONTSIZE];
extern boolean		message_dontfuckwithme;
extern char *		finale_backdrops [];

extern boolean		chat_on;		// in heads-up code

//
// defaulted values
//
int			mouseSensitivity;       // has default

// Show messages has default, 0 = off, 1 = on
int			showMessages;


// Blocky mode, has default, 0 = high, 1 = normal
int			detailLevel;
int			screenblocks;		// has default

// temp for screenblocks (0-9)
int			screenSize;

// -1 = no quicksave slot picked!
static int		quickSaveSlot;

 // 1 = message to be printed
static int		messageToPrint;
// ...and here is the message string!
static char*		messageString;

// message x & y
//static int		messx;
//static int		messy;
static int		messageLastMenuActive;

// timed message = no input from user
static boolean		messageNeedsInput;

void    (*messageRoutine)(int response);

#define SAVESTRINGSIZE 	24


// we are going to be entering a savegame string
static int		saveStringEnter;
static int		saveSlot;	// which slot to save in
// old save description before edit
static char		saveOldString[SAVESTRINGSIZE];

boolean			inhelpscreens;
static int		helpscreennum;
boolean			menuactive;

#define SKULLXOFF	-12
#define LINEHEIGHT	16

#define QTY_SAVE_SLOTS	8
#define SAVEGAME_OFFSET (((QTY_SAVE_SLOTS - 6) / 2) * LINEHEIGHT)


extern boolean		sendpause;
static char		savegamestrings[QTY_SAVE_SLOTS][SAVESTRINGSIZE];

static char		tempstring[160];

extern int		setblocks;
int			show_discicon = 1;
static menu_lumps_t	stdiscicon = ML_STDISC;
static int		stdisctimer = 0;

//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    char	status;

    unsigned char namenum;

    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void	(*routine)(int choice);

    // hotkey in menu
    char	alphaKey;
} menuitem_t;



typedef struct menu_s
{
    dushort_t		numitems;	// # of menu items
    struct menu_s*	prevMenu;	// previous menu
    menuitem_t*		menuitems;	// menu items
    void		(*routine)();	// draw routine
    dushort_t		x;
    dushort_t		y;		// x,y of menu
    dushort_t		lastOn;		// last item user was on in menu
} menu_t;

static dushort_t	itemOn;			// menu item skull is on
static dushort_t	whichSkull;		// which skull to draw


// current menudef
static menu_t*	currentMenu;

char * episode_names [] =
{
  NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};

/* ----------------------------------------------------------------------- */
//
// PROTOTYPES
//

void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_ChangeSensitivity(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
static void M_ChangeDetail(int choice);
static void M_ChangeGraphicDetail(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
int M_WriteText(int x, int y, char *string);
unsigned int M_StringWidth(char *string);
unsigned int M_StringHeight(char *string);
void M_StartControlPanel(void);
void M_StartMessage(char *string,void (*routine)(int response),boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);

/* ----------------------------------------------------------------------- */
//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

static menuitem_t MainMenu[]=
{
    {1,ML_NGAME,M_NewGame,'n'},
    {1,ML_OPTION,M_Options,'o'},
    {1,ML_LOADG,M_LoadGame,'l'},
    {1,ML_SAVEG,M_SaveGame,'s'},
    // Another hickup with Special edition.
    {1,ML_RDTHIS,M_ReadThis,'r'},
    {1,ML_QUITG,M_QuitDOOM,'q'}
};

static menu_t  MainDef =
{
    main_end,
    NULL,
    MainMenu,
    M_DrawMainMenu,
    97,64,
    0
};


//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep_end
} episodes_e;

static menuitem_t EpisodeMenu[]=
{
    {1,ML_EPI0, M_Episode,'0'},
    {1,ML_EPI1, M_Episode,'k'},
    {1,ML_EPI2, M_Episode,'t'},
    {1,ML_EPI3, M_Episode,'i'},
    {1,ML_EPI4, M_Episode,'t'},
    {1,ML_EPI5, M_Episode,'5'},
    {1,ML_EPI6, M_Episode,'6'},
    {1,ML_EPI7, M_Episode,'7'},
    {1,ML_EPI8, M_Episode,'8'},
    {1,ML_EPI9, M_Episode,'9'}
};

static unsigned char episode_num [ARRAY_SIZE(EpisodeMenu)] =
{
  1,2,3,4,5,6,7,8,9,0
};

static int epi;
static unsigned char episode_menu_inhibited = 0;

static menu_t  EpiDef =
{
    0,			// # of menu items (Filled in later if more...)
    &MainDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    48,63,		// x,y
    ep1			// lastOn
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

static menuitem_t NewGameMenu[]=
{
    {1,ML_JKILL, M_ChooseSkill, 'i'},
    {1,ML_ROUGH, M_ChooseSkill, 'h'},
    {1,ML_HURT,  M_ChooseSkill, 'h'},
    {1,ML_ULTRA, M_ChooseSkill, 'u'},
    {1,ML_NMARE, M_ChooseSkill, 'n'}
};

static menu_t  NewDef =
{
    newg_end,		// # of menu items
    &EpiDef,		// previous menu
    NewGameMenu,	// menuitem_t ->
    M_DrawNewGame,	// drawing routine ->
    48,63,		// x,y
    hurtme		// lastOn
};



//
// OPTIONS MENU
//
enum
{
    endgame,
    messages,
    detail,
    scrnsize,
    option_empty1,
    mousesens,
    option_empty2,
    soundvol,
    opt_end
} options_e;

static menuitem_t OptionsMenu[]=
{
    {1,ML_ENDGAM,	M_EndGame,'e'},
    {1,ML_MESSG,	M_ChangeMessages,'m'},
    {1,ML_DETAIL,	M_ChangeGraphicDetail,'g'},
    {2,ML_SCRNSZ,	M_SizeDisplay,'s'},
    {-1,ML_NULL,0},
    {2,ML_MSENS,	M_ChangeSensitivity,'m'},
    {-1,ML_NULL,0},
    {1,ML_SVOL,		M_Sound,'s'}
};

static menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

static menuitem_t ReadMenu1[] =
{
    {1,ML_NULL,M_ReadThis2,0}
};

static menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

static menuitem_t ReadMenu2[]=
{
    {1,ML_NULL,M_FinishReadThis,0}
};

static menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330,175,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    sound_end
} sound_e;

static menuitem_t SoundMenu[]=
{
    {2,ML_SFXVOL,M_SfxVol,'s'},
    {-1,ML_NULL,0},
    {2,ML_MUSVOL,M_MusicVol,'m'},
    {-1,ML_NULL,0}
};

static menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    80,64,
    0
};

static menuitem_t LoadMenu[]=
{
    {1,ML_NULL, M_LoadSelect,'1'},
    {1,ML_NULL, M_LoadSelect,'2'},
    {1,ML_NULL, M_LoadSelect,'3'},
    {1,ML_NULL, M_LoadSelect,'4'},
    {1,ML_NULL, M_LoadSelect,'5'},
    {1,ML_NULL, M_LoadSelect,'6'},
    {1,ML_NULL, M_LoadSelect,'7'},
    {1,ML_NULL, M_LoadSelect,'8'}
};

static menu_t  LoadDef =
{
    QTY_SAVE_SLOTS,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54-SAVEGAME_OFFSET,
    0
};

//
// SAVE GAME MENU
//
static menuitem_t SaveMenu[]=
{
    {1,ML_NULL, M_SaveSelect,'1'},
    {1,ML_NULL, M_SaveSelect,'2'},
    {1,ML_NULL, M_SaveSelect,'3'},
    {1,ML_NULL, M_SaveSelect,'4'},
    {1,ML_NULL, M_SaveSelect,'5'},
    {1,ML_NULL, M_SaveSelect,'6'},
    {1,ML_NULL, M_SaveSelect,'7'},
    {1,ML_NULL, M_SaveSelect,'8'}
};

static menu_t  SaveDef =
{
    QTY_SAVE_SLOTS,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54-SAVEGAME_OFFSET,
    0
};

/* -------------------------------------------------------------------------------------------- */

static void M_DrawDiscIcon (void)
{
  int x;
  int y;
  patch_t * stdisc;

  stdisc = (patch_t *) W_CacheLumpName0 (menu_lump_names[stdiscicon], PU_CACHE);
  if (stdisc)
  {
    x = ((320-2) - HU_MSGX) - SHORT(stdisc->width);
    y = HU_MSGY;
    V_DrawPatchScaled (x, y, 0, stdisc);
  }
}

/* -------------------------------------------------------------------------------------------- */

void M_DrawDisc (void)
{
  if (stdiscicon != ML_NULL)
  {
    stdisctimer = 10;
    M_DrawDiscIcon ();
  }
}

/* -------------------------------------------------------------------------------------------- */

void M_RemoveDisc (void)
{
  stdisctimer = 0;
}

/* -------------------------------------------------------------------------------------------- */

int M_ScreenUpdated (void)
{
  return (stdisctimer != 10);
}

/* -------------------------------------------------------------------------------------------- */
/*
   Sets the episode short cut key from the MAPINFO.
   Gets called before we reorganise the menu, so
   safe to just index in....
*/

void M_SetEpiKey (unsigned int episode, unsigned int key)
{
  menuitem_t * m_ptr;

  if (episode < ARRAY_SIZE(EpisodeMenu))
  {
    m_ptr = &EpisodeMenu [episode];
    m_ptr -> alphaKey = key;
  }
}

/* ----------------------------------------------------------------------- */
/*
   Sets the episode name (patch) from the MAPINFO.
   Gets called before we reorganise the menu, so
   safe to just index in....
*/

void M_SetEpiName (unsigned int episode, char * name, unsigned int len)
{
  unsigned char lumpnum;
  char * newname;
  menuitem_t * m_ptr;

  if (episode < ARRAY_SIZE(EpisodeMenu))
  {
    m_ptr = &EpisodeMenu [episode];
    lumpnum = m_ptr->namenum;
    // name = M_EPI5"  (ie has a trailing quote)
    if (name == NULL)
    {
      menu_lump_names[lumpnum][0] = 0xFF;
    }
    else
    {
      newname = malloc (len+1);
      if (newname)
      {
	strncpy (newname, name, len);
	newname [len] = 0;
	if (strcasecmp (menu_lump_names[lumpnum], name) == 0)
	{
	  free (newname);		// No change - don't bother.
	}
	else
	{
	  menu_lump_names[lumpnum] = newname;
	}
      }
    }
  }
}

/* ----------------------------------------------------------------------- */
/*
   Return the next available episode slot
*/

unsigned int M_GetNextEpi (unsigned int episode, unsigned int map)
{
  unsigned int nextepisode;
  map_starts_t * map_info_p;

  if (episode != 255)
  {
    map_info_p = G_Access_MapStartTab (episode);
    map_info_p -> start_map = map;
    return (episode);
  }

  nextepisode = EpiDef.numitems;
  if (nextepisode < ARRAY_SIZE(episode_num))
  {
    map_info_p = G_Access_MapStartTab (nextepisode);
    map_info_p -> start_map = map;
//  printf ("STARTING MAP %d %d\n", nextepisode, map_info_p -> start_map);
    episode_num [nextepisode] = nextepisode;
    EpiDef.numitems = nextepisode + 1;
  }

  return (nextepisode);
}

/* ----------------------------------------------------------------------- */
/*
   Reset the episode tables
*/

void M_ClearEpiSel (unsigned int episode, unsigned int map)
{
  uint8_t num;
  map_starts_t * map_info_p;

  EpiDef.numitems = 0;
  episode_menu_inhibited = 1;	// Possibly inhibited if nothing added later.
  epi = episode;

  num = 0;
  do
  {
    char * str;
    if ((str = episode_names [num]) != NULL)
      free (str);
    episode_names [num] = NULL;
    episode_num [num] = num + 1;
  } while (++num < ARRAY_SIZE(episode_num));

  // Assume, for the moment, that this map is the start map.
  map_info_p = G_Access_MapStartTab_E (episode);
  map_info_p -> start_map = map;
}

/* ----------------------------------------------------------------------- */
/*
   When we play a map, set the menu cursor to the current position
*/

void M_SetEpiSel (unsigned int episode)
{
  unsigned int this_epi;

  if (gamemode != commercial)
  {
    this_epi = 0;
    do
    {
      if (episode_num [this_epi] == episode)
      {
	EpiDef.lastOn = this_epi;
	break;
      }
    } while (++this_epi < EpiDef.numitems);
  }
}

/* ----------------------------------------------------------------------- */
/*
   When we complete an episode, set the menu cursor to the next
   one ready for selection.
*/

void M_SetNextEpiSel (unsigned int episode)
{
  unsigned int next_epi;

  if (gamemode != commercial)
  {
    next_epi = 0;
    do
    {
      if (++next_epi >= EpiDef.numitems)
	return;
    } while (episode_num [next_epi-1] != episode);

    EpiDef.lastOn = next_epi;
  }
}

/* ----------------------------------------------------------------------- */
//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
  int   i;
  FILE* handle;
  char  name[256];

  i = QTY_SAVE_SLOTS;
  do
  {
    i--;
    G_GetSaveGameName (name, i);

    handle = fopen (name, "rb");
    if (handle == NULL)
    {
      strcpy (savegamestrings[i],menu_messages [MM_EMPTYSTRING]);
      LoadMenu[i].status = 0;
    }
    else
    {
      fread (savegamestrings[i], SAVESTRINGSIZE, 1, handle);
      fclose (handle);
      LoadMenu[i].status = 1;
    }
  } while (i);
}

/* ----------------------------------------------------------------------- */
//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
  int i,y;
  patch_t * loadpatch;

  loadpatch = (patch_t *) W_CacheLumpName(menu_lump_names[ML_LGTTL],PU_CACHE);
  y = LoadDef.y - (SHORT(loadpatch->height) + 9);
  if (y < 0) y = 0;
  V_DrawPatchScaled (72,y,0,loadpatch);

  y = LoadDef.y;
  for (i = 0;i < QTY_SAVE_SLOTS; i++)
  {
      M_DrawSaveLoadBorder(LoadDef.x,y);
      M_WriteText(LoadDef.x,y,savegamestrings[i]);
      y += LINEHEIGHT;
  }
}

/* ----------------------------------------------------------------------- */
//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
  int i;

  V_DrawPatchScaled (x-8,y+7,0,W_CacheLumpName(menu_lump_names[ML_LSLEFT],PU_CACHE));

  for (i = 0;i < 24;i++)
  {
      V_DrawPatchScaled (x,y+7,0,W_CacheLumpName(menu_lump_names[ML_LSCNTR],PU_CACHE));
      x += 8;
  }

  V_DrawPatchScaled (x,y+7,0,W_CacheLumpName(menu_lump_names[ML_LSRGHT],PU_CACHE));
}


/* ----------------------------------------------------------------------- */
//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    char    name[256];
    G_GetSaveGameName (name, choice);
    G_LoadGame (name);
    M_ClearMenus ();
    M_DrawDisc ();
}

/* ----------------------------------------------------------------------- */
//
// Selected from DOOM menu
//
void M_LoadGame (int choice)
{
    if (netgame)
    {
	M_StartMessage(menu_messages [MM_LOADNET],NULL,false);
	return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}

/* ----------------------------------------------------------------------- */
//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
  int i,x,y;
  patch_t * savepatch;

  savepatch = (patch_t *) W_CacheLumpName(menu_lump_names[ML_SGTTL],PU_CACHE);
  y = LoadDef.y - (SHORT(savepatch->height) + 9);
  if (y < 0) y = 0;
  V_DrawPatchScaled (72,y,0,savepatch);

  y = LoadDef.y;
  i = 0;
  do
  {
    M_DrawSaveLoadBorder (LoadDef.x, y);
    x = M_WriteText (LoadDef.x, y, savegamestrings[i]);

    /* Append the flashing cursor if typing */

    if ((saveStringEnter) && (i == saveSlot) && ((whichSkull & 8) == 0))
      M_WriteText (x, y, "_");

    y += LINEHEIGHT;
  } while (++i < QTY_SAVE_SLOTS);
}

/* ----------------------------------------------------------------------- */
//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame (slot,savegamestrings[slot]);
    M_ClearMenus ();
    M_DrawDisc ();

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
	quickSaveSlot = slot;
}

/* ----------------------------------------------------------------------- */
//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;

    saveSlot = choice;
    strcpy(saveOldString,savegamestrings[choice]);
    if (!strcmp(savegamestrings[choice],menu_messages [MM_EMPTYSTRING]))
	savegamestrings[choice][0] = 0;
}

/* ----------------------------------------------------------------------- */
//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
    if (!usergame)
    {
	M_StartMessage(menu_messages [MM_SAVEDEAD],NULL,false);
	return;
    }

    if (gamestate != GS_LEVEL)
	return;

    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

/* ----------------------------------------------------------------------- */
//
//      M_QuickSave
//

void M_QuickSaveResponse(int ch)
{
    switch (ch)
    {
      case 'y':
      case 'Y':
	M_DoSave(quickSaveSlot);
	S_StartSound(NULL,sfx_swtchx);
	break;

      case ' ':
	quickSaveSlot = -1;
	M_ClearMenus ();
	break;
    }

}

/* ----------------------------------------------------------------------- */

void M_QuickSave(void)
{
    if (!usergame)
    {
	S_StartSound(NULL,sfx_oof);
	return;
    }

    if (gamestate != GS_LEVEL)
	return;

    if (quickSaveSlot < 0)
    {
	M_StartControlPanel();
	M_ReadSaveStrings();
	M_SetupNextMenu(&SaveDef);
	quickSaveSlot = -2;	// means to pick a slot now
	return;
    }
    sprintf(tempstring,menu_messages [MM_QSPROMPT],savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring,M_QuickSaveResponse,true);
}

/* ----------------------------------------------------------------------- */
//
// M_QuickLoad
//
void M_QuickLoadResponse(int ch)
{
    if ((ch == 'y') || (ch == 'Y'))
    {
	M_LoadSelect(quickSaveSlot);
	S_StartSound(NULL,sfx_swtchx);
    }
}

/* ----------------------------------------------------------------------- */

void M_QuickLoad(void)
{
    if (netgame)
    {
	M_StartMessage(menu_messages [MM_QLOADNET],NULL,false);
	return;
    }

    if (quickSaveSlot < 0)
    {
	M_StartMessage(menu_messages [MM_QSAVESPOT],NULL,false);
	return;
    }
    sprintf(tempstring,menu_messages [MM_QLPROMPT],savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring,M_QuickLoadResponse,true);
}

/* ----------------------------------------------------------------------- */
//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
    char patchname [10];

    inhelpscreens = true;

    sprintf (patchname, finale_backdrops[BG_HELPxx], 1);
    if (W_CheckNumForName (patchname) != -1)
    {
      helpscreennum = 1;
      D_PageDrawer (patchname);
      return;
    }

    helpscreennum = 0;

    switch ( gamemode )
    {
      case commercial:
	D_PageDrawer (finale_backdrops[BG_HELP]);
	break;
      case shareware:
      case registered:
      case retail:
	D_PageDrawer (finale_backdrops[BG_HELP1]);
	break;
      default:
	break;
    }
    return;
}

/* ----------------------------------------------------------------------- */
//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
    inhelpscreens = true;

    // Clear the screen memory first in case the graphic
    // doesn't fill the display - which it won't do if
    // we're larger than 320x200.
    // Potential problem here in that colour 0 may not be
    // black! Should really search through the palette table
    // and find which colour maps to black.

    memset (screens[0], 0, SCREENHEIGHT*SCREENWIDTH);

    if (helpscreennum)
    {
      char patchname [10];
      sprintf (patchname, finale_backdrops[BG_HELPxx], helpscreennum);
      D_PageDrawer (patchname);
      return;
    }

    switch ( gamemode )
    {
      case retail:
      case commercial:
	// This hack keeps us from having to change menus.
	D_PageDrawer (finale_backdrops[BG_CREDIT]);
	break;
      case shareware:
      case registered:
	D_PageDrawer (finale_backdrops[BG_HELP2]);
	break;
      default:
	break;
    }
    return;
}

/* ----------------------------------------------------------------------- */
//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
    V_DrawPatchScaled (60,38,0,W_CacheLumpName(menu_lump_names[ML_SVOL],PU_CACHE));

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(sfx_vol+1),
		 16,snd_SfxVolume);

    M_DrawThermo(SoundDef.x,SoundDef.y+LINEHEIGHT*(music_vol+1),
		 16,snd_MusicVolume);
}

/* ----------------------------------------------------------------------- */

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

/* ----------------------------------------------------------------------- */

void M_SfxVol(int choice)
{
    switch(choice)
    {
      case 0:
	if (snd_SfxVolume)
	    snd_SfxVolume--;
	break;
      case 1:
	if (snd_SfxVolume < 15)
	    snd_SfxVolume++;
	break;
    }

    S_SetSfxVolume(snd_SfxVolume /* *8 */);
}

/* ----------------------------------------------------------------------- */

void M_MusicVol(int choice)
{
    switch(choice)
    {
      case 0:
	if (snd_MusicVolume)
	    snd_MusicVolume--;
	break;
      case 1:
	if (snd_MusicVolume < 15)
	    snd_MusicVolume++;
	break;
    }

    S_SetMusicVolume(snd_MusicVolume /* *8 */);
}

/* ----------------------------------------------------------------------- */
//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
    V_DrawPatchScaled (94,2,0,W_CacheLumpName(menu_lump_names[ML_DOOM],PU_CACHE));
}

/* ----------------------------------------------------------------------- */
//
// M_NewGame
//
void M_DrawNewGame(void)
{
    V_DrawPatchScaled (96,14,0,W_CacheLumpName(menu_lump_names[ML_NEWG],PU_CACHE));
    V_DrawPatchScaled (54,38,0,W_CacheLumpName(menu_lump_names[ML_SKILL],PU_CACHE));
}

void M_NewGame(int choice)
{
    if (netgame && !demoplayback)
    {
	M_StartMessage(menu_messages [MM_NEWGAME],NULL,false);
	return;
    }

//  if ( gamemode == commercial )
    if (EpiDef.numitems < 2)
	M_SetupNextMenu(&NewDef);
    else
	M_SetupNextMenu(&EpiDef);
}

/* ----------------------------------------------------------------------- */
//
//      M_Episode
//

void M_DrawEpisode(void)
{
  V_DrawPatchScaled (54,EpiDef.y-25/*38*/,0,W_CacheLumpName(menu_lump_names[ML_EPISOD],PU_CACHE));
}

/* ----------------------------------------------------------------------- */

void M_DeferedInitNew (int ch, int epi)
{
  map_starts_t * map_info_p;

  map_info_p = G_Access_MapStartTab (epi);
  // printf ("STARTING MAP %d %d %d\n", ch, epi, map_info_p -> start_map);
  if (gamemode != commercial)
    epi = map_info_p -> start_episode;
  G_DeferedInitNew ((skill_t)ch, epi, map_info_p -> start_map);
}

/* ----------------------------------------------------------------------- */

void M_VerifyNightmare(int ch)
{
  if ((ch == 'y') || (ch == 'Y'))
  {
    M_DeferedInitNew (nightmare, epi);
    M_ClearMenus ();
    M_DrawDisc ();
  }
}

/* ----------------------------------------------------------------------- */

void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
	M_StartMessage(menu_messages [MM_NIGHTMARE],M_VerifyNightmare,true);
	return;
    }

    M_DeferedInitNew (choice, epi);
    M_ClearMenus ();
    M_DrawDisc ();
}

/* ----------------------------------------------------------------------- */

void M_Episode(int choice)
{
    if ( (gamemode == shareware)
	 && choice)
    {
	M_StartMessage(menu_messages [MM_SWSTRING],NULL,false);
	M_SetupNextMenu(&ReadDef1);
	return;
    }

    epi = episode_num [choice];
    M_SetupNextMenu(&NewDef);
}

/* ----------------------------------------------------------------------- */

//
// M_Options
//
unsigned char	detailNames[2]	= {ML_GDHIGH, ML_GDLOW};
unsigned char	msgNames[2]	= {ML_MSGOFF, ML_MSGON};


void M_DrawOptions(void)
{
  int		x,y;
  patch_t*	patch1;
  patch_t*	patch2;

  V_DrawPatchScaled (108,15,0,W_CacheLumpName(menu_lump_names[ML_OPTTTL],PU_CACHE));

  patch1 = W_CacheLumpName (menu_lump_names[OptionsMenu[1].namenum],PU_CACHE);
  patch2 = W_CacheLumpName (menu_lump_names[msgNames[showMessages]],PU_CACHE);
  x = OptionsDef.x + SHORT(patch1->width) + SHORT(patch2->leftoffset) + 2;
  y = OptionsDef.y + (LINEHEIGHT*messages);
  V_DrawPatchScaled (x,y,0,patch2);

  patch1 = W_CacheLumpName (menu_lump_names[OptionsMenu[2].namenum],PU_CACHE);
  patch2 = W_CacheLumpName (menu_lump_names[detailNames[detailLevel]],PU_CACHE);
  x = OptionsDef.x + SHORT(patch1->width) + SHORT(patch2->leftoffset) + 2;
  y = OptionsDef.y + (LINEHEIGHT*detail);
  V_DrawPatchScaled (x,y,0,patch2);

  M_DrawThermo (OptionsDef.x,OptionsDef.y+LINEHEIGHT*(mousesens+1),10,mouseSensitivity);
  M_DrawThermo (OptionsDef.x,OptionsDef.y+LINEHEIGHT*(scrnsize+1),9,screenSize);
}

/* ----------------------------------------------------------------------- */

void M_Options(int choice)
{
    M_SetupNextMenu (&OptionsDef);
}

/* ----------------------------------------------------------------------- */
//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;

    if (!showMessages)
	players[consoleplayer].message = menu_messages [MM_MSGOFF];
    else
	players[consoleplayer].message = menu_messages [MM_MSGON];

    message_dontfuckwithme = true;
}

/* ----------------------------------------------------------------------- */
//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
  if ((ch == 'y') || (ch == 'Y'))
  {
    currentMenu->lastOn = itemOn;
    M_ClearMenus ();
    D_StartTitle ();
  }
}

/* ----------------------------------------------------------------------- */

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
    {
	S_StartSound(NULL,sfx_oof);
	return;
    }

    if (netgame)
    {
	M_StartMessage(menu_messages [MM_NETEND],NULL,false);
	return;
    }

    M_StartMessage(menu_messages [MM_ENDGAME],M_EndGameResponse,true);
}

/* ----------------------------------------------------------------------- */
//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

/* ----------------------------------------------------------------------- */

void M_ReadThis2(int choice)
{
    char patchname [10];

    choice = 0;

    if (helpscreennum)
    {
      sprintf (patchname, finale_backdrops[BG_HELPxx], ++helpscreennum);
      if (W_CheckNumForName (patchname) == -1)
      {
	helpscreennum = 0;
      }
    }
    M_SetupNextMenu(&ReadDef2);
}

/* ----------------------------------------------------------------------- */

void M_FinishReadThis(int choice)
{
    char patchname [10];

    choice = 0;

    if (helpscreennum)
    {
      sprintf (patchname, finale_backdrops[BG_HELPxx], ++helpscreennum);
      if (W_CheckNumForName (patchname) != -1)
      {
	return;
      }
      helpscreennum = 0;
    }

    M_SetupNextMenu(&MainDef);
}

/* ----------------------------------------------------------------------- */
//
// M_QuitDOOM
//
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_getpow,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};

/* ----------------------------------------------------------------------- */

void M_QuitResponse(int ch)
{
  if ((ch == 'y') || (ch == 'Y'))
  {
    if (!netgame)
    {
	if (gamemode == commercial)
	    S_StartSound(NULL,quitsounds2[(gametic>>2)&7]);
	else
	    S_StartSound(NULL,quitsounds[(gametic>>2)&7]);
	I_WaitVBL(105);
    }
    I_Quit ();
  }
}

/* ----------------------------------------------------------------------- */


void M_QuitDOOM (int choice)
{
  int gt;
  unsigned int msgnum;
  const char * msg_ptr;

  // We pick index 0 which is language sensitive,
  //  or one at random, between 0 and maximum number.

  if (language != english)
  {
    msg_ptr = endmsg [0];
  }
  else
  {
    gt = gametic;
    do
    {
      gt++;
      msgnum = gt%qty_endmsg_nums;
      msg_ptr = endmsg [msgnum];
    } while (msg_ptr == NULL);		// In case the DEHACK left gaps.
  }

  // printf ("%d messages available\n", qty_endmsg_nums);
  sprintf(tempstring,"%s\n\n%s", msg_ptr, menu_messages [MM_DOSY]);
  M_StartMessage(tempstring,M_QuitResponse,true);
}

/* ----------------------------------------------------------------------- */

void M_ChangeSensitivity(int choice)
{
    switch(choice)
    {
      case 0:
	if (mouseSensitivity)
	    mouseSensitivity--;
	break;
      case 1:
	if (mouseSensitivity < 9)
	    mouseSensitivity++;
	break;
    }
}

/* ----------------------------------------------------------------------- */

static void M_ChangeDetail (int choice)
{
  unsigned int flags;

  choice = 0;
  // detailLevel = 1 - detailLevel;
  // detailLevel = 0;

  flags = 7;				// Work out what is visible.

  if (SCREENWIDTH <= 320)
    flags &= ~5;

  if (automapactive)			// If cannot see the weapon
    flags &= ~2;
  else if (setblocks == 11)		// If cannot see or scale the status bar
    flags &= ~1;

  // Ok, this is a terrible way to do it....

  switch (flags)
  {
    case 0x07:
      if (stbar_scale > 1) choice |= 1;
      if (weaponscale > 1) choice |= 2;
      if (hutext_scale > 1) choice |= 4;
      choice++;
      if (choice & 1) stbar_scale = 2; else stbar_scale = 1;
      if (choice & 2) weaponscale = 2; else weaponscale = 1;
      if (choice & 4) hutext_scale = 2; else hutext_scale = 1;
      break;

    case 0x06:
      if (weaponscale > 1) choice |= 1;
      if (hutext_scale > 1) choice |= 2;
      choice++;
      if (choice & 1) weaponscale = 2; else weaponscale = 1;
      if (choice & 2) hutext_scale = 2; else hutext_scale = 1;
      break;

    case 0x05:
      if (stbar_scale > 1) choice |= 1;
      if (hutext_scale > 1) choice |= 2;
      choice++;
      if (choice & 1) stbar_scale = 2; else stbar_scale = 1;
      if (choice & 2) hutext_scale = 2; else hutext_scale = 1;
      break;

    case 0x04:
      if (hutext_scale == 1)
	hutext_scale = 2;
      else
	hutext_scale = 1;
      break;

    case 0x03:
      if (stbar_scale > 1) choice |= 1;
      if (weaponscale > 1) choice |= 2;
      choice++;
      if (choice & 1) stbar_scale = 2; else stbar_scale = 1;
      if (choice & 2) weaponscale = 2; else weaponscale = 1;
      break;

    case 0x02:
      if (weaponscale == 1)
	weaponscale = 2;
      else
	weaponscale = 1;
      break;

    case 0x01:
      if (stbar_scale == 1)
	stbar_scale = 2;
      else
	stbar_scale = 1;
      break;
  }

  if (flags)
    R_SetViewSize (2, screenblocks, 0 /*detailLevel*/);
}

/* ----------------------------------------------------------------------- */

static void M_ChangeGraphicDetail (int choice)
{
  choice = 0;
  detailLevel = 1 - detailLevel;

  R_SetViewSize (1, screenblocks, 0 /*detailLevel*/);
}

/* ----------------------------------------------------------------------- */

void M_SizeDisplay(int choice)
{
    switch(choice)
    {
      case 0:
	if (screenSize > 0)
	{
	    screenblocks--;
	    screenSize--;
	}
	break;
      case 1:
	if (screenSize < 8)
	{
	    screenblocks++;
	    screenSize++;
	}
	break;
    }


    R_SetViewSize (1, screenblocks, 0 /*detailLevel*/);
}

/* ----------------------------------------------------------------------- */
//
//      Menu Functions
//
void
M_DrawThermo
( int	x,
  int	y,
  int	thermWidth,
  int	thermDot )
{
  int		i;
  int		xx;
  patch_t*	patchl;
  patch_t*	patchm;
  patch_t*	patchr;
  patch_t*	patcho;

  xx = x;
  patchl = W_CacheLumpName(menu_lump_names[ML_THERML],PU_CACHE);
  V_DrawPatchScaled (xx,y,0,patchl);

  xx += 8;
  patchm = W_CacheLumpName(menu_lump_names[ML_THERMM],PU_CACHE);
  for (i=0;i<thermWidth;i++)
  {
    V_DrawPatchScaled (xx,y,0,patchm);
    xx += 8;
  }

  patchr = W_CacheLumpName(menu_lump_names[ML_THERMR],PU_CACHE);
  V_DrawPatchScaled (xx,y,0,patchr);

  patcho = W_CacheLumpName(menu_lump_names[ML_THERMO],PU_CACHE);
  V_DrawPatchScaled ((x+8) + thermDot*8,y,0,patcho);
}

/* ----------------------------------------------------------------------- */

void
M_DrawEmptyCell
( menu_t*	menu,
  int		item )
{
    V_DrawPatchScaled (menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
		       W_CacheLumpName(menu_lump_names[ML_CELL1],PU_CACHE));
}

/* ----------------------------------------------------------------------- */

void
M_DrawSelCell
( menu_t*	menu,
  int		item )
{
    V_DrawPatchScaled (menu->x - 10, menu->y+item*LINEHEIGHT - 1, 0,
		       W_CacheLumpName(menu_lump_names[ML_CELL2],PU_CACHE));
}

/* ----------------------------------------------------------------------- */

void
M_StartMessage
( char*		string,
  void		(*routine)(int response),
  boolean	input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
    return;
}

/* ----------------------------------------------------------------------- */


void M_StopMessage(void)
{
    menuactive = (boolean) messageLastMenuActive;
    messageToPrint = 0;
}

/* ----------------------------------------------------------------------- */

//
// Find string width from hu_font chars
//
unsigned int M_StringWidth (char* string)
{
    char c;
    unsigned int w;

    w = 0;
    do
    {
      c = *string++;
      if (c == 0)
	break;
      c = toupper(c) - HU_FONTSTART;
      if (c < 0 || c >= HU_FONTSIZE)
	w += 4;
      else
	w += (SHORT (hu_font[c]->width)) - HUlib_Kern (c+HU_FONTSTART, toupper(*string));
    } while (1);

    return (w);
}

/* ----------------------------------------------------------------------- */
//
//      Find string height from hu_font chars
//
unsigned int M_StringHeight (char* string)
{
    char c;
    unsigned int h;
    unsigned int height;

    height = SHORT(hu_font[0]->height);
    h = height;

    do
    {
      c = *string++;
      if (c == '\n')
	h += height;
    } while (c);

    return (h);
}

/* ----------------------------------------------------------------------- */
//
//      Write a string using the hu_font
//
int
M_WriteText
( int		x,
  int		y,
  char*		string)
{
    int		w;
    char*	ch;
    int		c;
    int		cx;
    int		cy;


    ch = string;
    cx = x;
    cy = y;

    while(1)
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = x;
	    cy += 12;
	    continue;
	}

	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c>= HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	if (cx+w > SCREENWIDTH)
	    break;
	V_DrawPatchScaled(cx, cy, 0, hu_font[c]);
	w -= HUlib_Kern (c+HU_FONTSTART, toupper(*ch));
	cx+=w;
    }

    return (cx);
}

/* ----------------------------------------------------------------------- */
//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int		ch;
    int		i;
    int		saveCharIndex;
    static int	joywait = 0;
    static int	mousewait = 0;
    static int	mousey = 0;
    static int	lasty = 0;
    static int	mousex = 0;
    static int	lastx = 0;

    ch = -1;

    if (ev->type == ev_joystick && joywait < I_GetTime())
    {
	if (ev->data3 == -1)
	{
	    ch = KEY_UPARROW;
	    joywait = I_GetTime() + 5;
	}
	else if (ev->data3 == 1)
	{
	    ch = KEY_DOWNARROW;
	    joywait = I_GetTime() + 5;
	}

	if (ev->data2 == -1)
	{
	    ch = KEY_LEFTARROW;
	    joywait = I_GetTime() + 2;
	}
	else if (ev->data2 == 1)
	{
	    ch = KEY_RIGHTARROW;
	    joywait = I_GetTime() + 2;
	}

	if (ev->data1&1)
	{
	    ch = KEY_ENTER;
	    joywait = I_GetTime() + 5;
	}
	if (ev->data1&2)
	{
	    ch = KEY_BACKSPACE;
	    joywait = I_GetTime() + 5;
	}
    }
    else
    {
	if (ev->type == ev_mouse && mousewait < I_GetTime())
	{
	    mousey += ev->data3;
	    if (mousey < lasty-30)
	    {
		ch = KEY_DOWNARROW;
		mousewait = I_GetTime() + 5;
		mousey = lasty -= 30;
	    }
	    else if (mousey > lasty+30)
	    {
		ch = KEY_UPARROW;
		mousewait = I_GetTime() + 5;
		mousey = lasty += 30;
	    }

	    mousex += ev->data2;
	    if (mousex < lastx-30)
	    {
		ch = KEY_LEFTARROW;
		mousewait = I_GetTime() + 5;
		mousex = lastx -= 30;
	    }
	    else if (mousex > lastx+30)
	    {
		ch = KEY_RIGHTARROW;
		mousewait = I_GetTime() + 5;
		mousex = lastx += 30;
	    }

	    if (ev->data1&1)
	    {
		ch = KEY_ENTER;
		mousewait = I_GetTime() + 15;
	    }

	    if (ev->data1&2)
	    {
		ch = KEY_BACKSPACE;
		mousewait = I_GetTime() + 15;
	    }
	}
	else
	    if (ev->type == ev_keydown)
	    {
		ch = ev->data1;
	    }
    }

    if (ch == -1)
	return false;


    // Save Game string input
    if (saveStringEnter)
    {
	saveCharIndex = strlen(savegamestrings[saveSlot]);
	switch(ch)
	{
	  case KEY_BACKSPACE:
	    if (saveCharIndex > 0)
	    {
		saveCharIndex--;
		savegamestrings[saveSlot][saveCharIndex] = 0;
	    }
	    break;

	  case KEY_ESCAPE:
	    saveStringEnter = 0;
	    strcpy(&savegamestrings[saveSlot][0],saveOldString);
	    break;

	  case KEY_ENTER:
	    saveStringEnter = 0;
	    if (savegamestrings[saveSlot][0])
		M_DoSave(saveSlot);
	    break;

	  default:
	    ch = toupper(ch);
	    if (ch != 32)
		if (ch-HU_FONTSTART < 0 || ch-HU_FONTSTART >= HU_FONTSIZE)
		    break;
	    if (ch >= 32 && ch <= 127 &&
		saveCharIndex < SAVESTRINGSIZE-1 &&
		M_StringWidth(savegamestrings[saveSlot]) <
		(SAVESTRINGSIZE-2)*8)
	    {
		savegamestrings[saveSlot][saveCharIndex++] = ch;
		savegamestrings[saveSlot][saveCharIndex] = 0;
	    }
	    break;
	}
	return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
	if (messageNeedsInput == true &&
	    !(ch == ' ' || ch == 'n' || ch == 'N' || ch == 'y' || ch == 'Y' || ch == KEY_ESCAPE))
	    return false;

	menuactive = (boolean) messageLastMenuActive;
	messageToPrint = 0;
	if (messageRoutine)
	    messageRoutine(ch);

	menuactive = false;
	S_StartSound(NULL,sfx_swtchx);
	return true;
    }

    if (ch == KEY_PRINTSCRN)
    {
	G_ScreenShot ();
	return true;
    }


    // F-Keys
    if (!menuactive)
	switch(ch)
	{
	  case KEY_MINUS:	// Screen size down
	    if (automapactive || chat_on)
		return false;
	    M_SizeDisplay(0);
	    S_StartSound(NULL,sfx_stnmov);
	    return true;

	  case KEY_EQUALS:	// Screen size up
	  case KEY_PLUS:
	    if (automapactive || chat_on)
		return false;
	    M_SizeDisplay(1);
	    S_StartSound(NULL,sfx_stnmov);
	    return true;

	  case KEY_F1:		// Help key
	    M_StartControlPanel ();

	    if ( gamemode == retail )
	      currentMenu = &ReadDef2;
	    else
	      currentMenu = &ReadDef1;

	    itemOn = 0;
	    S_StartSound(NULL,sfx_swtchn);
	    return true;

	  case KEY_F2:		// Save
	    M_StartControlPanel();
	    S_StartSound(NULL,sfx_swtchn);
	    M_SaveGame(0);
	    return true;

	  case KEY_F3:		// Load
	    M_StartControlPanel();
	    S_StartSound(NULL,sfx_swtchn);
	    M_LoadGame(0);
	    return true;

	  case KEY_F4:		// Sound Volume
	    M_StartControlPanel ();
	    currentMenu = &SoundDef;
	    itemOn = sfx_vol;
	    S_StartSound(NULL,sfx_swtchn);
	    return true;

	  case KEY_F5:		// Detail toggle
	    M_ChangeDetail(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;

	  case KEY_F6:		// Quicksave
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuickSave();
	    return true;

	  case KEY_F7:		// End game
	    S_StartSound(NULL,sfx_swtchn);
	    M_EndGame(0);
	    return true;

	  case KEY_F8:		// Toggle messages
	    M_ChangeMessages(0);
	    S_StartSound(NULL,sfx_swtchn);
	    return true;

	  case KEY_F9:		// Quickload
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuickLoad();
	    return true;

	  case KEY_F10:		// Quit DOOM
	    S_StartSound(NULL,sfx_swtchn);
	    M_QuitDOOM(0);
	    return true;

	  case KEY_F11:		// gamma toggle
	    if (gamekeydown[KEY_RSHIFT])
	    {
	      lightScaleShift--;
	      if (lightScaleShift < 9)
		lightScaleShift = 12;
	      players[consoleplayer].message = HU_printf ("lightScaleShift = %u", lightScaleShift);
	    }
	    else
	    {
	      usegamma++;
	      if (usegamma > 4)
		usegamma = 0;
	      players[consoleplayer].message = menu_messages[MM_GAMMALVL0+usegamma];
	      I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));
	    }
	    return true;
	}


    // Pop-up menu?
    if (!menuactive)
    {
	if (ch == KEY_ESCAPE)
	{
	    M_StartControlPanel ();
	    S_StartSound(NULL,sfx_swtchn);
	    return true;
	}
	return false;
    }


    // Keys usable within menu
    switch (ch)
    {
      case KEY_DOWNARROW:
	do
	{
	    if (itemOn+1 > currentMenu->numitems-1)
		itemOn = 0;
	    else itemOn++;
	} while((unsigned)currentMenu->menuitems[itemOn].status>2);
	S_StartSound(NULL,sfx_pstop);
	return true;

      case KEY_UPARROW:
	do
	{
	    if (!itemOn)
		itemOn = currentMenu->numitems-1;
	    else itemOn--;
	} while((unsigned)currentMenu->menuitems[itemOn].status>2);
	S_StartSound(NULL,sfx_pstop);
	return true;

      case KEY_LEFTARROW:
	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status == 2)
	{
	    S_StartSound(NULL,sfx_stnmov);
	    currentMenu->menuitems[itemOn].routine(0);
	}
	return true;

      case KEY_RIGHTARROW:
	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status == 2)
	{
	    S_StartSound(NULL,sfx_stnmov);
	    currentMenu->menuitems[itemOn].routine(1);
	}
	return true;

      case KEY_ENTER:
	if (currentMenu->menuitems[itemOn].routine &&
	    currentMenu->menuitems[itemOn].status)
	{
	    currentMenu->lastOn = itemOn;
	    if (currentMenu->menuitems[itemOn].status == 2)
	    {
		currentMenu->menuitems[itemOn].routine(1);      // right arrow
		S_StartSound(NULL,sfx_stnmov);
	    }
	    else
	    {
		currentMenu->menuitems[itemOn].routine(itemOn);
		S_StartSound(NULL,sfx_pistol);
	    }
	}
	return true;

      case KEY_ESCAPE:
	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	S_StartSound(NULL,sfx_swtchx);
	return true;

      case KEY_BACKSPACE:
	currentMenu->lastOn = itemOn;
	if (currentMenu->prevMenu)
	{
	    currentMenu = currentMenu->prevMenu;
	    itemOn = currentMenu->lastOn;
	    S_StartSound(NULL,sfx_swtchn);
	}
	return true;

      default:
	for (i = itemOn+1;i < currentMenu->numitems;i++)
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
		itemOn = i;
		S_StartSound(NULL,sfx_pstop);
		return true;
	    }
	for (i = 0;i <= itemOn;i++)
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
		itemOn = i;
		S_StartSound(NULL,sfx_pstop);
		return true;
	    }
	break;

    }

    return false;
}

/* ----------------------------------------------------------------------- */
//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
  menuactive = true;
  M_SetupNextMenu (&MainDef);	// JDC
}

/* ----------------------------------------------------------------------- */
//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
  char		d;
  int		c;
  int		x;
  int		y;
  int		i;
  int		len;
  int		lump;
  unsigned int	max,episode;
  char*		start;
  patch_t*	patch;
  menuitem_t * 	m_ptr;
  char		string[44];

  inhelpscreens = false;


  // Horiz. & Vertically centre string and print it.
  if (messageToPrint)
  {
    start = messageString;
    c = 0;
    y = 100 - (M_StringHeight(messageString)/2);
    do
    {
      d = *start;
      if (d == '\n') d = 0;
      string[c++] = d;
      if (d == 0)
      {
	x = 160 - (M_StringWidth(string)/2);
	// printf ("MESSAGE: %d,%d:%s\n", x,y,string);
	M_WriteText(x,y,string);
	y += SHORT(hu_font[0]->height);
	c = 0;
      }
    } while (*start++);
    return;
  }

  if (menuactive)
  {
    if (currentMenu->routine)
      currentMenu->routine();		// call Draw routine

    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;

    if (max)				// Just in case!
    {
      m_ptr = currentMenu->menuitems;
      do
      {
	if (m_ptr -> namenum != ML_NULL)
	{
	  lump = W_CheckNumForName (menu_lump_names[m_ptr -> namenum]);
	  if (lump == -1)
	  {
	    if (currentMenu == &EpiDef)
	    {
	      episode = episode_num [(currentMenu->numitems - max)];
	      if (episode_names [episode] == NULL)
	      {
		sprintf (string, "Episode %u", episode);
		V_drawMenuText (x, y, string);
	      }
	      else
	      {
		if ((V_drawMenuText (x, y, episode_names [episode]) == 0)	// Did it fit?
		 && (x))
		  currentMenu->x = --x;
	      }
	    }
	  }
	  else
	  {
	    patch = W_CacheLumpNum (lump,PU_CACHE);
	    len = SHORT(patch->width) + x;
	    if ((len >= 319)			// Will it fit?
	     && (x > 0))
	    {
	      x -= (len - (320-2));
	      if (x < 0)
		x = 0;
	      currentMenu->x = x;
	    }
	    V_DrawPatchScaled (x,y,0,patch);
	  }
	}
	y += LINEHEIGHT;
	m_ptr++;
      } while (--max);
    }


    // DRAW SKULL
    if (currentMenu->numitems > 1)
    {
      unsigned int skullnum;

      skullnum = ML_SKULL1 + (whichSkull>>3);
      patch = W_CacheLumpName(menu_lump_names[skullnum],PU_CACHE);
      i = (SHORT(patch->width)) - SKULLXOFF;
      if (i < 0) i = 0;
      x -= i;
      if (x < 0) x = 0;
      i = SHORT(patch->leftoffset);
      if ((x - i) < 0)
	x = i;
      V_DrawPatchScaled(x, currentMenu->y - 5 + (itemOn*LINEHEIGHT), 0, patch);
    }
  }

  if (stdisctimer)
  {
    stdisctimer--;
    M_DrawDiscIcon ();
  }
}

/* ----------------------------------------------------------------------- */
//
// M_ClearMenus
//
void M_ClearMenus (void)
{
  menuactive = false;
  // if (!netgame && usergame && paused)
  //       sendpause = true;
  st_firsttime = true;
}

/* ----------------------------------------------------------------------- */
//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
  currentMenu = menudef;
  itemOn = currentMenu->lastOn;
}

/* ----------------------------------------------------------------------- */
//
// M_Ticker
//
void M_Ticker (void)
{
  whichSkull = (whichSkull + 1) & 15;
}

/* ----------------------------------------------------------------------- */
/*
   2002ado.wad has E5M1, but this is a secret level and does not need
   a menu entry. So we assume that for the menu entry then at least 8
   maps must exist or the predefined bitmap.
*/

static int M_EpisodePresent (const char * name, unsigned int episode)
{
  unsigned int map;

  if ((W_CheckNumForName (name) == -1)
   && (episode_names [episode] == NULL))
  {
    map = 1;
    do
    {
      if (G_MapLump (episode, map) == -1)
	return (0);
    } while (++map < 9);
  }
  return (1);
}

/* ----------------------------------------------------------------------- */

static void M_SetEpisodeMenuPos (void)
{
  int lump;
  unsigned int len;
  unsigned int episode;
  char * str;
  menuitem_t * m_ptr;
  map_starts_t *  map_info_p;

  episode = EpiDef.numitems;

  while (episode > 4)
  {
    if (EpiDef.y > 30)
      EpiDef.y -= 4;
    episode--;
  }

  /* Now check for oversize episode patches (e.g. Valiant.wad) */
  episode = 0;
  m_ptr = &EpisodeMenu[0];
  do
  {
    len = 0;
    lump = -1;
    if (m_ptr -> namenum != ML_NULL)
      lump = W_CheckNumForName (menu_lump_names[m_ptr -> namenum]);
    if (lump == -1)
    {
      str = episode_names[episode_num [episode]];
      if (str != NULL)
	len = V_MenuTextWidth (str);
    }
    else
    {
      patch_t* patch = W_CacheLumpNum (lump,PU_CACHE);
      len = SHORT(patch->width);
    }
    len += EpiDef.x;
    if (len >= 319)			// Will it fit?
    {
      int x = EpiDef.x - (len - (320-2));
      if (x < 0)
	x = 0;
      EpiDef.x = x;
    }
    m_ptr++;
  } while ((EpiDef.x) && (++episode < EpiDef.numitems));

  if (M_CheckParm("-showepisodetable"))
  {
    // DEBUG !!
    m_ptr = &EpisodeMenu[0];
    episode = 0;
    do
    {
      printf ("Menu entry %u is %s -> episode %u", episode, menu_lump_names[m_ptr -> namenum], episode_num [episode]);
      str = episode_names[episode_num [episode]];
      if (str != NULL)
	printf (" (%s)", str);
      map_info_p = G_Access_MapStartTab (episode_num [episode]);
      printf (" (%u,%u)\n", map_info_p->start_episode, map_info_p -> start_map);
      m_ptr++;
    } while (++episode < EpiDef.numitems);
  }
}

/* ----------------------------------------------------------------------- */
/*
   If the Load/Save titles are in the PWAD then use them.
*/

static void M_SetLoadSaveLumps (int menu_lump_num, int title_lump_num)
{
  int menu_lump;
  int title_lump;

  menu_lump = W_CheckNumForName (menu_lump_names [menu_lump_num]);
  title_lump = W_CheckNumForName (menu_lump_names [title_lump_num]);

  if (title_lump < 0)
  {
    menu_lump_names [title_lump_num] = menu_lump_names [menu_lump_num];
  }
  else if (menu_lump < 0)	// Cannot happen!
  {
    menu_lump_names [menu_lump_num] = menu_lump_names [title_lump_num];
  }
  else if ((lumpinfo[0].handle == lumpinfo[title_lump].handle)  // If the title lump is in a PWAD then use it.
	&& (lumpinfo[0].handle != lumpinfo[menu_lump].handle))  // If the menu lump is in a PWAD then use it.
  {
    menu_lump_names [title_lump_num] = menu_lump_names [menu_lump_num];
  }

//printf ("Using %s\n", menu_lump_names [title_lump_num]);
}

/* ----------------------------------------------------------------------- */
//
// M_Init
//
void M_Init (void)
{
  unsigned int menu_pos;
  unsigned int episode;
  unsigned int lump1,lump2;
  menuitem_t * m_ptr;

  M_SetLoadSaveLumps (ML_LOADG, ML_LGTTL);
  M_SetLoadSaveLumps (ML_SAVEG, ML_SGTTL);

  currentMenu = &MainDef;
  menuactive = false;
  itemOn = currentMenu->lastOn;
  whichSkull = 0;
  screenSize = screenblocks - 3;
  messageToPrint = 0;
  messageString = NULL;
  messageLastMenuActive = menuactive;
  quickSaveSlot = -1;

  // Here we could catch other version dependencies,
  //  like HELP1/2, and four episodes.

  switch (gamemode)
  {
    case commercial:
      // This is used because DOOM 2 had only one HELP
      // page. I use CREDIT as second page now, but
      // kept this hack for educational purposes.
      MainMenu[readthis] = MainMenu[quitdoom];
      MainDef.numitems--;
      MainDef.y += 8;
      NewDef.prevMenu = &MainDef;
      ReadDef1.routine = M_DrawReadThis1;
      ReadDef1.x = 330;
      ReadDef1.y = 165;
      ReadMenu1[0].routine = M_FinishReadThis;

      if (EpiDef.numitems)			// Has the MAPINFO reader created
      {						// an episode menu?
	episode = 0;
	m_ptr = &EpisodeMenu[0];
	do
	{
	  /* If the episode maps are in a pwad and the episode_names[] has */
	  /* been provided but not an M_EPIx lump then destroy the lump. */
	  if ((episode_names[episode] != NULL)
	   && ((lump1 = W_CheckNumForName (menu_lump_names[m_ptr -> namenum])) != -1)
	   && ((lump2 = G_MapLump (255, G_Access_MapStartTab (episode_num [episode]) -> start_map)) != -1)
	   && (lumpinfo[lump1].handle == lumpinfo[0].handle)
	   && (lumpinfo[lump2].handle != lumpinfo[0].handle))
	  {
//	    printf ("Destroyed menu lump %u for map %u\n", episode, G_Access_MapStartTab (episode_num [episode]) -> start_map);
	    menu_lump_names[m_ptr -> namenum][0] = 0xFF;
	  }
	  m_ptr++;
	} while (++episode < EpiDef.numitems);

	M_SetEpisodeMenuPos ();
      }
      break;

    case shareware:
      // Episode 2 and 3 are handled,
      // branching to an ad screen.
      EpiDef.menuitems = &EpisodeMenu[1];
      EpiDef.numitems = 3;
      break;

    case registered:
    case retail:
      if ((EpiDef.numitems != 0)		// Has the MAPINFO reader inhibited
       || (episode_menu_inhibited == 0))	// the episode menu?
      {
	// We are fine.
	// See if this Wad file contains any
	// extra episodes (e.g. DTWID-LE or VOE).
	episode = 0;
	menu_pos = 0;
	m_ptr = &EpisodeMenu[0];
	do
	{
	  //printf ("Checking episode %u\n", episode);
	  if (M_EpisodePresent (menu_lump_names[m_ptr -> namenum], episode))
	  {
	    episode_num [menu_pos++] = episode;

	    /* If the episode maps are in a pwad and the episode_names[] has */
	    /* been provided but not an M_EPIx lump then destroy the lump. */
	    if ((episode_names[episode] != NULL)
	     && ((lump1 = W_CheckNumForName (menu_lump_names[m_ptr -> namenum])) != -1)
	     && ((lump2 = G_MapLump (episode, 1)) != -1)
	     && (lumpinfo[lump1].handle == lumpinfo[0].handle)
	     && (lumpinfo[lump2].handle != lumpinfo[0].handle))
	    {
	      // printf ("Destroyed menu lump %u\n", pos);
	      menu_lump_names[m_ptr -> namenum][0] = 0xFF;
	    }
	    m_ptr++;
	  }
	  else
	  {
	    uintptr_t size;
	    menuitem_t * top;
	    top = &EpisodeMenu[(ARRAY_SIZE (EpisodeMenu))-1];
	    size = (unsigned char *) top - (unsigned char *) m_ptr;
	    if (size)
	      memcpy (m_ptr, m_ptr+1, size);
	  }
	} while (++episode < ARRAY_SIZE (EpisodeMenu));

	EpiDef.numitems = menu_pos;
      }
      M_SetEpisodeMenuPos ();
      break;

    default:
      break;
  }

  if ((show_discicon == 0)
   || (M_CheckParm ("-noshowdiscicon")))
  {
    stdiscicon = ML_NULL;
  }
  else if (M_CheckParm ("-cdrom"))
  {
    stdiscicon = ML_STCDROM;
  }
  else
  {
    stdiscicon = ML_STDISC;
  }
}

/* ----------------------------------------------------------------------- */
