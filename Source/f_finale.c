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
//	Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: f_finale.c,v 1.5 1997/02/03 21:26:34 b1 Exp $";
#endif

#include "includes.h"


extern unsigned int secretexit;
extern	patch_t *hu_font[HU_FONTSIZE];
extern boolean finale_message_changed;

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
static int	finalestage;
static int	finalecount;
static int	finalexpos = 10;
static int	finaleypos = 10;

#define	TEXTSPEED	3
#define	TEXTWAIT	250

/* Two copies of each of these to prevent circular defs in the deh files */

char * finale_messages_orig [] =
{
	E0TEXT,
	E1TEXT,
	E2TEXT,
	E3TEXT,
	E4TEXT,
	E5TEXT,
	E6TEXT,
	E7TEXT,
	E8TEXT,
	E9TEXT,

	C1TEXT,
	C2TEXT,
	C3TEXT,
	C4TEXT,
	C5TEXT,
	C6TEXT,
	C7TEXT,
	C8TEXT,
	C9TEXT,

	P1TEXT,
	P2TEXT,
	P3TEXT,
	P4TEXT,
	P5TEXT,
	P6TEXT,
	P7TEXT,
	P8TEXT,
	P9TEXT,

	T1TEXT,
	T2TEXT,
	T3TEXT,
	T4TEXT,
	T5TEXT,
	T6TEXT,
	T7TEXT,
	T8TEXT,
	T9TEXT,
	NULL
};

char * finale_messages [ARRAY_SIZE(finale_messages_orig)];


char *	finale_backdrops_orig [] =
{
  "F_SKY1",
  "FLOOR4_8",
  "SFLR6_1",
  "MFLR8_4",
  "MFLR8_3",
  "SLIME16",
  "RROCK14",
  "RROCK07",
  "RROCK17",
  "RROCK13",
  "RROCK19",
  "BOSSBACK",
  "PFUB2",
  "PFUB1",
  "END0",
  "END%i",
  "CREDIT",
  "HELP",
  "HELP1",
  "HELP2",
  "VICTORY2",
  "ENDPIC",
  "TITLEPIC",
  NULL
};

char *	finale_backdrops [ARRAY_SIZE(finale_backdrops_orig)];

clusterdefs_t * finale_clusterdefs_head = 0;


static char* finaletext;
static char* finaleflat;
static char* finalepic;
static char* nextfinaletext;
static char* nextfinaleflat;
static char* nextfinalepic;

static void F_StartCast (void);
void	F_CastTicker (void);
static boolean F_CastResponder (event_t *ev);

// --------------------------------------------------------------------------------------------

clusterdefs_t * F_Create_ClusterDef (unsigned int num)
{
  clusterdefs_t * cp;
  clusterdefs_t ** cpp;

  cpp = &finale_clusterdefs_head;

  while ((cp = *cpp) != NULL)
  {
    if (cp -> cnumber == num)
      return (cp);
    cpp = &cp -> next;
  }

  cp = malloc (sizeof (clusterdefs_t));
  if (cp)
  {
    *cpp = cp;
    memset (cp, 0, sizeof (clusterdefs_t));
    cp -> cnumber = num;
  }

  return (cp);
}

// --------------------------------------------------------------------------------------------

clusterdefs_t * F_Access_ClusterDef (unsigned int num)
{
  clusterdefs_t * cp;

  cp = finale_clusterdefs_head;
  if (cp)
    while ((cp -> cnumber != num) && ((cp = cp -> next) != NULL));

  return (cp);
}

// --------------------------------------------------------------------------------------------

