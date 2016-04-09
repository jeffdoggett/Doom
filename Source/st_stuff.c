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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: st_stuff.c,v 1.6 1997/02/03 22:45:13 b1 Exp $";
#endif


#include "includes.h"

extern void P_Massacre (void);

/* ----------------------------------------------------------------- */

char * stat_bar_messages_orig [] =
{
  STSTR_MUS,
  STSTR_NOMUS,
  STSTR_DQDON,
  STSTR_DQDOFF,
  STSTR_KFAADDED,
  STSTR_FAADDED,
  STSTR_NCON,
  STSTR_NCOFF,
  STSTR_BEHOLD,
  STSTR_BEHOLDX,
  STSTR_CHOPPERS,
  STSTR_CLEV,
  NULL
};

char * stat_bar_messages [ARRAY_SIZE(stat_bar_messages_orig)];


typedef enum
{
  ST_STSTR_MUS,
  ST_STSTR_NOMUS,
  ST_STSTR_DQDON,
  ST_STSTR_DQDOFF,
  ST_STSTR_KFAADDED,
  ST_STSTR_FAADDED,
  ST_STSTR_NCON,
  ST_STSTR_NCOFF,
  ST_STSTR_BEHOLD,
  ST_STSTR_BEHOLDX,
  ST_STSTR_CHOPPERS,
  ST_STSTR_CLEV
} dhjobs_t;

/* ----------------------------------------------------------------- */

static const unsigned char STKEYS6 [] =
{
  0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00,
  0x33, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x4A, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00,
  0x60, 0x00, 0x00, 0x00, 0x01, 0x02, 0xC6, 0xC6, 0xC8, 0xC8, 0xFF, 0x00, 0x03, 0xC6, 0xC6, 0xCA,
  0xCB, 0xCB, 0xFF, 0x00, 0x06, 0xC6, 0xC6, 0x00, 0xCA, 0xCC, 0xC7, 0xC5, 0xC5, 0xFF, 0x00, 0x07,
  0xC6, 0xC6, 0xCA, 0xCA, 0xC6, 0xCF, 0xC7, 0xCB, 0xCB, 0xFF, 0x00, 0x07, 0xC8, 0xC8, 0x00, 0xC9,
  0xC7, 0xC8, 0xCF, 0xCA, 0xCA, 0xFF, 0x02, 0x05, 0xCA, 0xCA, 0xC7, 0xCF, 0xC7, 0xCA, 0xCA, 0xFF,
  0x03, 0x03, 0xCC, 0xCC, 0xC7, 0xC5, 0xC5, 0xFF
};

static const unsigned char STKEYS7 [] =
{
  0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00,
  0x33, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x4A, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00,
  0x60, 0x00, 0x00, 0x00, 0x01, 0x02, 0xA1, 0xA1, 0xA1, 0xA1, 0xFF, 0x00, 0x03, 0xA1, 0xA1, 0xA3,
  0xA3, 0xA3, 0xFF, 0x00, 0x06, 0xA1, 0xA1, 0x00, 0xA3, 0xDC, 0xA2, 0xEB, 0xEB, 0xFF, 0x00, 0x07,
  0xA1, 0xA1, 0xA3, 0xD9, 0xA2, 0xF6, 0xD6, 0xDD, 0xDD, 0xFF, 0x00, 0x07, 0xA1, 0xA1, 0x00, 0xD6,
  0xD5, 0xD6, 0xF6, 0xDA, 0xDA, 0xFF, 0x02, 0x05, 0xD9, 0xD9, 0xA2, 0xF6, 0xD6, 0xDD, 0xDD, 0xFF,
  0x03, 0x03, 0xDC, 0xDC, 0xA2, 0xEB, 0xEB, 0xFF
};

static const unsigned char STKEYS8 [] =
{
  0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00,
  0x33, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x4A, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00,
  0x60, 0x00, 0x00, 0x00, 0x01, 0x02, 0xAE, 0xAE, 0xB0, 0xB0, 0xFF, 0x00, 0x03, 0xAE, 0xAE, 0xB3,
  0xB3, 0xB3, 0xFF, 0x00, 0x06, 0xB0, 0xB0, 0x00, 0xB3, 0xB8, 0xAE, 0xAE, 0xAE, 0xFF, 0x00, 0x07,
  0xB0, 0xB0, 0xB3, 0xB9, 0xAF, 0xF6, 0xB9, 0xB6, 0xB6, 0xFF, 0x00, 0x07, 0xB0, 0xB0, 0x00, 0xB7,
  0xB1, 0xB0, 0xF6, 0xB3, 0xB3, 0xFF, 0x02, 0x05, 0xB9, 0xB9, 0xB1, 0xF6, 0xB9, 0xB6, 0xB6, 0xFF,
  0x03, 0x03, 0xB9, 0xB9, 0xAF, 0xAE, 0xAE, 0xFF
};

static const unsigned char * const keypatches [] =
{
  NULL,NULL,NULL,
  NULL,NULL,NULL,
  STKEYS6,STKEYS7,STKEYS8
};

/* ----------------------------------------------------------------- */

//
// STATUS BAR DATA
//


// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS		1
#define STARTBONUSPALS		9
#define NUMREDPALS		8
#define NUMBONUSPALS		4
// Radiation suit, green shift.
#define RADIATIONPAL		13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY	96

// For Responder
#define ST_TOGGLECHAT		KEY_ENTER

// Location of status bar
#define ST_X			0
#define ST_X2			104

#define ST_FX  			143
#define ST_FY  			169

// Number of status faces.
#define ST_NUMPAINFACES		5
#define ST_NUMSTRAIGHTFACES	3
#define ST_NUMTURNFACES		2
#define ST_NUMSPECIALFACES	3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)

#define ST_NUMEXTRAFACES	2

#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_TURNOFFSET		(ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET		(ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET	(ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET	(ST_EVILGRINOFFSET + 1)
#define ST_GODFACE		(ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE		(ST_GODFACE+1)

#define ST_FACESX		143
#define ST_FACESY		168

