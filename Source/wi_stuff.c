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
//	Intermission screens.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: wi_stuff.c,v 1.7 1997/02/03 22:45:13 b1 Exp $";
#endif

#include "includes.h"

static void WI_unloadData (void);

/* ---------------------------------------------------------------------------- */
//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES	4
#define NUMMAPS		9


// in tics
//U #define PAUSELEN		(TICRATE*2)
//U #define SCORESTEP		100
//U #define ANIMPERIOD		32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST		10
//U #define WK 1

#define WI_SCREENWIDTH		320
#define WI_SCREENHEIGHT		200

// GLOBAL LOCATIONS
#define WI_TITLEY		((WI_SCREENHEIGHT/2)-100+2)
#define WI_SPACINGY    		33

// SINGPLE-PLAYER STUFF
#define SP_STATSX		((WI_SCREENWIDTH/2)-160+50)
#define SP_STATSY		((WI_SCREENHEIGHT/2)-100+50)

#define SP_TIMEX		((WI_SCREENWIDTH/2)-160+16)
#define SP_TIMEY		((WI_SCREENHEIGHT/2)-100+168)


// NET GAME STUFF
#define NG_STATSY		((WI_SCREENHEIGHT/2)-100+50)
#define NG_STATSX		(32 + SHORT(star->width)/2 + 32*!dofrags)

#define NG_SPACINGX    		64


// DEATHMATCH STUFF
#define DM_MATRIXX		((WI_SCREENWIDTH/2)-160+42)
#define DM_MATRIXY		((WI_SCREENHEIGHT/2)-100+68)

#define DM_SPACINGX		40

#define DM_TOTALSX		((WI_SCREENWIDTH/2)-160+269)

#define DM_KILLERSX		((WI_SCREENWIDTH/2)-160+10)
#define DM_KILLERSY		((WI_SCREENHEIGHT/2)-100+100)
#define DM_VICTIMSX    		((WI_SCREENWIDTH/2)-160+5)
#define DM_VICTIMSY		((WI_SCREENHEIGHT/2)-100+150)

/* ---------------------------------------------------------------------------- */

typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL
} animenum_t;

typedef struct
{
    int		x;
    int		y;
} point_t;


//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
    animenum_t	type;

    // period in tics between animations
    int		period;

    // number of animation frames
    int		nanims;

    // location of animation
    point_t	loc;

    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    int		data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    int		data2;

    // actual graphics for frames of animations
    patch_t*	p[3];

    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    int		nexttic;

    // last drawn animation frame
    int		lastdrawn;

    // next frame number to animate
    int		ctr;

    // used by RANDOM and LEVEL when animating
    int		state;

} anim_t;