static void F_DetermineIntermissionTexts (void)
{
  int i,j;
  int text_base;
  int finaletextnum;
  clusterdefs_t * cp_p;
  clusterdefs_t * cp_n;
  map_dests_t * map_info_p;
  map_dests_t * map_info_n;

  finaletext = NULL;
  nextfinaletext = NULL;
  finaleflat = NULL;
  nextfinaleflat = NULL;
  nextfinalepic = finalepic = finale_backdrops[BG_BOSSBACK];


  /* If this map is in a PWAD and the messages haven't */
  /* been changed then inhibit the messages. */

  if ((finale_message_changed == false)
   && (W_SameWadfile (0, G_MapLump (gameepisode, gamemap)) == 0))
    return;

  map_info_p = G_Access_MapInfoTab (gameepisode, gamemap);
  cp_p = F_Access_ClusterDef (map_info_p -> cluster);

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

  if (G_MapLump (i, j) == -1)
  {
    map_info_n = NULL;
    cp_n = NULL;
  }
  else
  {
    map_info_n = G_Access_MapInfoTab (i,j);
    cp_n = F_Access_ClusterDef (map_info_n -> cluster);
  }

  /* If the present map or the next map is in a cluster */
  /* then use the cluster based stuff. */

  if ((cp_p)
   || (cp_n)
   || (map_info_p -> cluster)
   || (map_info_n && map_info_n -> cluster))
  {
    if ((map_info_n == NULL)
     || (map_info_n -> cluster != map_info_p -> cluster))
    {
      if (cp_p)
      {
	finaletext = cp_p -> exittext;
	finaleflat = cp_p -> flat;
	if ((cp_p -> pic)
	 && (W_CheckNumForName (cp_p -> pic) != -1))
	  finalepic = cp_p -> pic;
      }

      /* Big problem here.				*/
      /* Ideally, if we are returning from a secret level */
      /* back to the main flow, then we do not want to	*/
      /* show the entry text for that cluster if it is	*/
      /* the same cluster that we came from.		*/
      /* Snag is that I have long since forgotten where	*/
      /* we came from.					*/

      if (cp_n)
      {
	nextfinaletext = cp_n -> entertext;
	nextfinaleflat = cp_n -> flat;
	if ((cp_n -> pic)
	 && (W_CheckNumForName (cp_n -> pic) != -1))
	  nextfinalepic = cp_n -> pic;
      }

      if ((finaletext == NULL)

       // If we have taken the secret exit, and the next normal
       // level is in the same cluster, then it is probably
       // wrong to show the exit text.
       || ((secretexit != 0)

	&& (G_MapLump (map_info_p -> normal_exit_to_episode,
		       map_info_p -> normal_exit_to_map) != -1)

	&& (G_Access_MapInfoTab (map_info_p -> normal_exit_to_episode,
				 map_info_p -> normal_exit_to_map) -> cluster == map_info_p->cluster)))
      {
	finaletext = nextfinaletext;
	finaleflat = nextfinaleflat;
	finalepic  = nextfinalepic;
	nextfinaletext = NULL;
	nextfinaleflat = NULL;
	nextfinalepic = NULL;
      }
    }
  }
  else
  {
    // Okay - IWAD dependend stuff.
    // This has been changed severly, and
    //  some stuff might have changed in the process.
    finaletextnum = -1;
    switch ( gamemode )
    {
      // DOOM 1 - E1, E3 or E4, but each nine missions
      case shareware:
      case registered:
      case retail:
	S_ChangeMusic(mus_victor, true);
	finaletextnum = gameepisode+e0text;

	switch (gameepisode)
	{
	  case 1:
	    finaleflat = finale_backdrops [BG_FLOOR4_8];
	    break;
	  case 2:
	    finaleflat = finale_backdrops [BG_SFLR6_1];
	    break;
	  case 3:
	    finaleflat = finale_backdrops [BG_MFLR8_4];
	    break;
	  case 4:
	    finaleflat = finale_backdrops [BG_MFLR8_3];
	    break;
	  default:
	    // Ouch.
	    finaleflat = finale_backdrops [BG_F_SKY1];	// Not used anywhere else.
	    break;
	}
	break;

      // DOOM II and missions packs with E1, M34
      case commercial:
	  switch (gamemission)
	  {
	    case pack_plut:
	      text_base = p1text;
	      break;
	    case pack_tnt:
	      text_base = t1text;
	      break;
	    default:
	      text_base = c1text;
	  }

	  S_ChangeMusic(mus_read_m, true);

	  switch (secretexit)
	  {
	    case 0:		/* Not a secret exit */
	      switch (map_info_p -> intermission_text)
	      {
		case 0:
		  break;

		case 1:
		  finaleflat = finale_backdrops [BG_SLIME16];
		  finaletextnum = text_base+0;
		  break;

		case 2:
		  finaleflat = finale_backdrops [BG_RROCK14];
		  finaletextnum = text_base+1;
		  break;

		case 3:
		  finaleflat = finale_backdrops [BG_RROCK07];
		  finaletextnum = text_base+2;
		  break;

		case 4:
		  finaleflat = finale_backdrops [BG_RROCK17];
		  finaletextnum = text_base+3;
		  break;

		case 5:
		  finaleflat = finale_backdrops [BG_RROCK13];
		  finaletextnum = text_base+4;
		  break;

		case 6:
		  finaleflat = finale_backdrops [BG_RROCK19];
		  finaletextnum = text_base+5;
		  break;

		case 7:
		  finaleflat = finale_backdrops [BG_F_SKY1];
		  finaletextnum = text_base+6;
		  break;

		case 8:
		  finaleflat = finale_backdrops [BG_F_SKY1];
		  finaletextnum = text_base+7;
		  break;

		case 9:
		  finaleflat = finale_backdrops [BG_F_SKY1];
		  finaletextnum = text_base+8;
		  break;

		default:
		  // Ouch.
		  finaleflat = finale_backdrops [BG_F_SKY1]; // Not used anywhere else.
		  finaletextnum = t6text;	     // FIXME - other text, music?
		  break;
	      }
	      break;

	    case 1:
	      finaleflat = finale_backdrops [BG_RROCK13];
	      finaletextnum = text_base+4;
	      break;

	    case 2:
	      finaleflat = finale_backdrops [BG_RROCK19];
	      finaletextnum = text_base+5;
	      break;

//	    default:					// Secret secret!
//	      break;
	  }
	  break;


      // Indeterminate.
      default:
	S_ChangeMusic(mus_read_m, true);
	finaleflat = finale_backdrops [BG_F_SKY1];	// Not used anywhere else.
	finaletextnum = c1text;				// FIXME - other text, music?
	break;
    }

    /* Check that this particular message was updated... */
    if ((finaletextnum != -1)
     && ((finale_messages [finaletextnum] != finale_messages_orig [finaletextnum])
      || (W_SameWadfile (0, G_MapLump (gameepisode, gamemap)))))
      finaletext = finale_messages [finaletextnum];
  }
}