#define ST_EVILGRINCOUNT	(2*TICRATE)
#define ST_STRAIGHTFACECOUNT	(TICRATE/2)
#define ST_TURNCOUNT		(1*TICRATE)
#define ST_OUCHCOUNT		(1*TICRATE)
#define ST_RAMPAGEDELAY		(2*TICRATE)

#define ST_MUCHPAIN		20


// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH		3
#define ST_AMMOX		44
#define ST_AMMOY		171

// HEALTH number pos.
#define ST_HEALTHWIDTH		3
#define ST_HEALTHX		90
#define ST_HEALTHY		171

// Weapon pos.
#define ST_ARMSX		111
#define ST_ARMSY		172
#define ST_ARMSBGX		104
#define ST_ARMSBGY		168
#define ST_ARMSXSPACE		12
#define ST_ARMSYSPACE		10

// Frags pos.
#define ST_FRAGSX		138
#define ST_FRAGSY		171
#define ST_FRAGSWIDTH		2

// ARMOUR number pos.
#define ST_ARMOURWIDTH		3
#define ST_ARMOURX		221
#define ST_ARMOURY		171

// Key icon positions.
#define ST_KEY0WIDTH		8
#define ST_KEY0HEIGHT		5
#define ST_KEY0X		239
#define ST_KEY0Y		171
#define ST_KEY1WIDTH		ST_KEY0WIDTH
#define ST_KEY1X		239
#define ST_KEY1Y		181
#define ST_KEY2WIDTH		ST_KEY0WIDTH
#define ST_KEY2X		239
#define ST_KEY2Y		191

// Ammunition counter.
#define ST_AMMO0WIDTH		3
#define ST_AMMO0HEIGHT		6
#define ST_AMMO0X		288
#define ST_AMMO0Y		173
#define ST_AMMO1WIDTH		ST_AMMO0WIDTH
#define ST_AMMO1X		288
#define ST_AMMO1Y		179
#define ST_AMMO2WIDTH		ST_AMMO0WIDTH
#define ST_AMMO2X		288
#define ST_AMMO2Y		191
#define ST_AMMO3WIDTH		ST_AMMO0WIDTH
#define ST_AMMO3X		288
#define ST_AMMO3Y		185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH	3
#define ST_MAXAMMO0HEIGHT	5
#define ST_MAXAMMO0X		314
#define ST_MAXAMMO0Y		173
#define ST_MAXAMMO1WIDTH	ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X		314
#define ST_MAXAMMO1Y		179
#define ST_MAXAMMO2WIDTH	ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X		314
#define ST_MAXAMMO2Y		191
#define ST_MAXAMMO3WIDTH	ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X		314
#define ST_MAXAMMO3Y		185

// pistol
#define ST_WEAPON0X		110
#define ST_WEAPON0Y		172

// shotgun
#define ST_WEAPON1X		122
#define ST_WEAPON1Y		172

// chain gun
#define ST_WEAPON2X		134
#define ST_WEAPON2Y		172

// missile launcher
#define ST_WEAPON3X		110
#define ST_WEAPON3Y		181

// plasma gun
#define ST_WEAPON4X		122
#define ST_WEAPON4Y		181

 // bfg
#define ST_WEAPON5X		134
#define ST_WEAPON5Y		181

// WPNS title
#define ST_WPNSX		109
#define ST_WPNSY		191

 // DETH title
#define ST_DETHX		109
#define ST_DETHY		191

//Incoming messages window location
//UNUSED
// #define ST_MSGTEXTX	   (viewwindowx)
// #define ST_MSGTEXTY	   (viewwindowy+viewheight-18)
#define ST_MSGTEXTX		0
#define ST_MSGTEXTY		0
// Dimensions given in characters.
#define ST_MSGWIDTH		52
// Or shall I say, in lines?
#define ST_MSGHEIGHT		1

#define ST_OUTTEXTX		0
#define ST_OUTTEXTY		6

// Width, in characters again.
#define ST_OUTWIDTH		52
 // Height, in lines.
#define ST_OUTHEIGHT		1

#define ST_MAPTITLEY		0
#define ST_MAPHEIGHT		1

#define DX(a)			(SCREENWIDTH/2)-((160*sbarscale)>>FRACBITS)+((sbarscale*a)>>FRACBITS)
#define DY(a)			(SCREENHEIGHT-(((200-a)*sbarscale)>>FRACBITS))
#define DW(a)			((sbarscale*a)>>FRACBITS)

// main player in game
static player_t*	plyr = NULL;

// ST_Start() has just been called
boolean		st_firsttime;

// lump number for PLAYPAL
static int		lu_palette;

// used for timing
static unsigned int	st_clock;

// used for making messages go away
static int		st_msgcounter=0;

// used when in chat
static st_chatstateenum_t	st_chatstate;

// whether in automap or first-person
static st_stateenum_t	st_gamestate;

// whether left-side main status bar is active
static boolean		st_statusbaron;

// whether status bar chat is active
static boolean		st_chat;

// value of st_chat before message popped up
static boolean		st_oldchat;

// whether chat window has the cursor on
static boolean		st_cursoron;

// !deathmatch
static boolean		st_notdeathmatch;

// !deathmatch && st_statusbaron
static boolean		st_armson;

// !deathmatch
static boolean		st_fragson;

// main bar left
static patch_t*		sbar;

// 0-9, tall numbers
static patch_t*		tallnum[10];

// tall % sign
static patch_t*		tallpercent;

// 0-9, short, yellow (,different!) numbers
static patch_t*		shortnum[10];

// 3 key-cards, 3 skulls, 3 card/skull combos
// jff 2/24/98 extend number of patches by three skull/card combos
static unsigned char	keyinwad [NUMCARDS+3];
static patch_t*		keys[NUMCARDS+3];

// face status patches
static patch_t*		faces[ST_NUMFACES];

// face background
static patch_t*		faceback;

 // main bar right
static patch_t*		armsbg;

// weapon ownership patches
static patch_t*		arms[6][2];