static const point_t lnodes[][NUMMAPS] =
{
    // Episode 0 World Map
    {
	{ 185, 164 },	// location of level 0 (CJ)
	{ 148, 143 },	// location of level 1 (CJ)
	{ 69, 122 },	// location of level 2 (CJ)
	{ 209, 102 },	// location of level 3 (CJ)
	{ 116, 89 },	// location of level 4 (CJ)
	{ 166, 55 },	// location of level 5 (CJ)
	{ 71, 56 },	// location of level 6 (CJ)
	{ 135, 29 },	// location of level 7 (CJ)
	{ 71, 24 }	// location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
	{ 254, 25 },	// location of level 0 (CJ)
	{ 97, 50 },	// location of level 1 (CJ)
	{ 188, 64 },	// location of level 2 (CJ)
	{ 128, 78 },	// location of level 3 (CJ)
	{ 214, 92 },	// location of level 4 (CJ)
	{ 133, 130 },	// location of level 5 (CJ)
	{ 208, 136 },	// location of level 6 (CJ)
	{ 148, 140 },	// location of level 7 (CJ)
	{ 235, 158 }	// location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
	{ 156, 168 },	// location of level 0 (CJ)
	{ 48, 154 },	// location of level 1 (CJ)
	{ 174, 95 },	// location of level 2 (CJ)
	{ 265, 75 },	// location of level 3 (CJ)
	{ 130, 48 },	// location of level 4 (CJ)
	{ 279, 23 },	// location of level 5 (CJ)
	{ 198, 48 },	// location of level 6 (CJ)
	{ 140, 25 },	// location of level 7 (CJ)
	{ 281, 136 }	// location of level 8 (CJ)
    }
#if 0
    // Episode 4 World Map should go here
    {
	{ 0, 0 },	// location of level 0 (JAD)
	{ 0, 0 },	// location of level 1 (JAD)
	{ 0, 0 },	// location of level 2 (JAD)
	{ 0, 0 },	// location of level 3 (JAD)
	{ 0, 0 },	// location of level 4 (JAD)
	{ 0, 0 },	// location of level 5 (JAD)
	{ 0, 0 },	// location of level 6 (JAD)
	{ 0, 0 },	// location of level 7 (JAD)
	{ 0, 0 },	// location of level 8 (JAD)
    }
#endif
};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_t epsd1animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_t epsd2animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static const int NUMANIMS[] =
{
    sizeof(epsd0animinfo)/sizeof(anim_t),
    sizeof(epsd1animinfo)/sizeof(anim_t),
    sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[] =
{
    epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo
};

/* ---------------------------------------------------------------------------- */
//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0


// States for single-player
#define SP_KILLS		0
#define SP_ITEMS		2
#define SP_SECRET		4
#define SP_FRAGS		6
#define SP_TIME			8
#define SP_PAR			ST_TIME

#define SP_PAUSE		1

// in seconds
#define SHOWNEXTLOCDELAY	4
//#define SHOWLASTLOCDELAY	SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static int		acceleratestage;

// wbs->pnum
static int		me;

 // specifies current state
static stateenum_t	state;

// contains information passed into intermission
static wbstartstruct_t*	wbs;

static wbplayerstruct_t* plrs;  // wbs->plyr[]

// used for general timing
static int 		cnt;

// used for timing of background animation
static int 		bcnt;

// signals to refresh everything for one frame
static int 		firstrefresh;

static int		cnt_kills[MAXPLAYERS];
static int		cnt_items[MAXPLAYERS];
static int		cnt_secret[MAXPLAYERS];
static int		cnt_time;
static int		cnt_par;
static int		cnt_pause;

// # of commercial levels
// #define	NUMCMAPS	32


//
//	GRAPHICS
//

// background (map of levels).
static patch_t*		bg_exit;
static patch_t*		bg_enter;

// You Are Here graphic
static patch_t*		yah[2];

// splat
static patch_t*		splat;

// %, : graphics
static patch_t*		percent;
static patch_t*		colon;

// 0-9 graphic
static patch_t*		num[10];

// minus sign
static patch_t*		wiminus;

// "Finished!" graphics
static patch_t*		finished;

// "Entering" graphic
static patch_t*		entering;

// "secret"
static patch_t*		sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static patch_t*		kills;
static patch_t*		secret;
static patch_t*		items;
static patch_t*		frags;

// Time sucks.
static patch_t*		wtime;		// Changed from "time" because clashes with something
					// in the cygwin include files.... (JAD)
static patch_t*		par;
static patch_t*		sucks;

// "killers", "victims"
static patch_t*		killers;
static patch_t*		victims;

// "Total", your face, your dead face
static patch_t*		total;
static patch_t*		star;
static patch_t*		bstar;

// "red P[1..MAXPLAYERS]"
static patch_t*		p[MAXPLAYERS];

// "gray P[1..MAXPLAYERS]"
static patch_t*		bp[MAXPLAYERS];

 // Name graphics of each level (centered)
static patch_t*		lpatch_last;
static patch_t*		lpatch_next;
static char *		ltext_last;
static char *		ltext_next;

/* ---------------------------------------------------------------------------- */
//
// CODE
//

// slam background
// UNUSED static unsigned char *background=0;


static void WI_slamBackground(void)
{
    memcpy(screens[0], screens[1], SCREENWIDTH * SCREENHEIGHT);
    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
}

/* ---------------------------------------------------------------------------- */
// The ticker is used to detect keys
//  because of timing issues in netgames.
boolean WI_Responder(event_t* ev)
{
    return false;
}

/* ---------------------------------------------------------------------------- */

// Draws "<Levelname> Finished!"
static void WI_drawLF (void)
{
    int x;
    int y;

    y = WI_TITLEY;

    if (lpatch_last != 0)
    {
      x = (WI_SCREENWIDTH - SHORT(lpatch_last->width))/2;
      if (x < 0) x = 0;

      // draw <LevelName>
      // Arctic wad has overlarge names (384 pixels)
      // so don't bother to scale up.
      if (SHORT(lpatch_last->width) > WI_SCREENWIDTH)
	V_DrawPatch (x, y, FB, lpatch_last);
      else
	V_DrawPatchScaled (x, y, FB, lpatch_last);

      y += (5*SHORT(lpatch_last->height))/4;
    }
    else if (ltext_last)
    {
      V_drawWILV (y, ltext_last);
      y += 14;
    }

    // draw "Finished!"

    x = (WI_SCREENWIDTH - SHORT(finished->width))/2;
    if (x < 0) x = 0;

    if (SHORT(finished->width) > WI_SCREENWIDTH)
      V_DrawPatch (x, y, FB, finished);
    else
      V_DrawPatchScaled (x, y, FB, finished);
}

/* ---------------------------------------------------------------------------- */
// Draws "Entering <LevelName>"
static void WI_drawEL (void)
{
  int x;
  int y;

  if (lpatch_next || ltext_next)
  {
    y = WI_TITLEY;
    x = (WI_SCREENWIDTH - SHORT(entering->width))/2;
    if (x < 0) x = 0;

    // draw "Entering"
    if (SHORT(entering->width) > WI_SCREENWIDTH)
      V_DrawPatch (x, y, FB, entering);
    else
      V_DrawPatchScaled (x, y, FB, entering);

    y += (5*SHORT(entering->height))/4;

    if (lpatch_next)
    {
      // draw level
      x = (WI_SCREENWIDTH - SHORT(lpatch_next->width))/2;
      if (x < 0) x = 0;

      if (SHORT(lpatch_next->width) > WI_SCREENWIDTH)
	V_DrawPatch (x, y, FB, lpatch_next);
      else
	V_DrawPatchScaled (x, y, FB, lpatch_next);
    }
    else
    {
      V_drawWILV (y, ltext_next);
    }
  }
}

/* ---------------------------------------------------------------------------- */

static void
WI_drawOnLnode (int n, patch_t*	c[])
{
  int		i;
  int		left;
  int		top;
  int		right;
  int		bottom;
  const point_t *ln_ptr;
  boolean	fits = false;

  ln_ptr = &lnodes[wbs->epsd][n];
  if ((ln_ptr -> x != 0)
   && (ln_ptr -> y != 0))
  {
    i = 0;
    do
    {
	left = ln_ptr -> x - SHORT(c[i]->leftoffset) + ((WI_SCREENWIDTH/2)-160);
	top = ln_ptr -> y - SHORT(c[i]->topoffset) + ((WI_SCREENHEIGHT/2)-100);
	right = left + SHORT(c[i]->width);
	bottom = top + SHORT(c[i]->height);

	if (left >= 0
	    && right < WI_SCREENWIDTH
	    && top >= 0
	    && bottom < WI_SCREENHEIGHT)
	{
	    fits = true;
	}
	else
	{
	    i++;
	}
    } while (!fits && i!=2);

    if (fits && i<2)
    {
      left = ln_ptr -> x + ((WI_SCREENWIDTH/2)-160);
      top  = ln_ptr -> y + ((WI_SCREENHEIGHT/2)-100);
	V_DrawPatchScaled(left, top, FB, c[i]);
    }
    else
    {
	// DEBUG
	printf("Could not place patch on level %d", n+1);
    }
  }
}

/* ---------------------------------------------------------------------------- */

static void WI_initAnimatedBack (void)
{
  int	i;
  anim_t* a;

  if ((gamemode != commercial)
   && (wbs->epsd < ARRAY_SIZE(anims)))
  {
    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
	a = &anims[wbs->epsd][i];

	// init variables
	a->ctr = -1;

	// specify the next time to draw it
	if (a->type == ANIM_ALWAYS)
	    a->nexttic = bcnt + 1 + (M_Random()%a->period);
	else if (a->type == ANIM_RANDOM)
	    a->nexttic = bcnt + 1 + a->data2+(M_Random()%a->data1);
	else if (a->type == ANIM_LEVEL)
	    a->nexttic = bcnt + 1;
    }
  }
}