// --------------------------------------------------------------------------------------------
//
// F_StartFinale
//
void F_StartFinale (int always)
{
    F_DetermineIntermissionTexts ();
    if (finaletext == NULL)
    {
      if (always)
      {
	gameaction = ga_nothing;
	gamestate = GS_FINALE;
	viewactive = false;
	automapactive = false;
	finalestage = 0;
	finalecount = 0;

	//finaleflat = finale_backdrops [BG_F_SKY1]; // Not used anywhere else.
	//finaletext = finale_messages[c1text];  // FIXME - other text, music?
	if (gamemode == commercial)
	{
	  map_dests_t * map_ptr;
	  map_ptr = G_Access_MapInfoTab_E (255, gamemap);
	  if (G_MapLump (255, map_ptr -> normal_exit_to_map) == -1)
	    F_StartCast ();
	  else
	    gameaction = ga_worlddone;
	}
	else
	{
	  finalestage = 1;
	}
      }
      return;
    }

    gameaction = ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = false;
    finalestage = 0;
    finalecount = 0;
}

// --------------------------------------------------------------------------------------------

boolean F_Responder (event_t *event)
{
    if (finalestage == 2)
	return F_CastResponder (event);

    return false;
}

// --------------------------------------------------------------------------------------------
//
// F_Ticker
//
void F_Ticker (void)
{
    int		fc;
    map_dests_t * map_ptr;

    // check for skipping
    if ( (gamemode == commercial)
      && ( finalecount > 50) )
    {
      // go on to the next level

      if (WI_checkForAccelerate ())
      {
	fc = strlen (finaletext)*TEXTSPEED;
	if (finalecount < fc)
	{
	  finalecount = fc;
	  return;
	}

	if (nextfinaletext)
	{
	  finaletext = nextfinaletext;
	  finaleflat = nextfinaleflat;
	  finalepic  = nextfinalepic;
	  nextfinaletext = NULL;
	  nextfinaleflat = NULL;
	  nextfinalepic  = NULL;
	  finalestage = 0;
	  finalecount = 0;
	  return;
	}

	map_ptr = G_Access_MapInfoTab_E (255, gamemap);
	if (G_MapLump (255, map_ptr -> normal_exit_to_map) == -1)
	  F_StartCast ();
	else
	  G_WorldDone2 ();
      }
    }

    // advance animation
    finalecount++;

    if (finalestage == 2)
    {
	F_CastTicker ();
	return;
    }

    if ( gamemode == commercial)
	return;

    if (finalestage)
      return;

    fc = strlen (finaletext)*TEXTSPEED;
    if ((finalecount > 50)
     && (finalecount < fc)
     && (WI_checkForAccelerate ()))
    {
      finalecount = fc;
      return;
    }


    if ((finalecount > fc)
     && ((WI_checkForAccelerate ())
      || (finalecount > (fc + TEXTWAIT))))
    {
      finalecount = 0;
      if (nextfinaletext)
      {
	finaletext = nextfinaletext;
	finaleflat = nextfinaleflat;
	finalepic  = nextfinalepic;
	nextfinaletext = NULL;
	nextfinaleflat = NULL;
	nextfinalepic  = NULL;
      }
      else
      {
	finalestage = 1;
	wipegamestate = (gamestate_t) -1;		// force a wipe
	if (gameepisode == 3)
	  S_StartMusic (mus_bunny);
      }
    }
}