// ready-weapon widget
static st_number_t	w_ready;

 // in deathmatch only, summary of frags stats
static st_number_t	w_frags;

// health widget
static st_percent_t	w_health;

// arms background
static st_binicon_t	w_armsbg;


// weapon ownership widgets
static st_multicon_t	w_arms[6];

// face status widget
static st_multicon_t	w_faces;

// keycard widgets
static st_multicon_t	w_keyboxes[3];

// armour widget
static st_percent_t	w_armour;

// ammo widgets
static st_number_t	w_ammo[4];

// max ammo widgets
static st_number_t	w_maxammo[4];



 // number of frags so far in deathmatch
static int	st_fragscount;

// used to use appopriately pained face
static int	st_oldhealth = -1;

// used for evil grin
static boolean	oldweaponsowned[NUMWEAPONS];

 // count until face changes
static int	st_facecount = 0;

// current face index, used by w_faces
static int	st_faceindex = 0;

// holds key-type for each key box on bar
static int	keyboxes[3];

// a random number per tick
static int	st_randomnumber;

int		weaponscale;
int		stbar_scale = 1;
int		hutext_scale = 1;

fixed_t		sbarscale = 1 << FRACBITS;
fixed_t		hutextscale = 1 << FRACBITS;

unsigned int God_Mode_Health = 100;
unsigned int IDFA_Armour = 200;
unsigned int IDKFA_Armour = 200;
armour_class_t IDFA_Armour_Class = BLUEARMOUR;
armour_class_t IDKFA_Armour_Class = BLUEARMOUR;

// Massive bunches of cheat shit
//  to keep it from being easy to figure them out.
// Yeah, right...
unsigned char	cheat_mus_seq[] =
{
    0xb2, 0x26, 0xb6, 0xae, 0xea, 1, 0, 0, 0xff
};

unsigned char	cheat_choppers_seq[] =
{
    0xb2, 0x26, 0xe2, 0x32, 0xf6, 0x2a, 0x2a, 0xa6, 0x6a, 0xea, 0xff // id...
};

unsigned char	cheat_god_seq[] =
{
    0xb2, 0x26, 0x26, 0xaa, 0x26, 0xff  // iddqd
};

unsigned char	cheat_ammo_seq[] =
{
    0xb2, 0x26, 0xf2, 0x66, 0xa2, 0xff	// idkfa
};

unsigned char	cheat_keys_seq[] =
{
    0xb2, 0x26, 0xf2, 0xa2, 1, 0, 0xff	// idka
};

unsigned char	cheat_ammonokey_seq[] =
{
    0xb2, 0x26, 0x66, 0xa2, 0xff	// idfa
};


// Smashing Pumpkins Into Samml Piles Of Putried Debris.
unsigned char	cheat_noclip_seq[] =
{
    0xb2, 0x26, 0xea, 0x2a, 0xb2,	// idspispopd
    0xea, 0x2a, 0xf6, 0x2a, 0x26, 0xff
};

//
unsigned char	cheat_commercial_noclip_seq[] =
{
    0xb2, 0x26, 0xe2, 0x36, 0xb2, 0x2a, 0xff	// idclip
};



unsigned char	cheat_powerup_seq[7][10] =
{
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6e, 0xff }, 	// beholdv
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xea, 0xff }, 	// beholds
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xb2, 0xff }, 	// beholdi
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x6a, 0xff }, 	// beholdr
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xa2, 0xff }, 	// beholda
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0x36, 0xff }, 	// beholdl
    { 0xb2, 0x26, 0x62, 0xa6, 0x32, 0xf6, 0x36, 0x26, 0xff }		// behold
};


unsigned char	cheat_clev_seq[] =
{
    0xb2, 0x26,  0xe2, 0x36, 0xa6, 0x6e, 1, 0, 0, 0xff	// idclev
};


// my position cheat
unsigned char	cheat_mypos_seq[] =
{
    0xb2, 0x26, 0xb6, 0xba, 0x2a, 0xf6, 0xea, 0xff	// idmypos
};

unsigned char	cheat_massacre_seq[] =
{
    0xb2, 0x26, 0xf2, 0xb2, 0x36, 0x36, 0xff		// idkill
};

// Now what?
cheatseq_t	cheat_mus = { cheat_mus_seq, 0 };
cheatseq_t	cheat_god = { cheat_god_seq, 0 };
cheatseq_t	cheat_ammo = { cheat_ammo_seq, 0 };
cheatseq_t	cheat_ammonokey = { cheat_ammonokey_seq, 0 };
cheatseq_t	cheat_keys = { cheat_keys_seq, 0 };
cheatseq_t	cheat_noclip = { cheat_noclip_seq, 0 };
cheatseq_t	cheat_commercial_noclip = { cheat_commercial_noclip_seq, 0 };

cheatseq_t	cheat_powerup[7] =
{
    { cheat_powerup_seq[0], 0 },
    { cheat_powerup_seq[1], 0 },
    { cheat_powerup_seq[2], 0 },
    { cheat_powerup_seq[3], 0 },
    { cheat_powerup_seq[4], 0 },
    { cheat_powerup_seq[5], 0 },
    { cheat_powerup_seq[6], 0 }
};

cheatseq_t	cheat_choppers = { cheat_choppers_seq, 0 };
cheatseq_t	cheat_clev = { cheat_clev_seq, 0 };
cheatseq_t	cheat_mypos = { cheat_mypos_seq, 0 };
cheatseq_t	cheat_massacre = { cheat_massacre_seq, 0 };


//
// STATUS BAR CODE
//
void ST_Stop(void);

void ST_refreshBackground(void)
{
  int x;
  if (st_statusbaron)
  {
    R_ClearSbarSides ();

    x = DX(ST_X);
    STlib_drawPatch (x, 0, ST_BG, sbar);

    if (netgame)
      STlib_drawPatch (DX(ST_FX), 0, ST_BG, faceback);

    V_CopyRect (x, 0, ST_BG, ST_WIDTH, ST_HEIGHT, x, ST_Y, ST_FG);
  }
}