/* ---------------------------------------------------------------------------- */

static void WI_updateAnimatedBack(void)
{
  int		i;
  anim_t*	a;

  if ((gamemode != commercial)
   && (wbs->epsd < ARRAY_SIZE(anims)))
  {
    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
	a = &anims[wbs->epsd][i];

	if (bcnt == a->nexttic)
	{
	    switch (a->type)
	    {
	      case ANIM_ALWAYS:
		if (++a->ctr >= a->nanims) a->ctr = 0;
		a->nexttic = bcnt + a->period;
		break;

	      case ANIM_RANDOM:
		a->ctr++;
		if (a->ctr == a->nanims)
		{
		    a->ctr = -1;
		    a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
		}
		else a->nexttic = bcnt + a->period;
		break;

	      case ANIM_LEVEL:
		// gawd-awful hack for level anims
		if (!(state == StatCount && i == 7)
		    && wbs->next == a->data1)
		{
		    a->ctr++;
		    if (a->ctr == a->nanims) a->ctr--;
		    a->nexttic = bcnt + a->period;
		}
		break;
	    }
	}
    }
  }
}

/* ---------------------------------------------------------------------------- */

static void WI_drawAnimatedBack(void)
{
  int		i;
  anim_t*	a;

  if ((gamemode != commercial)
   && (wbs->epsd < ARRAY_SIZE(anims)))
  {
    for (i=0 ; i<NUMANIMS[wbs->epsd] ; i++)
    {
	a = &anims[wbs->epsd][i];

	if (a->ctr >= 0)
	    V_DrawPatchScaled(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
    }
  }
}

/* ---------------------------------------------------------------------------- */
//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

static int
WI_drawNum
( int		x,
  int		y,
  int		n,
  int		digits )
{

    int		fontwidth = SHORT(num[0]->width);
    int		neg;
    int		temp;

    if (digits < 0)
    {
	if (!n)
	{
	    // make variable-length zeros 1 digit long
	    digits = 1;
	}
	else
	{
	    // figure out # of digits in #
	    digits = 0;
	    temp = n;

	    while (temp)
	    {
		temp /= 10;
		digits++;
	    }
	}
    }

    neg = n < 0;
    if (neg)
	n = -n;

    // if non-number, do not draw it
    if (n == 1994)
	return 0;

    // draw the new number
    while (digits--)
    {
	x -= fontwidth;
	V_DrawPatchScaled(x, y, FB, num[ n % 10 ]);
	n /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
	V_DrawPatchScaled(x-=8, y, FB, wiminus);

    return x;

}

/* ---------------------------------------------------------------------------- */

static void
WI_drawPercent
( int		x,
  int		y,
  int		p )
{
    if (p < 0)
	return;

    V_DrawPatchScaled(x, y, FB, percent);
    WI_drawNum(x, y, p, -1);
}

/* ---------------------------------------------------------------------------- */
//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void
WI_drawTime
( int		x,
  int		y,
  int		t,
  int		s )
{

    int		div;
    int		n;

    if (t<0)
	return;

    if (t < s)
    {
	div = 1;

	do
	{
	    n = (t / div) % 60;
	    x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
	    div *= 60;

	    // draw
	    if (div==60 || t / div)
		V_DrawPatchScaled(x, y, FB, colon);

	} while (t / div);
    }
    else
    {
	// "sucks"
	V_DrawPatchScaled(x - SHORT(sucks->width), y, FB, sucks);
    }
}

/* ---------------------------------------------------------------------------- */

static void WI_End(void)
{
    WI_unloadData();
}

/* ---------------------------------------------------------------------------- */

static void WI_initNoState(void)
{
    state = NoState;
    acceleratestage = 0;
    cnt = 10;
}

/* ---------------------------------------------------------------------------- */

static void WI_updateNoState(void) {

    WI_updateAnimatedBack();

    if (!--cnt)
    {
	WI_End();
	G_WorldDone();
    }

}

/* ---------------------------------------------------------------------------- */

static boolean		snl_pointeron = false;


static void WI_initShowNextLoc(void)
{
    state = ShowNextLoc;
    acceleratestage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_initAnimatedBack();
}

/* ---------------------------------------------------------------------------- */

static void WI_updateShowNextLoc(void)
{
    WI_updateAnimatedBack();

    if (!--cnt || acceleratestage)
	WI_initNoState();
    else
	snl_pointeron = (boolean)((cnt & 31) < 20);
}

/* ---------------------------------------------------------------------------- */

static void WI_drawShowNextLoc(void)
{

    int		i;
    int		last;

    if (bg_enter)
    {
      V_DrawPatchScaled(0, 0, 1, bg_enter);
      bg_enter = 0;
    }

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    if ((gamemode != commercial)
     && (wbs->epsd < ARRAY_SIZE(lnodes)))
    {
	last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

	// draw a splat on taken cities.
	for (i=0 ; i<=last ; i++)
	    WI_drawOnLnode(i, &splat);

	// splat the secret level?
	if (wbs->didsecret)
	    WI_drawOnLnode(8, &splat);

	// draw flashing ptr
	if (snl_pointeron)
	    WI_drawOnLnode(wbs->next, yah);
    }

    // draws which level you are entering..
    // if ( (gamemode != commercial)		Removed by JAD 30/10/99
    //	 || wbs->next != 30)
	WI_drawEL();

}

/* ---------------------------------------------------------------------------- */

static void WI_drawNoState(void)
{
    snl_pointeron = true;
    WI_drawShowNextLoc();
}

/* ---------------------------------------------------------------------------- */

static int WI_fragSum(int playernum)
{
    int		i;
    int		frags = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i]
	    && i!=playernum)
	{
	    frags += plrs[playernum].frags[i];
	}
    }


    // JDC hack - negative frags.
    frags -= plrs[playernum].frags[playernum];
    // UNUSED if (frags < 0)
    // 	frags = 0;

    return frags;
}