// --------------------------------------------------------------------------------------------

static unsigned int qty_of_lines (const char * s)
{
  char cc;
  unsigned int q;

  q = 0;

  if (*s)
  {
    do
    {
      cc = *s++;
      if (cc == '\n')
	q++;
    } while (cc);
    if (s[-1] != '\n')
      q++;
  }

  return (q);
}

// --------------------------------------------------------------------------------------------

void F_DrawBackgroundFlat (const char * flat)
{
  int c;
  int x,y,w;
  byte*	src;
  byte*	dest;

  if (((flat == NULL)
   || (flat [0] == 0)
   || ((c = W_CheckNumForName (flat)) == -1))
   && ((c = W_CheckNumForName (finale_backdrops [BG_F_SKY1])) == -1)
   && ((c = W_CheckNumForName (finale_backdrops_orig [BG_F_SKY1])) == -1))
  {
    memset (screens[0], 0x00, SCREENWIDTH * SCREENHEIGHT);
  }
  else
  {
    w = W_LumpLength (c);

    /* Assume that if the flat is > 4096 bytes then it's */
    /* a patch that doesn't need to be tiled.  */
    src = W_CacheLumpNum (c, PU_CACHE);

    if ((w > 4096) && (modifiedgame))
    {
      V_DrawPatchScaled (0,0,0, (patch_t*)src);
    }
    else
    {
      // erase the entire screen to a tiled background
      dest = screens[0];

      for (y=0 ; y<SCREENHEIGHT ; y++)
      {
	for (x=0 ; x<SCREENWIDTH/64 ; x++)
	{
	  memcpy (dest, src+((y&63)<<6), 64);
	  dest += 64;
	}
	if (SCREENWIDTH&63)
	{
	  memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
	  dest += (SCREENWIDTH&63);
	}
      }
    }
  }
  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
}

// --------------------------------------------------------------------------------------------
//
// F_TextWrite
//