/* ---------------------------------------------------------------------------- */

// Respond to keyboard input events,
//  intercept cheats.
boolean
ST_Responder (event_t* ev)
{
  int		i;

  // if a user keypress...
  if (ev->type == ev_keydown)
  {
    if (!netgame)
    {
      // b. - enabled for more debug fun.
      // if (gameskill != sk_nightmare) {

      // 'dqd' cheat for toggleable god mode
      if (cht_CheckCheat(&cheat_god, ev->data1))
      {
	plyr->cheats ^= CF_GODMODE;
	if (plyr->cheats & CF_GODMODE)
	{
	  if ((plyr->mo)
	   && (plyr->mo->health < God_Mode_Health))
	      plyr->mo->health = God_Mode_Health;
          if (plyr->health < God_Mode_Health)
	    plyr->health = God_Mode_Health;
	  plyr->message = stat_bar_messages [ST_STSTR_DQDON];
	}
	else
	  plyr->message = stat_bar_messages [ST_STSTR_DQDOFF];
      }
      // 'fa' cheat for killer fucking arsenal
      else if (cht_CheckCheat(&cheat_ammonokey, ev->data1))
      {
        if ((!netgame)
	 && (M_CheckParm ("-idfabackpack")))
	{
	  if (!plyr->backpack)
	  {
	    for (i=0 ; i<NUMAMMO ; i++)
		plyr->maxammo[i] *= 2;
	    plyr->backpack = true;
	  }
	}

        if (plyr->armourpoints < IDFA_Armour)
	  plyr->armourpoints = IDFA_Armour;

	if (plyr->armourtype < IDFA_Armour_Class)
	  plyr->armourtype = IDFA_Armour_Class;

	for (i=0;i<NUMWEAPONS;i++)
	  plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
	  plyr->ammo[i] = plyr->maxammo[i];

	plyr->message = stat_bar_messages [ST_STSTR_FAADDED];
      }
      // 'kfa' cheat for key full ammo
      else if (cht_CheckCheat(&cheat_ammo, ev->data1))
      {
        if ((!netgame)
	 && (M_CheckParm ("-idkfabackpack")))
	{
	  if (!plyr->backpack)
	  {
	    for (i=0 ; i<NUMAMMO ; i++)
		plyr->maxammo[i] *= 2;
	    plyr->backpack = true;
	  }
	}

        if (plyr->armourpoints < IDKFA_Armour)
	  plyr->armourpoints = IDKFA_Armour;

	if (plyr->armourtype < IDKFA_Armour_Class)
	  plyr->armourtype = IDKFA_Armour_Class;

	for (i=0;i<NUMWEAPONS;i++)
	  plyr->weaponowned[i] = true;

	for (i=0;i<NUMAMMO;i++)
	  plyr->ammo[i] = plyr->maxammo[i];

	for (i=0;i<NUMCARDS;i++)
	  plyr->cards[i] = true;

	plyr->message = stat_bar_messages [ST_STSTR_KFAADDED];
      }
      else if (cht_CheckCheat(&cheat_keys, ev->data1))
      {
	char	buf[3];
	int		keynum;
	cht_GetParam(&cheat_keys, buf);
	keynum = buf [0];
	if (keynum >= '1')
	{
	  keynum -= '1';
          if (keynum < 6)
	  {
	    char * msg;
	    msg = P_GiveCard (plyr, (card_t) keynum);
	    if (msg) plyr->message = msg;
	  }
	}
      }
      // 'mus' cheat for changing music
      else if (cht_CheckCheat(&cheat_mus, ev->data1))
      {

	char	buf[3];
	int		musnum;

	plyr->message = stat_bar_messages [ST_STSTR_MUS];
	cht_GetParam(&cheat_mus, buf);

	if (gamemode == commercial)
	{
	  musnum = mus_runnin + (buf[0]-'0')*10 + buf[1]-'0' - 1;

	  if (((buf[0]-'0')*10 + buf[1]-'0') > 35)
	  {
	    S_StopMusic ();
	    plyr->message = stat_bar_messages [ST_STSTR_NOMUS];
	  }
	  else
	  {
	    S_ChangeMusic(musnum, 1);
	  }
	}
	else
	{
	  musnum = mus_e1m1 + (buf[0]-'1')*9 + (buf[1]-'1');

	  if (((buf[0]-'1')*9 + buf[1]-'1') > 31)
	  {
	    S_StopMusic ();
	    plyr->message = stat_bar_messages [ST_STSTR_NOMUS];
	  }
	  else
	  {
	    S_ChangeMusic(musnum, 1);
	  }
	}
      }
      // Simplified, accepting both "noclip" and "idspispopd".
      // no clipping mode cheat
      else if ( cht_CheckCheat(&cheat_noclip, ev->data1)
		|| cht_CheckCheat(&cheat_commercial_noclip,ev->data1) )
      {
	plyr->cheats ^= CF_NOCLIP;

	if (plyr->cheats & CF_NOCLIP)
	  plyr->message = stat_bar_messages [ST_STSTR_NCON];
	else
	  plyr->message = stat_bar_messages [ST_STSTR_NCOFF];
      }
      else if (cht_CheckCheat(&cheat_massacre, ev->data1))
      {
	P_Massacre ();
      }
      // 'behold?' power-up cheats
      for (i=0;i<6;i++)
      {
	if (cht_CheckCheat(&cheat_powerup[i], ev->data1))
	{
	  if (!plyr->powers[i])
	    P_GivePower( plyr, i);
	  else if (i!=pw_strength)
	    plyr->powers[i] = 1;
	  else
	    plyr->powers[i] = 0;

	  plyr->message = stat_bar_messages [ST_STSTR_BEHOLDX];
	}
      }

      // 'behold' power-up menu
      if (cht_CheckCheat(&cheat_powerup[6], ev->data1))
      {
	plyr->message = stat_bar_messages [ST_STSTR_BEHOLD];
      }
      // 'choppers' invulnerability & chainsaw
      else if (cht_CheckCheat(&cheat_choppers, ev->data1))
      {
	plyr->weaponowned[wp_chainsaw] = true;
	// plyr->powers[pw_invulnerability] = true;
	plyr->message = stat_bar_messages [ST_STSTR_CHOPPERS];
      }
      // 'mypos' for player position
      else if (cht_CheckCheat(&cheat_mypos, ev->data1))
      {
	plyr->message = HU_printf ("ang=0x%x;x,y=%d,%d",
				players[consoleplayer].mo->angle,
				players[consoleplayer].mo->x / FRACUNIT,
				players[consoleplayer].mo->y / FRACUNIT);
      }
    }

    // 'clev' change-level cheat
    if (cht_CheckCheat(&cheat_clev, ev->data1))
    {
      char		buf[8];
      int		epsd;
      int		map;
      map_dests_t *	map_info_p;

      cht_GetParam(&cheat_clev, buf);

      switch (buf[0])
      {
        case 'e':
          switch (buf[1])
          {
            case 'e':
	      G_ExitLevel ();
	      break;

	    case 's':
	      G_SecretExitLevel ();
	      break;
	  }
          return (false);

        case 'n':
          switch (buf[1])
          {
            case 's':
	      map_info_p = G_Access_MapInfoTab (gameepisode, gamemap);
	      epsd = map_info_p -> normal_exit_to_episode;
	      map  = map_info_p -> normal_exit_to_map;
              break;

            default:
              epsd = gameepisode;
              map = gamemap;
              do
              {
		if (gamemode == commercial)
		{
		  map++;
		  if (map > 99)
		    break;
		}
		else
		{
		  map++;
		  if (map > 9)
		  {
		    map = 1;
		    epsd++;
		    if (epsd > 9)
		      break;
		  }
		}
              } while (G_MapLump (epsd,map) == -1);
	  }
          break;

	case 's':
	  map_info_p = G_Access_MapInfoTab (gameepisode, gamemap);
	  epsd = map_info_p -> secret_exit_to_episode;
	  map  = map_info_p -> secret_exit_to_map;
	  break;

	default:
	  if (gamemode == commercial)
	  {
	    epsd = 0;
	    map = (buf[0] - '0')*10 + buf[1] - '0';
	  }
	  else
	  {
	    epsd = buf[0] - '0';
	    map = buf[1] - '0';
	  }
      }

      // Catch invalid maps.

      if (G_MapLump (epsd,map) == -1)
	return false;

      // So be it.
      plyr->message = stat_bar_messages [ST_STSTR_CLEV];
      G_DeferedInitNewLater(gameskill, epsd, map);
    }
  }
  return false;
}