/* ---------------------------------------------------------------------------- */


static int		dm_state;
static int		dm_frags[MAXPLAYERS][MAXPLAYERS];
static int		dm_totals[MAXPLAYERS];



static void WI_initDeathmatchStats (void)
{
  int		i;
  int		j;

  state = StatCount;
  acceleratestage = 0;
  dm_state = 1;

  cnt_pause = TICRATE;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (playeringame[i])
    {
      for (j=0 ; j<MAXPLAYERS ; j++)
	if (playeringame[j])
	  dm_frags[i][j] = 0;

      dm_totals[i] = 0;
    }
  }

  WI_initAnimatedBack();
}

/* ---------------------------------------------------------------------------- */

static void WI_updateDeathmatchStats(void)
{

    int		i;
    int		j;

    boolean	stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && dm_state != 4)
    {
	acceleratestage = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (playeringame[i])
	    {
		for (j=0 ; j<MAXPLAYERS ; j++)
		    if (playeringame[j])
			dm_frags[i][j] = plrs[i].frags[j];

		dm_totals[i] = WI_fragSum(i);
	    }
	}


	S_StartSound(0, sfx_barexp);
	dm_state = 4;
    }

    switch (dm_state)
    {
      case 2:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (playeringame[i])
	    {
		for (j=0 ; j<MAXPLAYERS ; j++)
		{
		    if (playeringame[j]
			&& dm_frags[i][j] != plrs[i].frags[j])
		    {
			if (plrs[i].frags[j] < 0)
			    dm_frags[i][j]--;
			else
			    dm_frags[i][j]++;

			if (dm_frags[i][j] > 99)
			    dm_frags[i][j] = 99;

			if (dm_frags[i][j] < -99)
			    dm_frags[i][j] = -99;

			stillticking = true;
		    }
		}
		dm_totals[i] = WI_fragSum(i);

		if (dm_totals[i] > 99)
		    dm_totals[i] = 99;

		if (dm_totals[i] < -99)
		    dm_totals[i] = -99;
	    }

	}
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    dm_state++;
	}
	break;

      case 4:
	if (acceleratestage)
	{
	    S_StartSound(0, sfx_slop);

	    if ( gamemode == commercial)
		WI_initNoState();
	    else
		WI_initShowNextLoc();
	}
	break;

      default:
	if (!--cnt_pause)
	{
	    dm_state++;
	    cnt_pause = TICRATE;
	}
    }
}

/* ---------------------------------------------------------------------------- */

static void WI_drawDeathmatchStats(void)
{

    int		i;
    int		j;
    int		x;
    int		y;
    int		w;

    int		lh;	// line height

    lh = WI_SPACINGY;

    if (bg_exit)
    {
      V_DrawPatchScaled(0, 0, 1, bg_exit);
      bg_exit = 0;
    }

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    WI_drawLF();

    // draw stat titles (top line)
    V_DrawPatchScaled(DM_TOTALSX-SHORT(total->width)/2,
		DM_MATRIXY-WI_SPACINGY+10,
		FB,
		total);

    V_DrawPatchScaled(DM_KILLERSX, DM_KILLERSY, FB, killers);
    V_DrawPatchScaled(DM_VICTIMSX, DM_VICTIMSY, FB, victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (playeringame[i])
	{
	    V_DrawPatchScaled(x-SHORT(p[i]->width)/2,
			DM_MATRIXY - WI_SPACINGY,
			FB,
			p[i]);

	    V_DrawPatchScaled(DM_MATRIXX-SHORT(p[i]->width)/2,
			y,
			FB,
			p[i]);

	    if (i == me)
	    {
		V_DrawPatchScaled(x-SHORT(p[i]->width)/2,
			    DM_MATRIXY - WI_SPACINGY,
			    FB,
			    bstar);

		V_DrawPatchScaled(DM_MATRIXX-SHORT(p[i]->width)/2,
			    y,
			    FB,
			    star);
	    }
	}
	else
	{
	    // V_DrawPatch(x-SHORT(bp[i]->width)/2,
	    //   DM_MATRIXY - WI_SPACINGY, FB, 0, bp[i]);
	    // V_DrawPatch(DM_MATRIXX-SHORT(bp[i]->width)/2,
	    //   y, FB, 0, bp[i]);
	}
	x += DM_SPACINGX;
	y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = SHORT(num[0]->width);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	x = DM_MATRIXX + DM_SPACINGX;

	if (playeringame[i])
	{
	    for (j=0 ; j<MAXPLAYERS ; j++)
	    {
		if (playeringame[j])
		    WI_drawNum(x+w, y, dm_frags[i][j], 2);

		x += DM_SPACINGX;
	    }
	    WI_drawNum(DM_TOTALSX+w, y, dm_totals[i], 2);
	}
	y += WI_SPACINGY;
    }
}

/* ---------------------------------------------------------------------------- */

static int	cnt_frags[MAXPLAYERS];
static int	dofrags;
static int	ng_state;

static void WI_initNetgameStats (void)
{

    int i;

    state = StatCount;
    acceleratestage = 0;
    ng_state = 1;

    cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;

	cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

	dofrags += WI_fragSum(i);
    }

    dofrags = !!dofrags;

    WI_initAnimatedBack();
}

/* ---------------------------------------------------------------------------- */