static void F_TextWrite (void)
{
  int c;
  int cx;
  int w;
  int cy,dy;
  int count;
  int lines;
  char*	ch;

  // draw some of the text onto the screen

  if (finalecount == 0)
  {
    finalexpos = 10;
    finaleypos = 10;
  }

  count = (finalecount - 10)/TEXTSPEED;
  if (count < 1)
      return;

  lines = qty_of_lines (finaletext);
  if (lines > ((200-10)/11))
  {
    dy = (200-10) / lines;
  }
  else
  {
    dy = 11;
  }

  cx = finalexpos;
  cy = finaleypos;
  ch = finaletext;

  do
  {
    c = *ch++;
    if (!c)
      break;

    if (c == '\n')
    {
      cx = finalexpos;
      cy += dy;
      continue;
    }

    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }

    w = SHORT (hu_font[c]->width);
    if ((cx+w) > 320)
    {
      if (finalexpos)
        finalexpos--;
    }
    else
    {
      V_DrawPatchScaled (cx, cy, 0, hu_font[c]);
      w -= HUlib_Kern (c + HU_FONTSTART, toupper(*ch));
      cx+=w;
    }
  } while (--count);
}

// --------------------------------------------------------------------------------------------
//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//

static const char cc_obit [] = CC_OBIT;

castinfo_t castorder[] =
{
  {CC_ZOMBIE, (char*)cc_obit, MT_POSSESSED},
  {CC_SHOTGUN, (char*)cc_obit, MT_SHOTGUY},
  {CC_HEAVY, (char*)cc_obit, MT_CHAINGUY},
  {CC_WOLF, (char*)cc_obit, MT_WOLFSS},
  {CC_IMP, (char*)cc_obit, MT_TROOP},
  {CC_DEMON, (char*)cc_obit, MT_SERGEANT},
  {CC_SPECTRE, (char*)cc_obit, MT_SHADOWS},
  {CC_LOST, (char*)cc_obit, MT_SKULL},
  {CC_CACO, (char*)cc_obit, MT_HEAD},
  {CC_HELL, (char*)cc_obit, MT_KNIGHT},
  {CC_BARON, (char*)cc_obit, MT_BRUISER},
  {CC_ARACH, (char*)cc_obit, MT_BABY},
  {CC_PAIN, (char*)cc_obit, MT_PAIN},
  {CC_REVEN, (char*)cc_obit, MT_UNDEAD},
  {CC_MANCU, (char*)cc_obit, MT_FATSO},
  {CC_ARCH, (char*)cc_obit, MT_VILE},
  {CC_SPIDER, (char*)cc_obit, MT_SPIDER},
  {CC_CYBER, (char*)cc_obit, MT_CYBORG},
  {CC_HERO, (char*)cc_obit, MT_PLAYER},
  {NULL,NULL, (mobjtype_t) 0}
};

char * cast_names_copy [ARRAY_SIZE(castorder)];

static int		castnum;
static int		casttics;
static state_t*		caststate;
static unsigned int	castdeath;
static int		castframes;
static int		castonmelee;
static boolean		castattacking;
static int		castrot;

// --------------------------------------------------------------------------------------------

char * F_CastName (mobjtype_t rtype)
{
  castinfo_t * cast_ptr;

  cast_ptr = &castorder[0];

  do
  {
    if (cast_ptr -> type == rtype)
      return (cast_ptr -> name);
    cast_ptr++;
  } while (cast_ptr -> name);
  return ("Unknown");
}

// --------------------------------------------------------------------------------------------

static void F_NextCast (void)
{
  int attempts;
  castinfo_t * cast_ptr;

  attempts = ARRAY_SIZE(castorder) + 2;   // Don't get stuck in a loop looking
		     		      // for cast members if some silly bugger
		     		      // has emptied ALL the strings!

  cast_ptr = &castorder[castnum];
  do
  {
    castnum++;
    cast_ptr++;
    if (cast_ptr -> name == NULL)
    {
      castnum = 0;
      cast_ptr = &castorder[0];
    }
  } while ((cast_ptr -> name[0] == 0)
	&& (--attempts));
}


// --------------------------------------------------------------------------------------------
//
// F_StartCast
//

static void F_StartCast (void)
{
    wipegamestate = (gamestate_t) -1;		// force a screen wipe
    castnum = -1;
    castrot = 0;
    F_NextCast ();
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    casttics = (int) caststate->tics;
    castdeath = 0;
    finalestage = 2;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;
    S_ChangeMusic(mus_evil, true);
}