/* ---------------------------------------------------------------------------- */

void ST_AutoMapEvent (int am_message)
{
  switch (am_message)
  {
    case AM_MSGENTERED:
      st_gamestate = AutomapState;
      st_firsttime = true;
      break;

    case AM_MSGEXITED:
      //	fprintf(stderr, "AM exited\n");
      st_gamestate = FirstPersonState;
      break;
  }
}

/* ---------------------------------------------------------------------------- */

int ST_calcPainOffset(void)
{
    int		health;
    static int	lastcalc;
    static int	oldhealth = -1;

    health = plyr->health > 100 ? 100 : plyr->health;

    if (health != oldhealth)
    {
	lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
	oldhealth = health;
    }
    return lastcalc;
}

/* ---------------------------------------------------------------------------- */
//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
static void ST_updateFaceWidget(void)
{
    int		i;
    angle_t	badguyangle;
    angle_t	diffang;
    static int	lastattackdown = -1;
    static int	priority = 0;
    boolean	doevilgrin;

    if (priority < 10)
    {
	// dead
	if (!plyr->health)
	{
	    priority = 9;
	    st_faceindex = ST_DEADFACE;
	    st_facecount = 1;
	}
    }

    if (priority < 9)
    {
	if (plyr->bonuscount)
	{
	    // picking up bonus
	    doevilgrin = false;

	    for (i=0;i<NUMWEAPONS;i++)
	    {
		if (oldweaponsowned[i] != plyr->weaponowned[i])
		{
		    doevilgrin = true;
		    oldweaponsowned[i] = plyr->weaponowned[i];
		}
	    }
	    if (doevilgrin)
	    {
		// evil grin if just picked up weapon
		priority = 8;
		st_facecount = ST_EVILGRINCOUNT;
		st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
	    }
	}

    }

    if (priority < 8)
    {
	if (plyr->damagecount
	    && plyr->attacker
	    && plyr->attacker != plyr->mo)
	{
	    // being attacked
	    priority = 7;

	    // if (plyr->health - st_oldhealth > ST_MUCHPAIN) /* Correct the "Ouch face" bug.
	    if (st_oldhealth - plyr->health > ST_MUCHPAIN)
	    {
		st_facecount = ST_TURNCOUNT;
		st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
	    }
	    else
	    {
		badguyangle = R_PointToAngle2(plyr->mo->x,
					      plyr->mo->y,
					      plyr->attacker->x,
					      plyr->attacker->y);

		if (badguyangle > plyr->mo->angle)
		{
		    // whether right or left
		    diffang = badguyangle - plyr->mo->angle;
		    i = diffang > ANG180;
		}
		else
		{
		    // whether left or right
		    diffang = plyr->mo->angle - badguyangle;
		    i = diffang <= ANG180;
		} // confusing, aint it?


		st_facecount = ST_TURNCOUNT;
		st_faceindex = ST_calcPainOffset();

		if (diffang < ANG45)
		{
		    // head-on
		    st_faceindex += ST_RAMPAGEOFFSET;
		}
		else if (i)
		{
		    // turn face right
		    st_faceindex += ST_TURNOFFSET;
		}
		else
		{
		    // turn face left
		    st_faceindex += ST_TURNOFFSET+1;
		}
	    }
	}
    }

    if (priority < 7)
    {
	// getting hurt because of your own damn stupidity
	if (plyr->damagecount)
	{
	    // if (plyr->health - st_oldhealth > ST_MUCHPAIN) /* Correct the "Ouch face" bug.
	    if (st_oldhealth - plyr->health > ST_MUCHPAIN)
	    {
		priority = 7;
		st_facecount = ST_TURNCOUNT;
		st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
	    }
	    else
	    {
		priority = 6;
		st_facecount = ST_TURNCOUNT;
		st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
	    }

	}

    }

    if (priority < 6)
    {
	// rapid firing
	if (plyr->attackdown)
	{
	    if (lastattackdown==-1)
		lastattackdown = ST_RAMPAGEDELAY;
	    else if (!--lastattackdown)
	    {
		priority = 5;
		st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
		st_facecount = 1;
		lastattackdown = 1;
	    }
	}
	else
	    lastattackdown = -1;

    }

    if (priority < 5)
    {
	// invulnerability
	if ((plyr->cheats & CF_GODMODE)
	    || plyr->powers[pw_invulnerability])
	{
	    priority = 4;

	    st_faceindex = ST_GODFACE;
	    st_facecount = 1;

	}

    }

    // look left or look right if the facecount has timed out
    if (!st_facecount)
    {
	st_faceindex = ST_calcPainOffset() + (st_randomnumber % 3);
	st_facecount = ST_STRAIGHTFACECOUNT;
	priority = 0;
    }

    st_facecount--;

}