static void WI_updateNetgameStats(void)
{

    int		i;
    int		fsum;

    boolean	stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && ng_state != 10)
    {
	acceleratestage = 0;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!playeringame[i])
		continue;

	    if (wbs->maxkills)
	    {
	      cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
	    }
	    else
	    {
	      cnt_kills[i] = 100;
	    }

	    if (wbs->maxitems)
	    {
	      cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
	    }
	    else
	    {
	      cnt_items[i] = 100;
	    }

	    if (wbs->maxsecret)
	    {
	      cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;
	    }
	    else
	    {
	      cnt_secret[i] = 100;
	    }

	    if (dofrags)
	    {
		cnt_frags[i] = WI_fragSum(i);
	    }
	}
	S_StartSound(0, sfx_barexp);
	ng_state = 10;
    }

    switch (ng_state)
    {
      case 2:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!playeringame[i])
		continue;

	    cnt_kills[i] += 2;

	    if (wbs->maxkills)
	    {
	      fsum = (plrs[i].skills * 100) / wbs->maxkills;
	    }
	    else
	    {
	      fsum = 100;
	    }

	    if (cnt_kills[i] >= fsum)
		cnt_kills[i] = fsum;
	    else
		stillticking = true;
	}

	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ng_state++;
	}
	break;

      case 4:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!playeringame[i])
		continue;

	    cnt_items[i] += 2;

	    if (wbs->maxitems)
	    {
	      fsum = (plrs[i].sitems * 100) / wbs->maxitems;
	    }
	    else
	    {
	      fsum = 100;
	    }

	    if (cnt_items[i] >= fsum)
		cnt_items[i] = fsum;
	    else
		stillticking = true;
	}
	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ng_state++;
	}
	break;

      case 6:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!playeringame[i])
		continue;

	    cnt_secret[i] += 2;

	    if (wbs->maxsecret)
	    {
	      fsum = (plrs[i].ssecret * 100) / wbs->maxsecret;
	    }
	    else
	    {
	      fsum = 100;
	    }

	    if (cnt_secret[i] >= fsum)
		cnt_secret[i] = fsum;
	    else
		stillticking = true;
	}

	if (!stillticking)
	{
	    S_StartSound(0, sfx_barexp);
	    ng_state += 1 + 2*!dofrags;
	}
	break;

      case 8:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	stillticking = false;

	for (i=0 ; i<MAXPLAYERS ; i++)
	{
	    if (!playeringame[i])
		continue;

	    cnt_frags[i] += 1;

	    fsum = WI_fragSum(i);

	    if (cnt_frags[i] >= fsum)
		cnt_frags[i] = fsum;
	    else
		stillticking = true;
	}

	if (!stillticking)
	{
	    S_StartSound(0, sfx_pldeth);
	    ng_state++;
	}
	break;

      case 10:
	if (acceleratestage)
	{
	    S_StartSound(0, sfx_sgcock);
	    if ( gamemode == commercial )
		WI_initNoState();
	    else
		WI_initShowNextLoc();
	}
	break;

      default:
	if (!--cnt_pause)
	{
	    ng_state++;
	    cnt_pause = TICRATE;
	}
    }
}

/* ---------------------------------------------------------------------------- */

static void WI_drawNetgameStats(void)
{
    int		i;
    int		x;
    int		y;
    int		pwidth = SHORT(percent->width);

    if (bg_exit)
    {
      V_DrawPatchScaled(0, 0, 1, bg_exit);
      bg_exit = 0;
    }

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    WI_drawLF();

    // draw stat titles (top line)
    if (wbs->maxkills)
    {
      V_DrawPatchScaled(NG_STATSX+NG_SPACINGX-SHORT(kills->width),
		NG_STATSY, FB, kills);
    }

    V_DrawPatchScaled(NG_STATSX+2*NG_SPACINGX-SHORT(items->width),
		NG_STATSY, FB, items);

    if (wbs->maxsecret)
    {
      V_DrawPatchScaled(NG_STATSX+3*NG_SPACINGX-SHORT(secret->width),
		NG_STATSY, FB, secret);
    }

    if (dofrags)
	V_DrawPatchScaled(NG_STATSX+4*NG_SPACINGX-SHORT(frags->width),
		    NG_STATSY, FB, frags);

    // draw stats
    y = NG_STATSY + SHORT(kills->height);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	if (!playeringame[i])
	    continue;

	x = NG_STATSX;
	V_DrawPatchScaled(x-SHORT(p[i]->width), y, FB, p[i]);

	if (i == me)
	    V_DrawPatchScaled(x-SHORT(p[i]->width), y, FB, star);
	x += NG_SPACINGX;

	if (wbs->maxkills == 0)
	{
	  cnt_kills[i] = 100;
	}
	else
	{
	  WI_drawPercent(x-pwidth, y+10, cnt_kills[i]);
	}

	x += NG_SPACINGX;

	WI_drawPercent(x-pwidth, y+10, cnt_items[i]);
	x += NG_SPACINGX;

	if (wbs->maxsecret == 0)
	{
	  cnt_secret[i] = 100;
	}
	else
	{
	  WI_drawPercent(x-pwidth, y+10, cnt_secret[i]);
	}
	x += NG_SPACINGX;

	if (dofrags)
	    WI_drawNum(x, y+10, cnt_frags[i], -1);

	y += WI_SPACINGY;
    }

}

/* ---------------------------------------------------------------------------- */

static int	sp_state;

static void WI_initStats(void)
{
    state = StatCount;
    acceleratestage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = -1;
    cnt_pause = TICRATE;

    WI_initAnimatedBack();
}

/* ---------------------------------------------------------------------------- */