// --------------------------------------------------------------------------------------------
//
// F_CastTicker
//
void F_CastTicker (void)
{
    int		st;
    int		sfx;

    if (--casttics > 0)
	return;			// not time to change state yet

    if (((castdeath) && (--castdeath == 0))
     || (caststate->tics == -1 || caststate->nextstate == S_NULL))
    {
	// switch from deathstate to next monster
	F_NextCast ();
	castdeath = 0;
	castrot = 0;
	if (mobjinfo[castorder[castnum].type].seesound)
	    S_StartSound (NULL, mobjinfo[castorder[castnum].type].seesound);
	caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	castframes = 0;
    }
    else
    {
	// just advance to next state in animation
	if (caststate == &states[S_PLAY_ATK1])
	    goto stopattack;	// Oh, gross hack!
	st = caststate->nextstate;
	caststate = &states[st];
	castframes++;

	// sound hacks....
	switch (st)
	{
	  case S_PLAY_ATK1:	sfx = sfx_dshtgn; break;
	  case S_POSS_ATK2:	sfx = sfx_pistol; break;
	  case S_SPOS_ATK2:	sfx = sfx_shotgn; break;
	  case S_VILE_ATK2:	sfx = sfx_vilatk; break;
	  case S_SKEL_FIST2:	sfx = sfx_skeswg; break;
	  case S_SKEL_FIST4:	sfx = sfx_skepch; break;
	  case S_SKEL_MISS2:	sfx = sfx_skeatk; break;
	  case S_FATT_ATK8:
	  case S_FATT_ATK5:
	  case S_FATT_ATK2:	sfx = sfx_firsht; break;
	  case S_CPOS_ATK2:
	  case S_CPOS_ATK3:
	  case S_CPOS_ATK4:	sfx = sfx_shotgn; break;
	  case S_TROO_ATK3:	sfx = sfx_claw; break;
	  case S_SARG_ATK2:	sfx = sfx_sgtatk; break;
	  case S_BOSS_ATK2:
	  case S_BOS2_ATK2:
	  case S_HEAD_ATK2:	sfx = sfx_firsht; break;
	  case S_SKULL_ATK2:	sfx = sfx_sklatk; break;
	  case S_SPID_ATK2:
	  case S_SPID_ATK3:	sfx = sfx_shotgn; break;
	  case S_BSPI_ATK2:	sfx = sfx_plasma; break;
	  case S_CYBER_ATK2:
	  case S_CYBER_ATK4:
	  case S_CYBER_ATK6:	sfx = sfx_rlaunc; break;
	  case S_PAIN_ATK3:	sfx = sfx_sklatk; break;
	  default: sfx = 0; break;
	}

	if (sfx)
	    S_StartSound (NULL, sfx);
    }

    if (castframes == 12)
    {
	// go into attack frame
	castattacking = true;
	if (castonmelee)
	    caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
	else
	    caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
	castonmelee ^= 1;
	if (caststate == &states[S_NULL])
	{
	    if (castonmelee)
		caststate=
		    &states[mobjinfo[castorder[castnum].type].meleestate];
	    else
		caststate=
		    &states[mobjinfo[castorder[castnum].type].missilestate];
	}
    }

    if (castattacking)
    {
	if (castframes == 24
	    ||	caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
	{
	  stopattack:
	    castattacking = false;
	    castframes = 0;
	    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
	}
    }

    casttics = (int) caststate->tics;
    if (casttics == -1)
	casttics = 15;
}


// --------------------------------------------------------------------------------------------
//
// F_CastResponder
//

static boolean F_CastResponder (event_t* ev)
{
    if (ev->type != ev_keydown)
	return false;

    if (castdeath)
	return true;			// already in dying frames


    switch (ev->data1)			// Based on code from Doom Retro
    {					// which in turn based on Doom Eternal.
      case KEY_LEFTARROW:
	castrot++;
	return true;

      case KEY_RIGHTARROW:
	castrot--;
	return true;
    }

    /* The PAC-MAN character in original.wad crashes here, so */
    /* only do this with items that are shootable. */
    if (((mobjinfo[castorder[castnum].type].flags & MF_SHOOTABLE) == 0)
     || (mobjinfo[castorder[castnum].type].deathstate == S_NULL))
    {
      castdeath = 10;
    }
    else
    {
      // go into death frame
      castdeath = 50;			// Timer in case someone has messed
					// up the DeHacked and this baddie
					// never actually dies.

      caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
      casttics = (int)caststate->tics;
    }

    castframes = 0;
    castrot = 0;
    castattacking = false;

    if (mobjinfo[castorder[castnum].type].deathsound)
	S_StartSound (NULL, mobjinfo[castorder[castnum].type].deathsound);

    return true;
}

// --------------------------------------------------------------------------------------------

static void F_CastPrint (char* text)
{
    char*	ch;
    int		c;
    int		cx;
    int		w;
    int		width;

    // find width
    ch = text;
    width = 0;

    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    width += 4;
	    continue;
	}

	w = SHORT (hu_font[c]->width);
	width += w;
    }

    // draw it
    cx = 160-width/2;
    ch = text;
    while (ch)
    {
	c = *ch++;
	if (!c)
	    break;
	c = toupper(c) - HU_FONTSTART;
	if (c < 0 || c> HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}

	w = (SHORT (hu_font[c]->width)) - HUlib_Kern (c+HU_FONTSTART, toupper(*ch));
	V_DrawPatchScaled(cx, 180, 0, hu_font[c]);
	cx+=w;
    }
}