/* ---------------------------------------------------------------------------- */

static void ST_updateWidgets(void)
{
    static int	largeammo = 1994; // means "n/a"
    int		i;

    // must redirect the pointer if the ready weapon has changed.
    //  if (w_ready.data != plyr->readyweapon)
    //  {
    if (weaponinfo[plyr->readyweapon].ammo == am_noammo)
	w_ready.num = &largeammo;
    else
	w_ready.num = &plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
    //{
    // static int tic=0;
    // static int dir=-1;
    // if (!(tic&15))
    //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo]+=dir;
    // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    // tic++;
    // }
    w_ready.data = plyr->readyweapon;

    // if (*w_ready.on)
    //  STlib_updateNum(&w_ready, true);
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    for (i=0;i<3;i++)
    {
      int k;

      k = 0;
      if (plyr->cards[i]) k |= 1;
      if (plyr->cards[i+3]) k |= 2;
      if (k == 0)
	k = -1;
      else
	k = ((k-1) * 3) + i;
      keyboxes[i] = k;
    }

    // refresh everything if this is him coming back to life
    ST_updateFaceWidget();

    // used by the w_armsbg widget
    st_notdeathmatch = (boolean) (!deathmatch);

    // used by w_arms[] widgets
    st_armson = (boolean) (st_statusbaron && !deathmatch);

    // used by w_frags widget
    st_fragson = (boolean) (deathmatch && st_statusbaron);
    st_fragscount = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (i != consoleplayer)
	    st_fragscount += plyr->frags[i];
	else
	    st_fragscount -= plyr->frags[i];
    }

    // get rid of chat window if up because of message
    if (!--st_msgcounter)
	st_chat = st_oldchat;

}

/* ---------------------------------------------------------------------------- */

void ST_Ticker (void)
{

    st_clock++;
    st_randomnumber = M_Random();
    ST_updateWidgets();
    st_oldhealth = plyr->health;

}

/* ---------------------------------------------------------------------------- */

static int st_palette = 0;

void ST_doPaletteStuff(void)
{

    int		palette;
    byte*	pal;
    int		cnt;
    int		bzc;

    cnt = plyr->damagecount;

    if (plyr->powers[pw_strength])
    {
	// slowly fade the berzerk out
  	bzc = 12 - (plyr->powers[pw_strength]>>6);

	if (bzc > cnt)
	    cnt = bzc;
    }

    if (cnt)
    {
	palette = (cnt+7)>>3;

	if (palette >= NUMREDPALS)
	    palette = NUMREDPALS-1;

	palette += STARTREDPALS;
    }

    else if (plyr->bonuscount)
    {
	palette = (plyr->bonuscount+7)>>3;

	if (palette >= NUMBONUSPALS)
	    palette = NUMBONUSPALS-1;

	palette += STARTBONUSPALS;
    }

    else if ( plyr->powers[pw_ironfeet] > 4*32
	      || plyr->powers[pw_ironfeet]&8)
	palette = RADIATIONPAL;
    else
	palette = 0;

    if (palette != st_palette)
    {
	st_palette = palette;
	pal = (byte *) W_CacheLumpNum (lu_palette, PU_CACHE)+palette*768;
	I_SetPalette (pal);
    }

}

/* ---------------------------------------------------------------------------- */

void ST_drawWidgets(boolean refresh)
{
    int		i;

    // used by w_arms[] widgets
    st_armson = (boolean) (st_statusbaron && !deathmatch);

    // used by w_frags widget
    st_fragson = (boolean) (deathmatch && st_statusbaron);

    STlib_updateNum(&w_ready, refresh);

    for (i=0;i<4;i++)
    {
	STlib_updateNum(&w_ammo[i], refresh);
	STlib_updateNum(&w_maxammo[i], refresh);
    }

    STlib_updatePercent(&w_health, refresh);
    STlib_updatePercent(&w_armour, refresh);

    STlib_updateBinIcon(&w_armsbg, refresh);

    for (i=0;i<6;i++)
	STlib_updateMultIcon(&w_arms[i], refresh);

    STlib_updateMultIcon(&w_faces, refresh);

    for (i=0;i<3;i++)
	STlib_updateMultIcon(&w_keyboxes[i], refresh);

    STlib_updateNum(&w_frags, refresh);

}

/* ---------------------------------------------------------------------------- */

void ST_doRefresh(void)
{

    st_firsttime = false;

    // draw status bar background to off-screen buff
    ST_refreshBackground();

    // and refresh all widgets
    ST_drawWidgets(true);

}

/* ---------------------------------------------------------------------------- */

void ST_diffDraw(void)
{
    // update all widgets
    ST_drawWidgets(false);
}