static void WI_updateStats(void)
{
    int i;

    WI_updateAnimatedBack();

    if (acceleratestage && sp_state != 10)
    {
	acceleratestage = 0;

	if (wbs->maxkills)
	{
	  cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
	}
	else
	{
	  cnt_kills[0] = 100;
	}

	if (wbs->maxitems)
	{
	  cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
	}
	else
	{
	  cnt_items[0] = 100;
	}

	if (wbs->maxsecret)
	{
	  cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
	}
	else
	{
	  cnt_secret[0] = 100;
	}

	cnt_time = plrs[me].stime / TICRATE;
	cnt_par = wbs->partime / TICRATE;
	S_StartSound(0, sfx_barexp);
	sp_state = 10;
    }

    switch (sp_state)
    {
      case 2:
	cnt_kills[0] += 2;

	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	if (wbs->maxkills)
	{
	  i = (plrs[me].skills * 100) / wbs->maxkills;
	}
	else
	{
	  i = 100;
	}

	if (cnt_kills[0] >= i)
	{
	    cnt_kills[0] = i;
	    S_StartSound(0, sfx_barexp);
	    sp_state++;
	}
	break;

      case 4:
	cnt_items[0] += 2;

	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	if (wbs->maxitems)
	{
	  i = (plrs[me].sitems * 100) / wbs->maxitems;
	}
	else
	{
	  i = 100;
	}

	if (cnt_items[0] >= i)
	{
	    cnt_items[0] = i;
	    S_StartSound(0, sfx_barexp);
	    sp_state++;
	}
	break;

      case 6:
	cnt_secret[0] += 2;

	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	if (wbs->maxsecret)
	{
	  i = (plrs[me].ssecret * 100) / wbs->maxsecret;
	}
	else
	{
	  i = 100;
	}

	if (cnt_secret[0] >= i)
	{
	  cnt_secret[0] = i;
	  S_StartSound(0, sfx_barexp);
	  sp_state++;
	}
	break;

      case 8:
	if (!(bcnt&3))
	    S_StartSound(0, sfx_pistol);

	cnt_time += 3;

	i = plrs[me].stime / TICRATE;
	if (cnt_time >= i)
	    cnt_time = i;

	cnt_par += 3;

	if (cnt_par >= wbs->partime / TICRATE)
	{
	    cnt_par = wbs->partime / TICRATE;

	    if (cnt_time >= plrs[me].stime / TICRATE)
	    {
		S_StartSound(0, sfx_barexp);
		sp_state++;
	    }
	}
	break;

      case 10:
	if (acceleratestage)
	{
	    S_StartSound(0, sfx_sgcock);

	    if (gamemode == commercial)
		WI_initNoState();
	    else
		WI_initShowNextLoc();
	}
	break;

      default:
	if (!--cnt_pause)
	{
	    sp_state++;
	    cnt_pause = TICRATE;
	}
    }

}

/* ---------------------------------------------------------------------------- */

static void WI_drawStats(void)
{
    // line height
    int lh;

    lh = (3*SHORT(num[0]->height))/2;

    if (bg_exit)
    {
      V_DrawPatchScaled(0, 0, 1, bg_exit);
      bg_exit = 0;
    }

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    WI_drawLF();

    if (wbs->maxkills == 0)
    {
      cnt_kills[0] = 100;
    }
    else
    {
      V_DrawPatchScaled(SP_STATSX, SP_STATSY, FB, kills);
      WI_drawPercent(WI_SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);
    }

    V_DrawPatchScaled(SP_STATSX, SP_STATSY+lh, FB, items);
    WI_drawPercent(WI_SCREENWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);

    if (wbs->maxsecret == 0)
    {
      cnt_secret[0] = 100;
    }
    else
    {
      V_DrawPatchScaled(SP_STATSX, SP_STATSY+2*lh, FB, sp_secret);
      WI_drawPercent(WI_SCREENWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);
    }

    V_DrawPatchScaled(SP_TIMEX, SP_TIMEY, FB, wtime);
    WI_drawTime(WI_SCREENWIDTH/2 - (SP_TIMEX - ((WI_SCREENWIDTH/2)-160)), SP_TIMEY, cnt_time, wbs->timesucks);

    if (wbs->partime)
    {
      V_DrawPatchScaled(WI_SCREENWIDTH/2 + (SP_TIMEX - ((WI_SCREENWIDTH/2)-160)), SP_TIMEY, FB, par);
      WI_drawTime(WI_SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par, MAXINT);
    }
}

/* ---------------------------------------------------------------------------- */

int WI_checkForAccelerate (void)
{
    int   i;
    int   rc;
    player_t  *player;

    rc = 0;

    // check for button presses to skip delays
    for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
    {
	if (playeringame[i])
	{
	    if (player->cmd.buttons & BT_ATTACK)
	    {
		if (!player->attackdown)
		    rc = 1;
		player->attackdown = true;
	    }
	    else
		player->attackdown = false;
	    if (player->cmd.buttons & BT_USE)
	    {
		if (!player->usedown)
		    rc = 1;
		player->usedown = true;
	    }
	    else
		player->usedown = false;
	}
    }

  return (rc);
}

/* ---------------------------------------------------------------------------- */

// Updates stuff each tick
void WI_Ticker(void)
{
    // counter for general background animation
    bcnt++;

    if (bcnt == 1)
    {
	// intermission music
  	if ( gamemode == commercial )
	  S_ChangeMusic(mus_dm2int, true);
	else
	  S_ChangeMusic(mus_inter, true);
    }

    if (WI_checkForAccelerate())
      acceleratestage = 1;

    switch (state)
    {
      case StatCount:
	if (deathmatch) WI_updateDeathmatchStats();
	else if (netgame) WI_updateNetgameStats();
	else WI_updateStats();
	break;

      case ShowNextLoc:
	WI_updateShowNextLoc();
	break;

      case NoState:
	WI_updateNoState();
	break;
    }
}

/* ---------------------------------------------------------------------------- */

static patch_t * WI_levelpatch (int episode, int level, const char * patchname)
{
  int i;
  int patch;

  if (((patch = W_CheckNumForName (patchname)) == -1)
   || ((i = G_MapLump (episode,level)) == -1)
   || (W_SameWadfile (i, patch) == 0))
    return (NULL);

  return (W_CacheLumpNum (patch, PU_STATIC));
}