// --------------------------------------------------------------------------------------------
//
// F_CastDrawer
//

static void F_CastDrawer (void)
{
  int x,y;
  int lump;
  int s1,s2;
  int rot;
  int drawstyle;
  fixed_t scale;
  patch_t* patch;
  mobjinfo_t* info;
  spritedef_t* sprdef;
  spriteframe_t* sprframe;

  F_CastPrint (castorder[castnum].name);

  // draw the current frame in the middle of the screen
  sprdef = &sprites[caststate->sprite];
  sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];

  rot = 0;
  if (sprframe->rotate)
    rot = castrot & 7;

  lump = sprframe->lump[rot];
  patch = W_CacheLumpNum (lump, PU_CACHE);

  drawstyle = 0;
  if (sprframe->flip[rot])
    drawstyle |= 1;			/* Bit 0 = draw flipped */

  info = &mobjinfo[castorder[castnum].type];

  if (info->flags & MF_SHADOW)
    drawstyle |= 2;			/* Bit 1 = draw fuzzy */

  scale = info->scale;

#if 0
  if (scale == FRACUNIT)		/* Always do it! */
  {
    V_DrawPatchFlippedScaled (160, 170, 0, patch, drawstyle);
  }
  else
#endif
  {
    scale <<= 2;

    s1 = SCREENWIDTH / 320;
    s2 = SCREENHEIGHT / 200;

    if (s2 < s1) s1 = s2;

    do
    {
      scale >>= 1;

      y = 170 - (((SHORT(patch->topoffset) * scale) / s1) >> FRACBITS);
      x = 160 - (((SHORT(patch->leftoffset) * scale) / s1) >> FRACBITS);

      y *= s1;
      x *= s1;

      y += (SCREENHEIGHT - (s1 * 200))/2;
      x += (SCREENWIDTH  - (s1 * 320))/2;

    } while ((x < 0) || (y < 0));

    if (scale)
      V_DrawPatchScaleFlip (x, y, 0, patch, scale, scale, drawstyle);
  }
}