/* ---------------------------------------------------------------------------- */

void ST_Drawer (boolean fullscreen, boolean refresh)
{

    st_statusbaron = (boolean) ((!fullscreen) || automapactive);
    st_firsttime = (boolean) (st_firsttime || refresh);

    // Do red-/gold-shifts from damage/items
    ST_doPaletteStuff();

    // If just after ST_Start(), refresh all
    if (st_firsttime) ST_doRefresh();
    // Otherwise, update as little as possible
    else ST_diffDraw();

}

/* ---------------------------------------------------------------------------- */

void ST_loadGraphics(void)
{

    int		i;
    int		j;
    int		lump;
    int		pwad;
    int		facenum;

    char	namebuf[9];

    // Load the numbers, tall and short
    for (i=0;i<10;i++)
    {
	sprintf(namebuf, "STTNUM%d", i);
	tallnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

	sprintf(namebuf, "STYSNUM%d", i);
	shortnum[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
    }

    // Load percent key.
    //Note: why not load STMINUS here, too?
    tallpercent = (patch_t *) W_CacheLumpName("STTPRCNT", PU_STATIC);

    // key cards
    pwad = 0;
    for (i=0;i<ARRAY_SIZE(keys);i++)
    {
	sprintf(namebuf, "STKEYS%d", i);
	lump = W_CheckNumForName (namebuf);
	if (lump != -1)
	{
	  keyinwad[i] = 1;
	  if (lumpinfo[lump].handle != lumpinfo[0].handle)
	    pwad = 1;
	  keys[i] = (patch_t *) W_CacheLumpNum (lump, PU_STATIC);
	}
	else if (pwad == 0)	// Do not use internal key graphics
	{			// if other graphics loaded from a pwad.
	  keyinwad[i] = 0;
	  keys[i] = (patch_t*) keypatches [i];
	}
	else
	{
	  keyinwad[i] = 0;
	  keys[i] = keys[i-3];
	}
//	keys[i] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);
    }

    // arms background
    armsbg = (patch_t *) W_CacheLumpName("STARMS", PU_STATIC);

    // arms ownership widgets
    for (i=0;i<6;i++)
    {
	sprintf(namebuf, "STGNUM%d", i+2);

	// gray #
	arms[i][0] = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

	// yellow #
	arms[i][1] = shortnum[i+2];
    }

    // face backgrounds for different colour players
    sprintf(namebuf, "STFB%d", consoleplayer);
    faceback = (patch_t *) W_CacheLumpName(namebuf, PU_STATIC);

    // status bar background bits
    sbar = (patch_t *) W_CacheLumpName("STBAR", PU_STATIC);

    // face states
    facenum = 0;
    for (i=0;i<ST_NUMPAINFACES;i++)
    {
	for (j=0;j<ST_NUMSTRAIGHTFACES;j++)
	{
	    sprintf(namebuf, "STFST%d%d", i, j);
	    faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	}
	sprintf(namebuf, "STFTR%d0", i);	// turn right
	faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	sprintf(namebuf, "STFTL%d0", i);	// turn left
	faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	sprintf(namebuf, "STFOUCH%d", i);	// ouch!
	faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	sprintf(namebuf, "STFEVL%d", i);	// evil grin ;)
	faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
	sprintf(namebuf, "STFKILL%d", i);	// pissed off
	faces[facenum++] = W_CacheLumpName(namebuf, PU_STATIC);
    }
    faces[facenum++] = W_CacheLumpName("STFGOD0", PU_STATIC);
    faces[facenum++] = W_CacheLumpName("STFDEAD0", PU_STATIC);

}

/* ---------------------------------------------------------------------------- */

void ST_loadData(void)
{
    lu_palette = W_GetNumForName ("PLAYPAL");
    ST_loadGraphics();
    AM_LoadColours (lu_palette);
}

/* ---------------------------------------------------------------------------- */

void ST_unloadGraphics(void)
{

    int i;

    // unload the numbers, tall and short
    for (i=0;i<10;i++)
    {
	Z_ChangeTag(tallnum[i], PU_CACHE);
	Z_ChangeTag(shortnum[i], PU_CACHE);
    }
    // unload tall percent
    Z_ChangeTag(tallpercent, PU_CACHE);

    // unload arms background
    Z_ChangeTag(armsbg, PU_CACHE);

    // unload gray #'s
    for (i=0;i<6;i++)
	Z_ChangeTag(arms[i][0], PU_CACHE);

    // unload the key cards
    for (i=0;i<ARRAY_SIZE(keys);i++)
    {
      if (keyinwad[i])
	Z_ChangeTag(keys[i], PU_CACHE);
    }

    Z_ChangeTag(sbar, PU_CACHE);
    Z_ChangeTag(faceback, PU_CACHE);

    for (i=0;i<ST_NUMFACES;i++)
	Z_ChangeTag(faces[i], PU_CACHE);

    // Note: nobody ain't seen no unloading
    //   of stminus yet. Dude.


}

/* ---------------------------------------------------------------------------- */

void ST_unloadData(void)
{
    ST_unloadGraphics();
}

/* ---------------------------------------------------------------------------- */

static void ST_initData(void)
{

    int		i;

    st_firsttime = true;
    plyr = &players[consoleplayer];

    st_clock = 0;
    st_chatstate = StartChatState;
    st_gamestate = FirstPersonState;

    st_statusbaron = true;
    st_oldchat = st_chat = false;
    st_cursoron = false;

    st_faceindex = 0;
    st_palette = -1;

    st_oldhealth = -1;

    for (i=0;i<NUMWEAPONS;i++)
	oldweaponsowned[i] = plyr->weaponowned[i];

    for (i=0;i<3;i++)
	keyboxes[i] = -1;

    STlib_init();

}

/* ---------------------------------------------------------------------------- */

void ST_createWidgets(void)
{
  int i;

  if (plyr)
  {
    st_firsttime = true;

    // ready weapon ammo
    STlib_initNum(&w_ready,
		  DX(ST_AMMOX),
		  DY(ST_AMMOY),
		  tallnum,
		  &plyr->ammo[weaponinfo[plyr->readyweapon].ammo],
		  &st_statusbaron,
		  ST_AMMOWIDTH);

    // the last weapon type
    w_ready.data = plyr->readyweapon;

    // health percentage
    STlib_initPercent(&w_health,
		      DX(ST_HEALTHX),
		      DY(ST_HEALTHY),
		      tallnum,
		      &plyr->health,
		      &st_statusbaron,
		      tallpercent);

    // arms background
    STlib_initBinIcon(&w_armsbg,
		      DX(ST_ARMSBGX),
		      DY(ST_ARMSBGY),
		      armsbg,
		      &st_notdeathmatch,
		      &st_statusbaron);

    // weapons owned
    for(i=0;i<6;i++)
    {
	STlib_initMultIcon(&w_arms[i],
			   DX(ST_ARMSX)+((i%3)*DW(ST_ARMSXSPACE)),
			   DY(ST_ARMSY)+((i/3)*DW(ST_ARMSYSPACE)),
			   arms[i], (int *) &plyr->weaponowned[i+1],
			   &st_armson);
    }

    // frags sum
    STlib_initNum(&w_frags,
		  DX(ST_FRAGSX),
		  DY(ST_FRAGSY),
		  tallnum,
		  &st_fragscount,
		  &st_fragson,
		  ST_FRAGSWIDTH);

    // faces
    STlib_initMultIcon(&w_faces,
		       DX(ST_FACESX),
		       DY(ST_FACESY),
		       faces,
		       &st_faceindex,
		       &st_statusbaron);

    // armour percentage - should be coloured later
    STlib_initPercent(&w_armour,
		      DX(ST_ARMOURX),
		      DY(ST_ARMOURY),
		      tallnum,
		      &plyr->armourpoints,
		      &st_statusbaron, tallpercent);

    // keyboxes 0-2
    STlib_initMultIcon(&w_keyboxes[0],
		       DX(ST_KEY0X),
		       DY(ST_KEY0Y),
		       keys,
		       &keyboxes[0],
		       &st_statusbaron);

    STlib_initMultIcon(&w_keyboxes[1],
		       DX(ST_KEY1X),
		       DY(ST_KEY1Y),
		       keys,
		       &keyboxes[1],
		       &st_statusbaron);

    STlib_initMultIcon(&w_keyboxes[2],
		       DX(ST_KEY2X),
		       DY(ST_KEY2Y),
		       keys,
		       &keyboxes[2],
		       &st_statusbaron);

    // ammo count (all four kinds)
    STlib_initNum(&w_ammo[0],
		  DX(ST_AMMO0X),
		  DY(ST_AMMO0Y),
		  shortnum,
		  &plyr->ammo[0],
		  &st_statusbaron,
		  ST_AMMO0WIDTH);

    STlib_initNum(&w_ammo[1],
		  DX(ST_AMMO1X),
		  DY(ST_AMMO1Y),
		  shortnum,
		  &plyr->ammo[1],
		  &st_statusbaron,
		  ST_AMMO1WIDTH);

    STlib_initNum(&w_ammo[2],
		  DX(ST_AMMO2X),
		  DY(ST_AMMO2Y),
		  shortnum,
		  &plyr->ammo[2],
		  &st_statusbaron,
		  ST_AMMO2WIDTH);

    STlib_initNum(&w_ammo[3],
		  DX(ST_AMMO3X),
		  DY(ST_AMMO3Y),
		  shortnum,
		  &plyr->ammo[3],
		  &st_statusbaron,
		  ST_AMMO3WIDTH);

    // max ammo count (all four kinds)
    STlib_initNum(&w_maxammo[0],
		  DX(ST_MAXAMMO0X),
		  DY(ST_MAXAMMO0Y),
		  shortnum,
		  &plyr->maxammo[0],
		  &st_statusbaron,
		  ST_MAXAMMO0WIDTH);

    STlib_initNum(&w_maxammo[1],
		  DX(ST_MAXAMMO1X),
		  DY(ST_MAXAMMO1Y),
		  shortnum,
		  &plyr->maxammo[1],
		  &st_statusbaron,
		  ST_MAXAMMO1WIDTH);

    STlib_initNum(&w_maxammo[2],
		  DX(ST_MAXAMMO2X),
		  DY(ST_MAXAMMO2Y),
		  shortnum,
		  &plyr->maxammo[2],
		  &st_statusbaron,
		  ST_MAXAMMO2WIDTH);

    STlib_initNum(&w_maxammo[3],
		  DX(ST_MAXAMMO3X),
		  DY(ST_MAXAMMO3Y),
		  shortnum,
		  &plyr->maxammo[3],
		  &st_statusbaron,
		  ST_MAXAMMO3WIDTH);
  }
}

/* ---------------------------------------------------------------------------- */

static boolean	st_stopped = true;


void ST_Start (void)
{

    if (!st_stopped)
	ST_Stop();

    ST_initData();
    ST_createWidgets();
    st_stopped = false;

}

/* ---------------------------------------------------------------------------- */

void ST_Stop (void)
{
    if (st_stopped)
	return;

    I_SetPalette (W_CacheLumpNum (lu_palette, PU_CACHE));

    st_stopped = true;
}

/* ---------------------------------------------------------------------------- */

void ST_Init (void)
{
    sbarscale = 1 << FRACBITS;
    if (stbar_scale > 1)
      sbarscale = (FRACUNIT*SCREENWIDTH)/320;

    hutextscale = 1 << FRACBITS;
    if (hutext_scale > 1)
      hutextscale = (FRACUNIT*SCREENWIDTH)/320;

    ST_loadData();
    /* Allow max size for dynamic resize of status bar */
    screens[4] = (byte *) Z_Malloc(SCREENWIDTH*ST_HEIGHT*((SCREENWIDTH+319)/320), PU_STATIC, 0);
}

/* ---------------------------------------------------------------------------- */