/* ---------------------------------------------------------------------------- */

static char * WI_GetMapName (int epi, int map)
{
  int i;
  char * s;

  /* If we are playing a map in a PWAD and the */
  /* mapname hasn't been changed then don't show */
  /* the out of date name. */

  i = G_MapLump (epi, map);
  if ((i != -1)
   && (lumpinfo[i].handle != lumpinfo[0].handle)
   && (mapnameschanged == false))
  {
    return (NULL);
  }
  else
  {
    s = *(HU_access_mapname (epi, map));

    if ((s == NULL) || (s[0] == 0))
      return (NULL);
  }

  i = dh_inchar (s, ':');
  if ((i) && (i < 10))
  {
    s += i;
    while (*s == ' ') s++;
  }

  return (s);
}

/* -------------------------------------------------------------------------------------------- */

static void WI_loadData (void)
{
    int		i;
    int		j;
    char	name[10];
    anim_t*	a;
    map_dests_t * mdest_ptr;

#if 0
    if (gamemode == commercial)
	strcpy(name, "INTERPIC");
    else
	sprintf(name, "WIMAP%d", wbs->epsd);

    if ( gamemode == retail )
    {
      if (wbs->epsd == 3)
	strcpy(name,"INTERPIC");
    }

    // background
    if (W_CheckNumForName (name) == -1)
	strcpy(name,"INTERPIC");

    bg = W_CacheLumpName(name, PU_CACHE);
    V_DrawPatchScaled(0, 0, 1, bg);
#endif

    // UNUSED unsigned char *pic = screens[1];
    // if (gamemode == commercial)
    // {
    // darken the background image
    // while (pic != screens[1] + SCREENHEIGHT*SCREENWIDTH)
    // {
    //   *pic = colormaps[256*25 + *pic];
    //   pic++;
    // }
    //}

    ltext_last = WI_GetMapName (gameepisode, wbs->last+1);
    ltext_next = WI_GetMapName (gameepisode, wbs->next+1);

    if (gamemode == commercial)
    {
	mdest_ptr = G_Access_MapInfoTab (255, wbs->last+1);
	sprintf (name, mdest_ptr -> titlepatch, wbs->last);
	lpatch_last = WI_levelpatch (255,wbs->last+1, name);

	sprintf (name, mdest_ptr -> exitpic, wbs->last);
	bg_exit = W_CacheLumpName0 (name, PU_CACHE);

	mdest_ptr = G_Access_MapInfoTab (255, wbs->next+1);
	sprintf (name, mdest_ptr -> titlepatch, wbs->next);
	lpatch_next = WI_levelpatch (255,wbs->next+1, name);

	sprintf (name, mdest_ptr -> enterpic, wbs->next);
	bg_enter = W_CacheLumpName0 (name, PU_CACHE);
    }
    else
    {
	mdest_ptr = G_Access_MapInfoTab (gameepisode, wbs->last+1);
	sprintf (name, mdest_ptr -> titlepatch, gameepisode-1, wbs->last);
	lpatch_last = WI_levelpatch (gameepisode,wbs->last+1, name);

	sprintf (name, mdest_ptr -> exitpic, wbs->epsd);
	bg_exit = W_CacheLumpName0 (name, PU_CACHE);

	mdest_ptr = G_Access_MapInfoTab (wbs->epsd+1, wbs->next+1);
	sprintf (name, mdest_ptr -> titlepatch, wbs->epsd, wbs->next);
	lpatch_next = WI_levelpatch (wbs->epsd+1,wbs->next+1, name);

	sprintf (name, mdest_ptr -> enterpic, wbs->epsd);
	bg_enter = W_CacheLumpName0 (name, PU_CACHE);

	// you are here
	yah[0] = W_CacheLumpName("WIURH0", PU_STATIC);

	// you are here (alt.)
	yah[1] = W_CacheLumpName("WIURH1", PU_STATIC);

	// splat
	splat = W_CacheLumpName("WISPLAT", PU_STATIC);

	if (wbs->epsd < ARRAY_SIZE(anims))
	{
	    for (j=0;j<NUMANIMS[wbs->epsd];j++)
	    {
		a = &anims[wbs->epsd][j];
		for (i=0;i<a->nanims;i++)
		{
		    // MONDO HACK!
		    if (wbs->epsd != 1 || j != 8)
		    {
			// animations
			sprintf(name, "WIA%d%.2d%.2d", wbs->epsd, j, i);
			a->p[i] = W_CacheLumpName(name, PU_STATIC);
		    }
		    else
		    {
			// HACK ALERT!
			a->p[i] = anims[1][4].p[i];
		    }
		}
	    }
	}
    }

    if (bg_exit == 0)
      bg_exit = W_CacheLumpName0 ("INTERPIC", PU_CACHE);

    if (bg_enter == 0)
      bg_enter = W_CacheLumpName0 ("INTERPIC", PU_CACHE);

    // More hacks on minus sign.
    wiminus = W_CacheLumpName("WIMINUS", PU_STATIC);

    for (i=0;i<10;i++)
    {
	 // numbers 0-9
	sprintf(name, "WINUM%d", i);
	num[i] = W_CacheLumpName(name, PU_STATIC);
    }

    // percent sign
    percent = W_CacheLumpName("WIPCNT", PU_STATIC);

    // "finished"
    finished = W_CacheLumpName("WIF", PU_STATIC);

    // "entering"
    entering = W_CacheLumpName("WIENTER", PU_STATIC);

    // "kills"
    kills = W_CacheLumpName("WIOSTK", PU_STATIC);

    // "scrt"
    secret = W_CacheLumpName("WIOSTS", PU_STATIC);

     // "secret"
    sp_secret = W_CacheLumpName("WISCRT2", PU_STATIC);

    // Yuck.
    if (language == french)
    {
	// "items"
	if (netgame && !deathmatch)
	    items = W_CacheLumpName("WIOBJ", PU_STATIC);
  	else
	    items = W_CacheLumpName("WIOSTI", PU_STATIC);
    } else
	items = W_CacheLumpName("WIOSTI", PU_STATIC);

    // "frgs"
    frags = W_CacheLumpName("WIFRGS", PU_STATIC);

    // ":"
    colon = W_CacheLumpName("WICOLON", PU_STATIC);

    // "time"
    wtime = W_CacheLumpName("WITIME", PU_STATIC);

    // "sucks"
    sucks = W_CacheLumpName("WISUCKS", PU_STATIC);

    // "par"
    par = W_CacheLumpName("WIPAR", PU_STATIC);

    // "killers" (vertical)
    killers = W_CacheLumpName("WIKILRS", PU_STATIC);

    // "victims" (horiz)
    victims = W_CacheLumpName("WIVCTMS", PU_STATIC);

    // "total"
    total = W_CacheLumpName("WIMSTT", PU_STATIC);

    // your face
    star = W_CacheLumpName("STFST01", PU_STATIC);

    // dead face
    bstar = W_CacheLumpName("STFDEAD0", PU_STATIC);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	// "1,2,3,4"
	sprintf(name, "STPB%d", i);
	p[i] = W_CacheLumpName(name, PU_STATIC);

	// "1,2,3,4"
	sprintf(name, "WIBP%d", i+1);
	bp[i] = W_CacheLumpName(name, PU_STATIC);
    }

}