// --------------------------------------------------------------------------------------------
//
// F_DrawPatchCol
//
static void
F_DrawPatchCol
( int		x,
  patch_t*	patch,
  int		col )
{
  int y;
  int td;
  int topdelta;
  int lastlength;
  fixed_t row;
  fixed_t colm;
  fixed_t xscale,yscale;
  fixed_t xiscale,yiscale;
  byte*	source;
  byte*	dest;
  byte*	desttop;
  column_t* column;

  xscale = (SCREENWIDTH << FRACBITS) / 320;
  yscale = (SCREENHEIGHT << FRACBITS) / 200;

  x = (x * xscale) >> FRACBITS;
  y = 0;

  if (SHORT(patch->height) > 200)
  {
    yiscale = FixedDiv (SHORT(patch->height) << FRACBITS, SCREENHEIGHT << FRACBITS);
  }
  else
  {
    if (yscale == FRACUNIT)
      yiscale = yscale;
    else
      yiscale = FixedDiv (FRACUNIT, yscale);
  }

  if (xscale == FRACUNIT)
    xiscale = xscale;
  else
    xiscale = FixedDiv (FRACUNIT, xscale);

  desttop = screens[0] + x + (y*SCREENWIDTH);

  colm = 0;
  do
  {
    topdelta = -1;
    lastlength = 0;
    column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

    // step through the posts in a column
    while ((td = column->topdelta) != 0xff )
    {
      if (td < (topdelta+(lastlength-1)))		// Bodge for oversize patches
      {
	topdelta += td;
      }
      else
      {
	topdelta = td;
      }

      lastlength = column->length;
      source = (byte *)column + 3;
      dest = desttop + (topdelta*SCREENWIDTH);

      row = 0;
      do
      {
	*dest = source [row >> FRACBITS];
	dest += SCREENWIDTH;
	row += yiscale;
      } while ((row >> FRACBITS) < lastlength);

      column = (column_t *)(  (byte *)column + lastlength + 4 );
    }
    desttop++;
    colm += xiscale;
  } while (colm < (1 << FRACBITS));
}


// --------------------------------------------------------------------------------------------
//
// F_BunnyScroll
//
static void F_BunnyScroll (void)
{
    int		scrolled;
    int		x;
    patch_t*	p1;
    patch_t*	p2;
    char	name[10];
    int		stage;
    static int	laststage;

    p1 = W_CacheLumpName (finale_backdrops[BG_PFUB2], PU_LEVEL);
    p2 = W_CacheLumpName (finale_backdrops[BG_PFUB1], PU_LEVEL);

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
	scrolled = 320;
    if (scrolled < 0)
	scrolled = 0;

    for ( x=0 ; x<320 ; x++)
    {
	if (x+scrolled < 320)
	    F_DrawPatchCol (x, p1, x+scrolled);
	else
	    F_DrawPatchCol (x, p2, x+scrolled - 320);
    }

    if (finalecount < 1130)
	return;
    if (finalecount < 1180)
    {
      p1 = W_CacheLumpName0 (finale_backdrops[BG_END0],PU_CACHE);
      if (p1)
	V_DrawPatchScaled (108,68,0,p1);
      laststage = 0;
      return;
    }

    stage = (finalecount-1180) / 5;
    if (stage > 6)
	stage = 6;
    if (stage > laststage)
    {
	S_StartSound (NULL, sfx_pistol);
	laststage = stage;
    }

    sprintf (name,finale_backdrops[BG_ENDI],stage);
    p1 = W_CacheLumpName0 (name,PU_CACHE);
    if (p1)
      V_DrawPatchScaled (108,68,0,p1);
}


// --------------------------------------------------------------------------------------------
//
// F_Drawer
//
void F_Drawer (void)
{
    switch (finalestage)
    {
      case 0:
	if (finaleflat == NULL)
	  F_DrawBackgroundFlat (finalepic);
	else
	  F_DrawBackgroundFlat (finaleflat);
	F_TextWrite ();
	break;

      case 2:
	D_PageDrawer (finalepic);
	F_CastDrawer ();
	break;

      default:
	switch (gameepisode)
	{
	  case 1:
	    if ( gamemode == retail )
	      D_PageDrawer (finale_backdrops[BG_CREDIT]);
	    else
	      D_PageDrawer (finale_backdrops[BG_HELP2]);
	    break;

	  case 2:
	    D_PageDrawer (finale_backdrops[BG_VICTORY2]);
	    break;

	  case 3:
	    F_BunnyScroll ();
	    break;

	  default:
	    D_PageDrawer (finale_backdrops[BG_ENDPIC]);
	    break;
	}
    }
}

// --------------------------------------------------------------------------------------------