/* ---------------------------------------------------------------------------- */

static void WI_unloadData(void)
{
    int		i;
    int		j;

    Z_ChangeTag(wiminus, PU_CACHE);

    for (i=0 ; i<10 ; i++)
	Z_ChangeTag(num[i], PU_CACHE);

    if (lpatch_last)
      Z_ChangeTag(lpatch_last, PU_CACHE);

    if (lpatch_next)
      Z_ChangeTag(lpatch_next, PU_CACHE);

    if (bg_exit)
      Z_ChangeTag(bg_exit, PU_CACHE);

    if (bg_enter)
      Z_ChangeTag(bg_enter, PU_CACHE);

    if (gamemode != commercial)
    {
	Z_ChangeTag(yah[0], PU_CACHE);
	Z_ChangeTag(yah[1], PU_CACHE);

	Z_ChangeTag(splat, PU_CACHE);

	if (wbs->epsd < ARRAY_SIZE(anims))
	{
	    for (j=0;j<NUMANIMS[wbs->epsd];j++)
	    {
		if (wbs->epsd != 1 || j != 8)
		    for (i=0;i<anims[wbs->epsd][j].nanims;i++)
			Z_ChangeTag(anims[wbs->epsd][j].p[i], PU_CACHE);
	    }
	}
    }

    Z_ChangeTag(percent, PU_CACHE);
    Z_ChangeTag(colon, PU_CACHE);
    Z_ChangeTag(finished, PU_CACHE);
    Z_ChangeTag(entering, PU_CACHE);
    Z_ChangeTag(kills, PU_CACHE);
    Z_ChangeTag(secret, PU_CACHE);
    Z_ChangeTag(sp_secret, PU_CACHE);
    Z_ChangeTag(items, PU_CACHE);
    Z_ChangeTag(frags, PU_CACHE);
    Z_ChangeTag(wtime, PU_CACHE);
    Z_ChangeTag(sucks, PU_CACHE);
    Z_ChangeTag(par, PU_CACHE);

    Z_ChangeTag(victims, PU_CACHE);
    Z_ChangeTag(killers, PU_CACHE);
    Z_ChangeTag(total, PU_CACHE);
    //  Z_ChangeTag(star, PU_CACHE);
    //  Z_ChangeTag(bstar, PU_CACHE);

    for (i=0 ; i<MAXPLAYERS ; i++)
	Z_ChangeTag(p[i], PU_CACHE);

    for (i=0 ; i<MAXPLAYERS ; i++)
	Z_ChangeTag(bp[i], PU_CACHE);
}

/* ---------------------------------------------------------------------------- */

void WI_Drawer (void)
{
    switch (state)
    {
      case StatCount:
	if (deathmatch)
	    WI_drawDeathmatchStats();
	else if (netgame)
	    WI_drawNetgameStats();
	else
	    WI_drawStats();
	break;

      case ShowNextLoc:
	WI_drawShowNextLoc();
	break;

      case NoState:
	WI_drawNoState();
	break;
    }
}

/* ---------------------------------------------------------------------------- */

static void WI_initVariables(wbstartstruct_t* wbstartstruct)
{
  wbs = wbstartstruct;

#ifdef RANGECHECKING
  if (gamemode != commercial)
  {
    if ( gamemode == retail )
      RNGCHECK(wbs->epsd, 0, 3);
    else
      RNGCHECK(wbs->epsd, 0, 2);
  }
  else
  {
    RNGCHECK(wbs->last, 0, 8);
    RNGCHECK(wbs->next, 0, 8);
  }
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

  acceleratestage = 0;
  cnt = bcnt = 0;
  firstrefresh = 1;
  me = wbs->pnum;
  plrs = wbs->plyr;

  // if (!wbs->maxkills)			// ID cludges to avoid divide by zero
  //  wbs->maxkills = 1;			// but they make the counts wrong...

  // if (!wbs->maxitems)			// ie 0% instead of 100%
  //  wbs->maxitems = 1;

  // if (!wbs->maxsecret)
  //  wbs->maxsecret = 1;

  // if ((gamemode != retail )
  //  && (wbs->epsd > 2))
  //   wbs->epsd -= 3;
}

/* ---------------------------------------------------------------------------- */

void WI_Start(wbstartstruct_t* wbstartstruct)
{
  WI_initVariables (wbstartstruct);
  WI_loadData ();

  if (deathmatch)
    WI_initDeathmatchStats();
  else if (netgame)
    WI_initNetgameStats();
  else
    WI_initStats();
}

/* ---------------------------------------------------------------------------- */
