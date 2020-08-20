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
//
// $Log:$
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------

// Note: Much of this code is courtesy of DIY doom.

//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: am_map.c,v 1.4 1997/02/03 21:24:33 b1 Exp $";
#endif

#include "includes.h"

typedef unsigned char pixel_t;
extern const unsigned char PLAYPAL [3*256];

/* -------------------------------------------------------------- */

char * am_map_messages_orig [] =
{
  AMSTR_FOLLOWON,
  AMSTR_FOLLOWOFF,
  AMSTR_GRIDON,
  AMSTR_GRIDOFF,
  AMSTR_MARKEDSPOT,
  AMSTR_MARKSCLEARED,
  NULL
};

char * am_map_messages [ARRAY_SIZE(am_map_messages_orig)];

typedef enum
{
  AM_AMSTR_FOLLOWON,
  AM_AMSTR_FOLLOWOFF,
  AM_AMSTR_GRIDON,
  AM_AMSTR_GRIDOFF,
  AM_AMSTR_MARKEDSPOT,
  AM_AMSTR_MARKSCLEARED
} am_map_texts_t;

/* ---------------------------------------------------------------------- */

typedef struct
{
  unsigned char back;
  unsigned char grid;
  unsigned char wall;
  unsigned char fchg;
  unsigned char cchg;
  unsigned char clsd;
  unsigned char rcard;
  unsigned char bcard;
  unsigned char ycard;
  unsigned char rskull;
  unsigned char bskull;
  unsigned char yskull;
  unsigned char rcarddoor;
  unsigned char bcarddoor;
  unsigned char ycarddoor;
  unsigned char rskulldoor;
  unsigned char bskulldoor;
  unsigned char yskulldoor;
  unsigned char tele;
  unsigned char secr;
  unsigned char exit;
  unsigned char unsn;
  unsigned char flat;
  unsigned char sprt;
  unsigned char hair;
  unsigned char sngl;
  unsigned char plyr[4];
  unsigned char plyr_invis;
  unsigned char frnd;
  unsigned char monster;
  unsigned char ammo;
  unsigned char body;
  unsigned char dead;
  unsigned char bonus;
  unsigned char special;
  unsigned char pillar;
  unsigned char tree;
  unsigned char light;
  unsigned char blood;
  unsigned char skull;
} mapcolour_t;

static mapcolour_t mapcolour =
{
  247,		// back		map background
  104,		// grid		grid lines colour
  181,		// wall		normal 1s wall colour (23)
  64,		// fchg		line at floor height change colour
  162,		// cchg		line at ceiling height change colour
  152,		// clsd		line at sector with floor=ceiling colour
  175,		// rcard	red card colour
  204,		// bcard	blue card colour
  231,		// ycard	yellow card colour
  175,		// rskull	red skull colour
  204,		// bskull	blue skull colour
  231,		// yskull	yellow skull colour
  175,		// rkeydoor	red door colour  (diff from keys to allow option)
  204,		// bkeydoor	blue door colour (of enabling one but not other)
  231,		// ykeydoor	yellow door colour
  175,		// rskulldoor	red door colour
  204,		// bskulldoor	blue door colour
  231,		// yskulldoor	yellow door colour
  119,		// tele		teleporter line colour
  252,		// secr		secret sector boundary colour
  112,		// exit		jff 4/23/98 add exit line colour
  104,		// unsn		computer map unseen line colour
  88,		// flat		line with no floor/ceiling changes
  112,		// sprt		general sprite colour
  208,		// hair		crosshair colour
  208,		// sngl		single player arrow colour
  112,		// plyr_0
  88,		// plyr_1
  64,		// plyr_2
  176,		// plyr_3
  246,		// plyr_invis
  252,		// friends of player
  216,		// monster
  200,		// ammo
  190,		// body
  190,		// dead
  64,		// bonus
  250,		// special
  76,		// pillar
  76,		// tree
  190,		// light
  188,		// blood
  188		// skull
};

/* ---------------------------------------------------------------------- */
// drawing stuff
#define FB	      0

#define AM_PANDOWNKEY   KEY_DOWNARROW
#define AM_PANUPKEY     KEY_UPARROW
#define AM_PANRIGHTKEY  KEY_RIGHTARROW
#define AM_PANLEFTKEY   KEY_LEFTARROW
#define AM_ZOOMINKEY1   '='
#define AM_ZOOMINKEY2   '+'
#define AM_ZOOMOUTKEY   '-'
#define AM_STARTKEY     KEY_TAB
#define AM_ENDKEY       KEY_TAB
#define AM_GOBIGKEY     '0'
#define AM_FOLLOWKEY    'f'
#define AM_GRIDKEY      'g'
#define AM_MARKKEY      'm'
#define AM_CLEARMARKKEY 'c'

#define AM_NUMMARKPOINTS 10

// scale on entry
#define INITSCALEMTOF	(.2*FRACUNIT)
// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC	4
// how much zoom-in per tic
// goes to 2x in 1 second
#define M_ZOOMIN	((int) (1.02*FRACUNIT))
// how much zoom-out per tic
// pulls out to 0.5x in 1 second
#define M_ZOOMOUT       ((int) (FRACUNIT/1.02))

// translates between frame-buffer and map distances
#define FTOM(x) FixedMul(((x)<<16),scale_ftom)
#define MTOF(x) (FixedMul((x),scale_mtof)>>16)
// translates between frame-buffer and map coordinates
#define CXMTOF(x)  (f_x + MTOF((x)-m_x))
#define CYMTOF(y)  (f_y + (f_h - MTOF((y)-m_y)))

//-----------------------------------------------------------------------------

typedef struct
{
    int x, y;
} fpoint_t;

typedef struct
{
    fpoint_t a, b;
} fline_t;

typedef struct
{
    fixed_t	     x,y;
} mpoint_t;

typedef struct
{
    mpoint_t a, b;
} mline_t;

typedef struct
{
    fixed_t slp, islp;
} islope_t;

typedef struct
{
  int numlines;
  const mline_t *shape;
} mshape_t;


//-----------------------------------------------------------------------------
//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8*PLAYERRADIUS)/7)
static const mline_t player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/4 } },  // ----->
    { { R, 0 }, { R-R/2, -R/4 } },
    { { -R+R/8, 0 }, { -R-R/8, R/4 } }, // >---->
    { { -R+R/8, 0 }, { -R-R/8, -R/4 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/4 } }, // >>--->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/4 } }
};
#undef R
#define NUMPLYRLINES (sizeof(player_arrow)/sizeof(mline_t))

#define R ((8*PLAYERRADIUS)/7)
static const mline_t cheat_player_arrow[] = {
    { { -R+R/8, 0 }, { R, 0 } }, // -----
    { { R, 0 }, { R-R/2, R/6 } },  // ----->
    { { R, 0 }, { R-R/2, -R/6 } },
    { { -R+R/8, 0 }, { -R-R/8, R/6 } }, // >----->
    { { -R+R/8, 0 }, { -R-R/8, -R/6 } },
    { { -R+3*R/8, 0 }, { -R+R/8, R/6 } }, // >>----->
    { { -R+3*R/8, 0 }, { -R+R/8, -R/6 } },
    { { -R/2, 0 }, { -R/2, -R/6 } }, // >>-d--->
    { { -R/2, -R/6 }, { -R/2+R/6, -R/6 } },
    { { -R/2+R/6, -R/6 }, { -R/2+R/6, R/4 } },
    { { -R/6, 0 }, { -R/6, -R/6 } }, // >>-dd-->
    { { -R/6, -R/6 }, { 0, -R/6 } },
    { { 0, -R/6 }, { 0, R/4 } },
    { { R/6, R/4 }, { R/6, -R/7 } }, // >>-ddt->
    { { R/6, -R/7 }, { R/6+R/32, -R/7-R/32 } },
    { { R/6+R/32, -R/7-R/32 }, { R/6+R/10, -R/7 } }
};
#undef R
#define NUMCHEATPLYRLINES (sizeof(cheat_player_arrow)/sizeof(mline_t))
#if 0
#define R (FRACUNIT)
static const mline_t triangle_guy[] = {
    { { (fixed_t)(-.867*R), (fixed_t)(-.5*R) }, { (fixed_t)(.867*R), (fixed_t)(-.5*R) } },
    { { (fixed_t)(.867*R), (fixed_t)(-.5*R) } , { (fixed_t)0, (fixed_t)R } },
    { { (fixed_t)0, (fixed_t)R }, { (fixed_t)(-.867*R), (fixed_t)(-.5*R) } }
};
#undef R
#define NUMTRIANGLEGUYLINES (sizeof(triangle_guy)/sizeof(mline_t))
#endif
#define R (FRACUNIT)
static const mline_t thintriangle_guy[] = {
    { { (fixed_t)(-.5*R), (fixed_t)(-.7*R) }, { (fixed_t)R, (fixed_t)0 } },
    { { (fixed_t)R, (fixed_t)0 }, { (fixed_t)(-.5*R), (fixed_t)(.7*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(.7*R) }, { (fixed_t)(-.5*R), (fixed_t)(-.7*R) } }
};
#undef R
#define NUMTHINTRIANGLEGUYLINES (sizeof(thintriangle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t pentacle_guy[] = {
    { { (fixed_t)0, (fixed_t)R }, { (fixed_t)(.588*R), (fixed_t)(-.809*R) } },
    { { (fixed_t)(.588*R), (fixed_t)(-.809*R) }, { (fixed_t)(-.951*R), (fixed_t)(.309*R) }  },
    { { (fixed_t)(-.951*R), (fixed_t)(.309*R) }, { (fixed_t)(.951*R), (fixed_t)(.309*R) } },
    { { (fixed_t)(.951*R), (fixed_t)(.309*R) }, { (fixed_t)(-.588*R), (fixed_t)(-.809*R) } },
    { { (fixed_t)(-.588*R), (fixed_t)(-.809*R) }, { (fixed_t)0, (fixed_t)R }  }
};
#undef R
#define NUMPENTACLEGUYLINES (sizeof(pentacle_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t hexagon_guy[] = {
    { { (fixed_t)(.5*R), (fixed_t)(.866*R) }, { (fixed_t)R, (fixed_t)0 } },
    { { (fixed_t)R, (fixed_t)0 }, { (fixed_t)(.5*R), (fixed_t)(-.866*R) } },
    { { (fixed_t)(.5*R), (fixed_t)(-.866*R) }, { (fixed_t)(-.5*R), (fixed_t)(-.866*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(-.866*R) }, { (fixed_t)(-R), (fixed_t)0 } },
    { { (fixed_t)(-R), (fixed_t)0 }, { (fixed_t)(-.5*R), (fixed_t)(.866*R) } },
    { { (fixed_t)(-.5*R), (fixed_t)(.866*R) }, { (fixed_t)(.5*R), (fixed_t)(.866*R) } }
};
#undef R
#define NUMHEXAGONGUYLINES (sizeof(hexagon_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t keyshaped_guy[] = {
    { { (fixed_t)(-.6*R), (fixed_t)(.4*R) }, { (fixed_t)(-.3*R), (fixed_t)(.2*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(.2*R) }, { (fixed_t)(-.3*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.6*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.6*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.9*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.9*R), (fixed_t)(.2*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(.2*R) }, { (fixed_t)(-.6*R), (fixed_t)(.4*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(0*R) }, { (fixed_t)(.9*R), (fixed_t)(0*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(0*R) }, { (fixed_t)(.9*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(-.4*R) }, { (fixed_t)(.6*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.6*R), (fixed_t)(-.2*R) }, { (fixed_t)(.3*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.3*R), (fixed_t)(-.4*R) }, { (fixed_t)(.3*R), (fixed_t)(0*R) } }
};
#undef R
#define NUMKEYSHAPEDGUYLINES (sizeof(keyshaped_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t skullkeyshaped_guy[] = {
    { { (fixed_t)(.6*R), (fixed_t)(.4*R) }, { (fixed_t)(.3*R), (fixed_t)(.1*R) } },
    { { (fixed_t)(.3*R), (fixed_t)(.1*R) }, { (fixed_t)(.4*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.4*R), (fixed_t)(-.2*R) }, { (fixed_t)(.4*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.4*R), (fixed_t)(-.4*R) }, { (fixed_t)(.8*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(.8*R), (fixed_t)(-.4*R) }, { (fixed_t)(.8*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(.8*R), (fixed_t)(-.2*R) }, { (fixed_t)(.9*R), (fixed_t)(.1*R) } },
    { { (fixed_t)(.9*R), (fixed_t)(.1*R) }, { (fixed_t)(.6*R), (fixed_t)(.4*R) } },
    { { (fixed_t)(1.0/3*R), (fixed_t)0 }, { (fixed_t)(-.9*R), (fixed_t)0 } },
    { { (fixed_t)(-.9*R), (fixed_t)0 }, { (fixed_t)(-.9*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.9*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.6*R), (fixed_t)(-.2*R) } },
    { { (fixed_t)(-.6*R), (fixed_t)(-.2*R) }, { (fixed_t)(-.3*R), (fixed_t)(-.4*R) } },
    { { (fixed_t)(-.3*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.3*R), (fixed_t)0 } }
};
#undef R
#define NUMSKULLKEYSHAPEDGUYLINES (sizeof(skullkeyshaped_guy)/sizeof(mline_t))

#define R (FRACUNIT)
static const mline_t musicshaped_guy[] =
{
  {{ (fixed_t)(.2*R), (fixed_t)(.2*R) }, { (fixed_t)(.0*R), (fixed_t)(.4*R) }},
  {{ (fixed_t)(.0*R), (fixed_t)(.4*R) }, { (fixed_t)(.0*R), (fixed_t)(-.4*R) }},
  {{ (fixed_t)(.0*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.2*R), (fixed_t)(-.4*R) }},
  {{ (fixed_t)(-.2*R), (fixed_t)(-.4*R) }, { (fixed_t)(-.2*R), (fixed_t)(-.2*R) }},
  {{ (fixed_t)(-.2*R), (fixed_t)(-.2*R) }, { (fixed_t)(.0*R), (fixed_t)(-.2*R) }}
};
#undef R
#define NUMMUSICSHAPEDGUYLINES (sizeof(musicshaped_guy)/sizeof(mline_t))


static const mshape_t thintriangle_shape = { NUMTHINTRIANGLEGUYLINES, thintriangle_guy };
static const mshape_t pentacle_shape = { NUMPENTACLEGUYLINES, pentacle_guy };
static const mshape_t hexagon_shape = { NUMHEXAGONGUYLINES, hexagon_guy };
static const mshape_t key_shape = { NUMKEYSHAPEDGUYLINES, keyshaped_guy };
static const mshape_t skullkey_shape = { NUMSKULLKEYSHAPEDGUYLINES, skullkeyshaped_guy };
static const mshape_t music_shape = { NUMMUSICSHAPEDGUYLINES, musicshaped_guy };


static int      grid = 0;

static int      leveljuststarted = 1;	// kluge until AM_LevelInit() is called

boolean	 automapactive = false;

// location of window on screen
static int      f_x;
static int      f_y;

// size of window on screen
static int      f_w;
static int      f_h;

//static int	lightlev;		// used for funky strobing effect
static pixel_t*	fb;			// pseudo-frame buffer
static int	amclock;

static mpoint_t m_paninc;		// how far the window pans each tic (map coords)
static fixed_t  mtof_zoommul;		// how far the window zooms in each tic (map coords)
static fixed_t  ftom_zoommul;		// how far the window zooms in each tic (fb coords)

static fixed_t  m_x, m_y;		// LL x,y where the window is on the map (map coords)
static fixed_t  m_x2, m_y2;		// UR x,y where the window is on the map (map coords)

//
// width/height of window on map (map coords)
//
static fixed_t  m_w;
static fixed_t  m_h;

// based on level size
static fixed_t  min_x;
static fixed_t  min_y;
static fixed_t  max_x;
static fixed_t  max_y;

static fixed_t  max_w;		// max_x-min_x,
static fixed_t  max_h;		// max_y-min_y

// based on player size
static fixed_t  min_w;
static fixed_t  min_h;


static fixed_t  min_scale_mtof;	// used to tell when to stop zooming out
static fixed_t  max_scale_mtof; // used to tell when to stop zooming in

// old stuff for recovery later
static fixed_t old_m_w, old_m_h;
static fixed_t old_m_x, old_m_y;

// old location used by the Follower routine
static mpoint_t f_oldloc;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t scale_mtof = (fixed_t) INITSCALEMTOF;
// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t scale_ftom;

static player_t *plr;		// the player represented by an arrow

static patch_t *marknums[10];	// numbers used for marking by the automap
static mpoint_t markpoints[AM_NUMMARKPOINTS]; // where the points are
static int markpointnum = 0;	// next point to be assigned

static int followplayer = 1;	// specifies whether to follow the player around

unsigned char cheat_amap_seq[] = { 0xb2, 0x26, 0x26, 0x2e, 0xff };
static cheatseq_t cheat_amap = { cheat_amap_seq, 0 };

static boolean stopped = true;

extern boolean viewactive;
//extern byte screens[][SCREENWIDTH*SCREENHEIGHT];

static int ddt_cheating;
static int d_cheating;
static int k_cheating;

/* ---------------------------------------------------------------------- */
/*
   Find the best match in the palette table for this colour
*/

unsigned char AM_load_colour (unsigned char r1, unsigned char g1, unsigned char b1, const byte * palette)
{
  unsigned char colnum;
  unsigned char rc;
  unsigned int best;
  unsigned int rmean;
  unsigned int r,g,b;
  unsigned int r2,g2,b2;
  unsigned int diff;

  best = ~0;
  colnum = 0;
  rc = 0;

  do
  {
    r2 = *palette++;
    g2 = *palette++;
    b2 = *palette++;

    // From http://www.compuphase.com/cmetric.htm
    rmean = (r1 + r2) / 2;
    r = r1 - r2;
    g = g1 - g2;
    b = b1 - b2;
    diff = (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8);

    if (diff <= best)		// Use <= to find highest numbered....
    {
      best = diff;
      rc = colnum;
    }
  } while (++colnum);

  return (rc);
}

/* ---------------------------------------------------------------------- */

void AM_LoadColours (int palette_lump)
{
  unsigned int count;
  unsigned char * colnum;
  unsigned char * palette;
  const byte * q;

  palette = W_CacheLumpNum (palette_lump, PU_STATIC);
  if (memcmp (PLAYPAL, palette, sizeof (PLAYPAL)))
  {
    colnum = &mapcolour.back;
    count = sizeof(mapcolour);
    do
    {
      q = &PLAYPAL [*colnum * 3];
      *colnum++ = AM_load_colour (q[0], q[1], q[2], palette);
    } while (--count);
  }
}

/* ---------------------------------------------------------------------- */
// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.
//
#if 0
static void
AM_getIslope
( const mline_t* ml,
  islope_t*     is )
{
    int dx, dy;

    dy = ml->a.y - ml->b.y;
    dx = ml->b.x - ml->a.x;
    if (!dy) is->islp = (dx<0?-MAXINT:MAXINT);
    else is->islp = FixedDiv(dx, dy);
    if (!dx) is->slp = (dy<0?-MAXINT:MAXINT);
    else is->slp = FixedDiv(dy, dx);

}
#endif
/* ---------------------------------------------------------------------- */
//
//
//
static void AM_activateNewScale(void)
{
    m_x += m_w/2;
    m_y += m_h/2;
    m_w = FTOM(f_w);
    m_h = FTOM(f_h);
    m_x -= m_w/2;
    m_y -= m_h/2;
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_saveScaleAndLoc(void)
{
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;
}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_restoreScaleAndLoc(void)
{

    m_w = old_m_w;
    m_h = old_m_h;
    if (!followplayer)
    {
	m_x = old_m_x;
	m_y = old_m_y;
    } else {
	m_x = plr->mo->x - m_w/2;
	m_y = plr->mo->y - m_h/2;
    }
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;

    // Change the scaling multipliers
    scale_mtof = FixedDiv(f_w<<FRACBITS, m_w);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

/* ---------------------------------------------------------------------- */
//
// adds a marker at the current location
//
static void AM_addMark(void)
{
    markpoints[markpointnum].x = m_x + m_w/2;
    markpoints[markpointnum].y = m_y + m_h/2;
    markpointnum = (markpointnum + 1) % AM_NUMMARKPOINTS;

}

/* ---------------------------------------------------------------------- */
//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
static void AM_findMinMaxBoundaries(void)
{
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y =  MAXINT;
    max_x = max_y = -MAXINT;

    for (i=0;i<numvertexes;i++)
    {
	if (vertexes[i].x < min_x)
	    min_x = vertexes[i].x;
	else if (vertexes[i].x > max_x)
	    max_x = vertexes[i].x;

	if (vertexes[i].y < min_y)
	    min_y = vertexes[i].y;
	else if (vertexes[i].y > max_y)
	    max_y = vertexes[i].y;
    }

    max_w = max_x - min_x;
    max_h = max_y - min_y;

    min_w = 2*PLAYERRADIUS;		// const? never changed?
    min_h = 2*PLAYERRADIUS;

    a = FixedDiv(f_w<<FRACBITS, max_w);
    b = FixedDiv(f_h<<FRACBITS, max_h);

    min_scale_mtof = a < b ? a : b;
    max_scale_mtof = FixedDiv(f_h<<FRACBITS, 2*PLAYERRADIUS);

}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_changeWindowLoc(void)
{
    if (m_paninc.x || m_paninc.y)
    {
	followplayer = 0;
	f_oldloc.x = MAXINT;
    }

    m_x += m_paninc.x;
    m_y += m_paninc.y;

    if (m_x + m_w/2 > max_x)
	m_x = max_x - m_w/2;
    else if (m_x + m_w/2 < min_x)
	m_x = min_x - m_w/2;

    if (m_y + m_h/2 > max_y)
	m_y = max_y - m_h/2;
    else if (m_y + m_h/2 < min_y)
	m_y = min_y - m_h/2;

    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_initVariables(void)
{
    int pnum;

    automapactive = true;
    fb = screens[0];

    f_oldloc.x = MAXINT;
    amclock = 0;
    // lightlev = 0;

    m_paninc.x = m_paninc.y = 0;
    ftom_zoommul = FRACUNIT;
    mtof_zoommul = FRACUNIT;

    m_w = FTOM(f_w);
    m_h = FTOM(f_h);

    // find player to center on initially
    if (!playeringame[pnum = consoleplayer])
	for (pnum=0;pnum<MAXPLAYERS;pnum++)
	    if (playeringame[pnum])
		break;

    plr = &players[pnum];

    plr -> message =
	HU_printf ("Monsters killed = %d of %d. Secrets = %d of %d",
		plr -> killcount, totalkills, plr -> secretcount, totalsecret);

    m_x = plr->mo->x - m_w/2;
    m_y = plr->mo->y - m_h/2;
    AM_changeWindowLoc();

    // for saving & restoring
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;

    // inform the status bar of the change
    ST_AutoMapEvent (AM_MSGENTERED);
}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_loadPics(void)
{
    int i;
    char namebuf[9];

    for (i=0;i<10;i++)
    {
	sprintf(namebuf, "AMMNUM%d", i);
	marknums[i] = W_CacheLumpName(namebuf, PU_STATIC);
    }

}

/* ---------------------------------------------------------------------- */

static void AM_unloadPics(void)
{
    int i;

    for (i=0;i<10;i++)
	Z_ChangeTag(marknums[i], PU_CACHE);

}

/* ---------------------------------------------------------------------- */

static void AM_clearMarks(void)
{
    int i;

    for (i=0;i<AM_NUMMARKPOINTS;i++)
	markpoints[i].x = -1;	// means empty
    markpointnum = 0;
}

/* ---------------------------------------------------------------------- */
//
// should be called at the start of every level
// right now, i figure it out myself
//
void AM_LevelInit(void)
{
  leveljuststarted = 0;

  f_x = f_y = 0;
  f_w = SCREENWIDTH;
  f_h = ST_Y;

  AM_clearMarks();

  AM_findMinMaxBoundaries();
  scale_mtof = FixedDiv(min_scale_mtof, (int) (0.7*FRACUNIT));
  if (scale_mtof > max_scale_mtof)
      scale_mtof = min_scale_mtof;
  scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

/* ---------------------------------------------------------------------- */
//
//
//
void AM_Stop (void)
{
    AM_unloadPics();
    automapactive = false;
    ST_AutoMapEvent (AM_MSGEXITED);
    stopped = true;
}

/* ---------------------------------------------------------------------- */
//
//
//
void AM_Start (unsigned int restart)
{
  static int lastlevel = -1, lastepisode = -1;

  if (!stopped) AM_Stop();
  stopped = false;
  if ((restart)
   || (lastlevel != gamemap)
   || (lastepisode != gameepisode))
  {
    AM_LevelInit();
    lastlevel = gamemap;
    lastepisode = gameepisode;
  }
  AM_initVariables();
  AM_loadPics();
}

/* ---------------------------------------------------------------------- */
//
// set the window scale to the maximum size
//
static void AM_minOutWindowScale(void)
{
    scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

/* ---------------------------------------------------------------------- */
//
// set the window scale to the minimum size
//
static void AM_maxOutWindowScale(void)
{
    scale_mtof = max_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

/* ---------------------------------------------------------------------- */
//
// Handle events (user inputs) in automap mode
//
boolean
AM_Responder
(event_t* ev )
{

    int rc;
    static int cheatstate=0;
    static int bigstate=0;

    rc = false;

    if (!automapactive)
    {
	if (ev->type == ev_keydown && ev->data1 == AM_STARTKEY)
	{
	    AM_Start (0);
	    viewactive = false;
	    rc = true;
	}
    }

    else if (ev->type == ev_keydown)
    {

	rc = true;
	switch(ev->data1)
	{
	  case AM_PANRIGHTKEY: // pan right
	    if (!followplayer) m_paninc.x = FTOM(F_PANINC);
	    else rc = false;
	    break;
	  case AM_PANLEFTKEY: // pan left
	    if (!followplayer) m_paninc.x = -FTOM(F_PANINC);
	    else rc = false;
	    break;
	  case AM_PANUPKEY: // pan up
	    if (!followplayer) m_paninc.y = FTOM(F_PANINC);
	    else rc = false;
	    break;
	  case AM_PANDOWNKEY: // pan down
	    if (!followplayer) m_paninc.y = -FTOM(F_PANINC);
	    else rc = false;
	    break;
	  case AM_ZOOMOUTKEY: // zoom out
	    mtof_zoommul = M_ZOOMOUT;
	    ftom_zoommul = M_ZOOMIN;
	    break;
	  case AM_ZOOMINKEY1: // zoom in
	  case AM_ZOOMINKEY2:
	    mtof_zoommul = M_ZOOMIN;
	    ftom_zoommul = M_ZOOMOUT;
	    break;
	  case AM_ENDKEY:
	    bigstate = 0;
	    viewactive = true;
	    AM_Stop ();
	    break;
	  case AM_GOBIGKEY:
	    bigstate = !bigstate;
	    if (bigstate)
	    {
		AM_saveScaleAndLoc();
		AM_minOutWindowScale();
	    }
	    else AM_restoreScaleAndLoc();
	    break;
	  case AM_FOLLOWKEY:
	    followplayer = !followplayer;
	    f_oldloc.x = MAXINT;
	    plr->message = followplayer ?
		am_map_messages [AM_AMSTR_FOLLOWON] : am_map_messages [AM_AMSTR_FOLLOWOFF];
	    break;
	  case AM_GRIDKEY:
	    grid = !grid;
	    plr->message = grid ?
		am_map_messages [AM_AMSTR_GRIDON] : am_map_messages [AM_AMSTR_GRIDOFF];
	    break;
	  case AM_MARKKEY:
	    plr->message = HU_printf ("%s %d", am_map_messages [AM_AMSTR_MARKEDSPOT], markpointnum);
	    AM_addMark();
	    break;
	  case AM_CLEARMARKKEY:
	    AM_clearMarks();
	    plr->message = am_map_messages [AM_AMSTR_MARKSCLEARED];
	    break;
	  case 'k':
	    k_cheating = 1;
	    break;
	  case 'd':
	    d_cheating = 1;
	    break;
	  default:
	    cheatstate=0;
	    rc = false;
	}
	if (!deathmatch && cht_CheckCheat(&cheat_amap, ev->data1))
	{
	    rc = false;
	    ddt_cheating = (ddt_cheating+1) % 4;
	}
    }

    else if (ev->type == ev_keyup)
    {
	rc = false;
	switch (ev->data1)
	{
	  case AM_PANRIGHTKEY:
	    if (!followplayer) m_paninc.x = 0;
	    break;
	  case AM_PANLEFTKEY:
	    if (!followplayer) m_paninc.x = 0;
	    break;
	  case AM_PANUPKEY:
	    if (!followplayer) m_paninc.y = 0;
	    break;
	  case AM_PANDOWNKEY:
	    if (!followplayer) m_paninc.y = 0;
	    break;
	  case AM_ZOOMOUTKEY:
	  case AM_ZOOMINKEY1:
	  case AM_ZOOMINKEY2:
	    mtof_zoommul = FRACUNIT;
	    ftom_zoommul = FRACUNIT;
	    break;
	  case 'k':
	    k_cheating = 0;
	    break;
	  case 'd':
	    d_cheating = 0;
	    break;
	}
    }

    return (boolean)rc;
}

/* ---------------------------------------------------------------------- */
//
// Zooming
//
static void AM_changeWindowScale(void)
{

    // Change the scaling multipliers
    scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

    if (scale_mtof < min_scale_mtof)
	AM_minOutWindowScale();
    else if (scale_mtof > max_scale_mtof)
	AM_maxOutWindowScale();
    else
	AM_activateNewScale();
}

/* ---------------------------------------------------------------------- */
//
//
//
static void AM_doFollowPlayer(void)
{

    if (f_oldloc.x != plr->mo->x || f_oldloc.y != plr->mo->y)
    {
	m_x = FTOM(MTOF(plr->mo->x)) - m_w/2;
	m_y = FTOM(MTOF(plr->mo->y)) - m_h/2;
	m_x2 = m_x + m_w;
	m_y2 = m_y + m_h;
	f_oldloc.x = plr->mo->x;
	f_oldloc.y = plr->mo->y;

	//  m_x = FTOM(MTOF(plr->mo->x - m_w/2));
	//  m_y = FTOM(MTOF(plr->mo->y - m_h/2));
	//  m_x = plr->mo->x - m_w/2;
	//  m_y = plr->mo->y - m_h/2;
    }
}

/* ---------------------------------------------------------------------- */
//
//
//
#if 0
static void AM_updateLightLev(void)
{
    static int nexttic = 0;
    //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
    static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
    static int litelevelscnt = 0;

    // Change light level
    if (amclock>nexttic)
    {
	lightlev = litelevels[litelevelscnt++];
	if (litelevelscnt == sizeof(litelevels)/sizeof(int)) litelevelscnt = 0;
	nexttic = amclock + 6 - (amclock % 6);
    }
}
#endif

/* ---------------------------------------------------------------------- */
//
// Updates on Game Tick
//
void AM_Ticker (void)
{

    if (!automapactive)
	return;

    amclock++;

    if (followplayer)
	AM_doFollowPlayer();

    // Change the zoom if necessary
    if (ftom_zoommul != FRACUNIT)
	AM_changeWindowScale();

    // Change x,y location
    if (m_paninc.x || m_paninc.y)
	AM_changeWindowLoc();

    // Update light level
    // AM_updateLightLev();

    // Clear the screen either side of the status bar.
    ST_ClearSbarSides ();
}

/* ---------------------------------------------------------------------- */
//
// Clear automap frame buffer.
//
static void AM_clearFB(int colour)
{
    memset(fb, colour, f_w*f_h*sizeof(pixel_t));
}

/* ---------------------------------------------------------------------- */
//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
#define ADJUST_COORDS

#ifndef ADJUST_COORDS
static boolean AM_clipMline (const mline_t* ml, fline_t* fl)
{
    enum
    {
	LEFT   = 1,
	RIGHT  = 2,
	TOP    = 4,
	BOTTOM = 8
    };

    unsigned int outcode1 = 0;
    unsigned int outcode2 = 0;

    fl->a.x = CXMTOF(ml->a.x);
    if (fl->a.x < -1)
	outcode1 = LEFT;
    else if (fl->a.x >= (int)f_w)
	outcode1 = RIGHT;
    fl->b.x = CXMTOF(ml->b.x);
    if (fl->b.x < -1)
	outcode2 = LEFT;
    else if (fl->b.x >= (int)f_w)
	outcode2 = RIGHT;
    if (outcode1 & outcode2)
	return false;
    fl->a.y = CYMTOF(ml->a.y);
    if (fl->a.y < -1)
	outcode1 |= TOP;
    else if (fl->a.y >= (int)f_h)
	outcode1 |= BOTTOM;
    fl->b.y = CYMTOF(ml->b.y);
    if (fl->b.y < -1)
	outcode2 |= TOP;
    else if (fl->b.y >= (int)f_h)
	outcode2 |= BOTTOM;
    return (boolean)(!(outcode1 & outcode2));
}

/* ---------------------------------------------------------------------- */

#define PUTDOT(xx,yy,cc)		\
	if ((xx >= 0) && (xx < f_w)	\
	 && (yy >= 0) && (yy < f_h))	\
	  fb[(yy)*f_w+(xx)]=(cc)
#else
/* ---------------------------------------------------------------------- */

static boolean AM_clipMline (const mline_t* ml, fline_t* fl)
{
    enum
    {
	LEFT    =1,
	RIGHT   =2,
	BOTTOM  =4,
	TOP     =8
    };

    register int	outcode1 = 0;
    register int	outcode2 = 0;
    register int	outside;

    fpoint_t    tmp;
    int	 dx;
    int	 dy;


#define DOOUTCODE(oc, mx, my) \
    (oc) = 0; \
    if ((my) < 0) (oc) |= TOP; \
    else if ((my) >= f_h) (oc) |= BOTTOM; \
    if ((mx) < 0) (oc) |= LEFT; \
    else if ((mx) >= f_w) (oc) |= RIGHT;


    // do trivial rejects and outcodes
    if (ml->a.y > m_y2)
	outcode1 = TOP;
    else if (ml->a.y < m_y)
	outcode1 = BOTTOM;

    if (ml->b.y > m_y2)
	outcode2 = TOP;
    else if (ml->b.y < m_y)
	outcode2 = BOTTOM;

    if (outcode1 & outcode2)
	return false; // trivially outside

    if (ml->a.x < m_x)
	outcode1 |= LEFT;
    else if (ml->a.x > m_x2)
	outcode1 |= RIGHT;

    if (ml->b.x < m_x)
	outcode2 |= LEFT;
    else if (ml->b.x > m_x2)
	outcode2 |= RIGHT;

    if (outcode1 & outcode2)
	return false; // trivially outside

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
	return false;

    while (outcode1 | outcode2)
    {
	// may be partially inside box
	// find an outside point
	if (outcode1)
	    outside = outcode1;
	else
	    outside = outcode2;

	// clip to each side
	if (outside & TOP)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y))/dy;
	    tmp.y = 0;
	}
	else if (outside & BOTTOM)
	{
	    dy = fl->a.y - fl->b.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.x = fl->a.x + (dx*(fl->a.y-f_h))/dy;
	    tmp.y = f_h-1;
	}
	else if (outside & RIGHT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(f_w-1 - fl->a.x))/dx;
	    tmp.x = f_w-1;
	}
	else if (outside & LEFT)
	{
	    dy = fl->b.y - fl->a.y;
	    dx = fl->b.x - fl->a.x;
	    tmp.y = fl->a.y + (dy*(-fl->a.x))/dx;
	    tmp.x = 0;
	}

	if (outside == outcode1)
	{
	    fl->a = tmp;
	    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
	}
	else
	{
	    fl->b = tmp;
	    DOOUTCODE(outcode2, fl->b.x, fl->b.y);
	}

	if (outcode1 & outcode2)
	    return false; // trivially outside
    }

    return true;
}
#undef DOOUTCODE
#define PUTDOT(xx,yy,cc) fb[(yy)*f_w+(xx)]=(cc)
#endif
/* ---------------------------------------------------------------------- */
//
// This function is based on Bresenham's algorithm
//
static void AM_drawFline (const fline_t* fl, int colour)
{
  int x0, y0, x1, y1;
  int dy;
  int dx;
  int stepx, stepy;
  int fraction;

  x0 = fl->a.x;
  x1 = fl->b.x;
  y0 = fl->a.y;
  y1 = fl->b.y;

#ifdef ADJUST_COORDS
  // For debugging only
  if (x0 < 0 || x0 >= f_w
   || y0 < 0 || y0 >= f_h
   || x1 < 0 || x1 >= f_w
   || y1 < 0 || y1 >= f_h)
  {
//    fprintf(stderr, "fuck %d \r", fuck++);
      return;
  }
#endif

  dy = y1 - y0;
  dx = x1 - x0;

  if (dy < 0)
  {
    dy = -dy;
    stepy = -1;
  }
  else
  {
    stepy = 1;
  }

  if (dx < 0)
  {
    dx = -dx;
    stepx = -1;
  }
  else
  {
    stepx = 1;
  }

  dy <<= 1;
  dx <<= 1;

  PUTDOT (x0, y0, colour);

  if (dx > dy)
  {
    fraction = dy - (dx >> 1);
    while (x0 != x1)
    {
      if (fraction >= 0)
      {
	y0 += stepy;
	fraction -= dx;
      }
      x0 += stepx;
      fraction += dy;
      PUTDOT (x0, y0, colour);
    }
  }
  else
  {
    fraction = dx - (dy >> 1);
    while (y0 != y1)
    {
      if (fraction >= 0)
      {
	x0 += stepx;
	fraction -= dy;
      }
      y0 += stepy;
      fraction += dx;
      PUTDOT (x0, y0, colour);
    }
  }
}

/* ---------------------------------------------------------------------- */
//
// Clip lines, draw visible part sof lines.
//
static void
AM_drawMline
( const mline_t* ml,
  int	   colour )
{
    fline_t fl;

    if (colour==-1)	// jff 4/3/98 allow not drawing any sort of line
      return;		// by setting its colour to -1
#if 0
    if (colour==247)	// jff 4/3/98 if colour is 247 (xparent), use black
      colour=0;
#endif
    if (AM_clipMline(ml, &fl))
	AM_drawFline(&fl, colour); // draws it on frame buffer using fb coords
}

/* ---------------------------------------------------------------------- */
//
// Draws flat (floor/ceiling tile) aligned grid lines.
//
static void AM_drawGrid(int colour)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;

    // Figure out start of vertical gridlines
    start = m_x;
    if ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgx)%(MAPBLOCKUNITS<<FRACBITS));
    end = m_x + m_w;

    // draw vertical gridlines
    ml.a.y = m_y;
    ml.b.y = m_y+m_h;
    for (x=start; x<end; x+=(MAPBLOCKUNITS<<FRACBITS))
    {
	ml.a.x = x;
	ml.b.x = x;
	AM_drawMline(&ml, colour);
    }

    // Figure out start of horizontal gridlines
    start = m_y;
    if ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS))
	start += (MAPBLOCKUNITS<<FRACBITS)
	    - ((start-bmaporgy)%(MAPBLOCKUNITS<<FRACBITS));
    end = m_y + m_h;

    // draw horizontal gridlines
    ml.a.x = m_x;
    ml.b.x = m_x + m_w;
    for (y=start; y<end; y+=(MAPBLOCKUNITS<<FRACBITS))
    {
	ml.a.y = y;
	ml.b.y = y;
	AM_drawMline(&ml, colour);
    }

}

/* ---------------------------------------------------------------------- */
// AM_DoorColour()

// Returns the 'colour' or key needed for a door linedef type

// Passed the type of linedef, returns:
//   -1 if not a keyed door
//    0 if a red key required
//    1 if a blue key required
//    2 if a yellow key required
//    3 if a multiple keys required

// jff 4/3/98 add routine to get colour of generalized keyed door

static int AM_DoorColour(int type)
{
  if (d_cheating == 0)
  {
    if ((GenLockedBase <= type) && (type < 0x3C00))
    {
      type = (type >> LockedKeyShift) & 7;
      switch (type)
      {
	case RCard:
	  return (mapcolour.rcarddoor);

	case RSkull:
	  return (mapcolour.rskulldoor);

	case BCard:
	  return (mapcolour.bcarddoor);

	case BSkull:
	  return (mapcolour.bskulldoor);

	case YCard:
	  return (mapcolour.ycarddoor);

	case YSkull:
	  return (mapcolour.yskulldoor);

	default:
	  return (mapcolour.clsd);
      }
    }

    switch (type)	// closed keyed door
    {
      case 26: case 32:			// Blue card or skull
      case 99: case 133:
	return (mapcolour.bcarddoor);

      case 27: case 34:			// Yellow card or skull
      case 136: case 137:
	return (mapcolour.ycarddoor);

      case 28: case 33:			// Red card or skull
      case 134: case 135:
	return (mapcolour.rcarddoor);
    }
  }

  return (-1);		// Not a keyed door
}

/* ---------------------------------------------------------------------- */

static boolean secret_sector (sector_t * sector)
{
  unsigned int ss;

  ss = sector -> special;
  if ((ss & 0x1F) == 9)
    return (true);
  if (ss & SECRET_MASK)
    return (true);
  ss = sector -> oldspecial;
  if ((ss & 0x1F) == 9)
    return (true);
  if (ss & SECRET_MASK)
    return (true);
  return (false);
}

/* ---------------------------------------------------------------------- */

static boolean moving_sector (sector_t * sector)
{
  if (((amclock & 8) == 0)
   || ((sector->floordata == 0) && (sector->ceilingdata == 0)))
    return (false);
  return (true);
}

/* ---------------------------------------------------------------------- */
/* We treat a door as closed if the ceiling is near to the floor. */
/* (Fragport.wad level 13 has a Blue key door that is one pixel */
/* above the floor and Nova.wad level 28 has some bars way above the floor) */

static boolean door_closed (line_t* line)
{
  fixed_t floorheight;
  fixed_t ceilingheight;
  sector_t * sector;

  sector = line->backsector;
  floorheight = sector->floorheight;
  ceilingheight = sector->ceilingheight;

  if ((floorheight >= ceilingheight)
   || ((ceilingheight - floorheight) < mobjinfo [MT_PLAYER].height))
    return (true);

  sector = line->frontsector;
  floorheight = sector->floorheight;
  ceilingheight = sector->ceilingheight;

  if ((floorheight >= ceilingheight)
   || ((ceilingheight - floorheight) < mobjinfo [MT_PLAYER].height))
    return (true);

  return (false);
}

/* ---------------------------------------------------------------------- */

static boolean is_exit_line (int special)
{
  if (mapcolour.exit
   && (special==11
    || special==52
    || special==197
    || special==51
    || special==124
    || special==198))
    return (true);

  return (false);
}

/* ---------------------------------------------------------------------- */

static boolean is_teleport_line (int special)
{
  if (mapcolour.tele
   && (special == 39
    || special == 97
    || special == 125
    || special == 126
    || special == 174
    || special == 195
    || special == 207
    || special == 208
    || special == 209
    || special == 210
    || special == 243
    || special == 244
    || special == 262
    || special == 263
    || special == 264
    || special == 265
    || special == 266
    || special == 267
    || special == 268
    || special == 269))
    return (true);

  return (false);
}

/* ---------------------------------------------------------------------- */

static int AM_line_colour (line_t * line)
{
  int dcol;

  // if line has been seen or IDDT has been used
  if (ddt_cheating || (line->flags & ML_MAPPED))
  {
    if ((line->flags & ML_DONTDRAW) && !ddt_cheating)
      return (-1);

    if (!line->backsector)
    {
      if (moving_sector (line->frontsector))
	return (mapcolour.unsn);

      if (is_teleport_line (line->special))
	return (mapcolour.tele);

      if (is_exit_line (line->special))		//jff 4/23/98 add exit lines to automap
	return (mapcolour.exit);		// exit line

      if ((dcol = AM_DoorColour (line->special)) != -1)
	return (dcol);

      // jff 1/10/98 add new colour for 1S secret sector boundary
      if (secret_sector (line->frontsector))
	return (mapcolour.secr);		// line bounding secret sector
						//jff 2/16/98 fixed bug
      return (mapcolour.wall);			// special was cleared
    }

    // Two sided line

    if ((moving_sector (line->frontsector))
     || (moving_sector (line->backsector)))
      return (mapcolour.unsn);

    // jff 1/10/98 add colour change for all teleporter types
    if (is_teleport_line (line->special))
      return (mapcolour.tele);

    if (is_exit_line (line->special))		//jff 4/23/98 add exit lines to automap
      return (mapcolour.exit);			// exit line

    dcol = AM_DoorColour (line->special);	//jff 1/5/98 this clause implements showing keyed doors
    if (dcol != -1)
    {
      if ((line->tag != 0)
       || (door_closed (line)))
	return (dcol);

      return (mapcolour.cchg);			// open keyed door
    }

    if (line->flags & ML_SECRET)		// secret door
      return (mapcolour.wall);			// wall colour

    if (door_closed (line))
      return (mapcolour.clsd);

    //jff 1/6/98 show secret sector 2S lines
    if ((secret_sector (line->frontsector)
     || secret_sector (line->backsector)))
      return (mapcolour.secr);			// line bounding secret sector

    if (line->backsector->floorheight != line->frontsector->floorheight)
      return (mapcolour.fchg);			// floor level change

    if (line->backsector->ceilingheight != line->frontsector->ceilingheight)
      return (mapcolour.cchg);			// ceiling level change

    if (ddt_cheating)
      return (mapcolour.flat);			// 2S lines that appear only in IDDT
  }

  // now draw the lines only visible because the player has computermap


  if ((plr->powers[pw_allmap])			// computermap visible lines
   && (!(line->flags & ML_DONTDRAW))		// invisible flag lines do not show
   && (!line->backsector
    || (line->backsector->floorheight != line->frontsector->floorheight)
    || (line->backsector->ceilingheight != line->frontsector->ceilingheight)))
     return (mapcolour.unsn);

  return (-1);
}

/* ---------------------------------------------------------------------- */
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
static void AM_drawWalls(void)
{
  unsigned int i;
  line_t* line;
  mline_t l;

// draw the unclipped visible portions of all lines
  line = lines;
  i = numlines;
  do
  {
    l.a.x = line->v1->x;
    l.a.y = line->v1->y;
    l.b.x = line->v2->x;
    l.b.y = line->v2->y;
    AM_drawMline (&l, AM_line_colour (line));
    line++;
  } while (--i);
}

/* ---------------------------------------------------------------------- */
//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
static void
AM_rotate
( fixed_t*      x,
  fixed_t*      y,
  angle_t       a )
{
    fixed_t tmpx;

    tmpx =
	FixedMul(*x,finecosine[a>>ANGLETOFINESHIFT])
	- FixedMul(*y,finesine[a>>ANGLETOFINESHIFT]);

    *y   =
	FixedMul(*x,finesine[a>>ANGLETOFINESHIFT])
	+ FixedMul(*y,finecosine[a>>ANGLETOFINESHIFT]);

    *x = tmpx;
}

/* ---------------------------------------------------------------------- */

static void
AM_drawLineCharacter
( const mline_t* lineguy,
  int	   lineguylines,
  fixed_t       scale,
  angle_t       angle,
  int	   colour,
  fixed_t       x,
  fixed_t       y )
{
    int	 i;
    mline_t     l;

    for (i=0;i<lineguylines;i++)
    {
	l.a.x = lineguy[i].a.x;
	l.a.y = lineguy[i].a.y;

	if (scale)
	{
	    l.a.x = FixedMul(scale, l.a.x);
	    l.a.y = FixedMul(scale, l.a.y);
	}

	if (angle)
	    AM_rotate(&l.a.x, &l.a.y, angle);

	l.a.x += x;
	l.a.y += y;

	l.b.x = lineguy[i].b.x;
	l.b.y = lineguy[i].b.y;

	if (scale)
	{
	    l.b.x = FixedMul(scale, l.b.x);
	    l.b.y = FixedMul(scale, l.b.y);
	}

	if (angle)
	    AM_rotate(&l.b.x, &l.b.y, angle);

	l.b.x += x;
	l.b.y += y;

	AM_drawMline(&l, colour);
    }
}

/* ---------------------------------------------------------------------- */

static void AM_drawPlayers(void)
{
    int	 i;
    int	 colour;
    player_t*   p;

    if (!netgame)
    {
	if (ddt_cheating)
	    AM_drawLineCharacter
		(cheat_player_arrow, NUMCHEATPLYRLINES, 0,
		 plr->mo->angle, mapcolour.sngl, plr->mo->x, plr->mo->y);
	else
	    AM_drawLineCharacter
		(player_arrow, NUMPLYRLINES, 0, plr->mo->angle,
		 mapcolour.sngl, plr->mo->x, plr->mo->y);
	return;
    }

    p = &players[0];
    for (i=0;i<MAXPLAYERS;i++,p++)
    {
	if ( (deathmatch && !singledemo) && p != plr)
	    continue;

	if (!playeringame[i])
	    continue;

	if (p->powers[pw_invisibility])
	    colour = mapcolour.plyr_invis;		// *close* to black
	else
	    colour = mapcolour.plyr[i];			//jff 1/6/98 use default colour

	AM_drawLineCharacter
	    (player_arrow, NUMPLYRLINES, 0, p->mo->angle,
	     colour, p->mo->x, p->mo->y);
    }

}

/* ---------------------------------------------------------------------- */

static void
AM_drawThings (void)
{
  int		i;
  int		colour;
  mobj_t*       t;
  sector_t*     sector;

  i = numsectors;
  sector = &sectors [0];
  do
  {
    t = sector->thinglist;
    if (t)
    {
      do
      {
	colour = mapcolour.sprt;
	if (t->flags & MF_FRIEND && !t->player)
	  colour = mapcolour.frnd;
	AM_drawLineCharacter
		(thintriangle_guy, NUMTHINTRIANGLEGUYLINES,
		 16<<FRACBITS, t->angle, colour, t->x, t->y);
	t = t->snext;
      } while (t);
    }
    sector++;
  } while (--i);
}

/* ---------------------------------------------------------------------- */

static void
AM_drawkeys (void)
{
  int i;
  int colour;
  int angle;
  sector_t* sector;
  const mobj_t *t;
  const mshape_t *shape;

  i = numsectors;
  sector = &sectors [0];
  do
  {
    t = sector -> thinglist;
    if (t)
    {
      do
      {
//      shape = &thintriangle_shape;
//      colour = mapcolour.sprt;
	angle = 0;

	if (t->info)
	switch (t -> sprite)	// These are identified by sprite number in p_inter.c
	{
	  // Key cards
	  case SPR_BKEY:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.bcard;
	      shape = &key_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;

	  case SPR_YKEY:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.ycard;
	      shape = &key_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;

	  case SPR_RKEY:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.rcard;
	      shape = &key_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;

	  // Skull keys
	  case SPR_BSKU:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.bskull;
	      shape = &skullkey_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;

	  case SPR_YSKU:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.yskull;
	      shape = &skullkey_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;

	  case SPR_RSKU:
	    if (t->info->flags & MF_SPECIAL)
	    {
	      colour = mapcolour.rskull;
	      shape = &skullkey_shape;
	      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
				angle, colour, t->x, t->y);
	    }
	    break;
	}

	t = t -> snext;
      } while (t);
    }
    sector++;
  } while (--i);
}

/* ---------------------------------------------------------------------- */

static void
AM_drawThingsDifferently (void)
{
  int i;
  sector_t* sector;

  sector = sectors;
  for (i = 0; i < numsectors; i++, sector++)
  {
    const mobj_t *t = sector->thinglist;
    while (t)
    {
      const mshape_t *shape = &thintriangle_shape;
      int colour = mapcolour.sprt;
      int angle = 0;

      if (t->info)
      {
	if (t->info->flags & (MF_SHOOTABLE|MF_COUNTKILL))
	{
	  if (t->health > 0)
	  {
	    colour = mapcolour.monster;
	    if (t->flags & MF_FRIEND && !t->player)
	      colour = mapcolour.frnd;
	    angle = t->angle;
	  }
	  else
	  {
	    colour = mapcolour.dead;
	    shape = &hexagon_shape;
	  }
	}
	else
	{
	  switch (t -> sprite)	// These are identified by sprite number in p_inter.c
	  {
	    // Weapons
	    case SPR_BFUG:
	    case SPR_MGUN:
	    case SPR_CSAW:
	    case SPR_LAUN:
	    case SPR_PLAS:
	    case SPR_SHOT:
	    case SPR_SGN2:
	      break;

	    // Ammunition
	    case SPR_CLIP:
	    case SPR_AMMO:
	    case SPR_ROCK:
	    case SPR_BROK:
	    case SPR_CELL:
	    case SPR_CELP:
	    case SPR_SHEL:
	    case SPR_SBOX:
	    case SPR_BPAK:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.ammo;
		shape = &pentacle_shape;
	      }
	      break;

	    // Health, armour
	    case SPR_ARM1:
	    case SPR_ARM2:
	    case SPR_BON1:
	    case SPR_BON2:
	    case SPR_STIM:
	    case SPR_MEDI:
	    case SPR_BEXP:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.bonus;
		shape = &pentacle_shape;
	      }
	      break;

	    // Special objects
	    case SPR_SOUL:
	    case SPR_MEGA:
	    case SPR_PINV:
	    case SPR_PSTR:
	    case SPR_PINS:
	    case SPR_SUIT:
	    case SPR_PMAP:
	    case SPR_PVIS:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.special;
		shape = &pentacle_shape;
	      }
	      break;

	    // Key cards
	    case SPR_BKEY:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.bcard;
		shape = &key_shape;
	      }
	      break;

	    case SPR_YKEY:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.ycard;
		shape = &key_shape;
	      }
	      break;

	    case SPR_RKEY:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.rcard;
		shape = &key_shape;
	      }
	      break;

	    // Skull keys
	    case SPR_BSKU:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.bskull;
		shape = &skullkey_shape;
	      }
	      break;

	    case SPR_YSKU:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.yskull;
		shape = &skullkey_shape;
	      }
	      break;

	    case SPR_RSKU:
	      if (t->info->flags & MF_SPECIAL)
	      {
		colour = mapcolour.rskull;
		shape = &skullkey_shape;
	      }
	      break;

	  default:				// Not known by its sprite
	    switch (t->info->doomednum)
	    {
#if 0
	      // Monsters
	      case 7: case 9: case 16: case 58: case 64: case 65: case 66:
	      case 67: case 68: case 69: case 71: case 84: case 3001: case 3002:
	      case 3003: case 3004: case 3005: case 3006:
	      // Boss brain, Commander Keen
	      case 88: case 72:
	      case 888:
		if (t->health > 0)
		{
		  colour = mapcolour.monster;
		  angle = t->angle;
		}
		else
		{
		  colour = mapcolour.dead;
		  shape = &hexagon_shape;
		}
		break;

	      // Weapons
	      case 82: case 2001: case 2002: case 2003: case 2004: case 2005:
	      case 2006: case 2035:
		break;
	      // Ammunition
	      case 8: case 17: case 2007: case 2008: case 2010: case 2046:
	      case 2047: case 2048: case 2049:
		colour = mapcolour.ammo;
		shape = &pentacle_shape;
		break;
	      // Health, armour
	      case 2011: case 2012: case 2014: case 2015: case 2018: case 2019:
		colour = mapcolour.bonus;
		shape = &pentacle_shape;
		break;
	      // Special objects
	      case 83: case 2013: case 2022: case 2023: case 2024: case 2025:
	      case 2026: case 2045:
		colour = mapcolour.special;
		shape = &pentacle_shape;
		break;
	      // Key cards
	      case 5 : colour = mapcolour.bcard; shape = &key_shape; break;
	      case 6 : colour = mapcolour.ycard; shape = &key_shape; break;
	      case 13: colour = mapcolour.rcard; shape = &key_shape; break;
	      // Skull keys
	      case 40: colour = mapcolour.bskull; shape = &skullkey_shape; break;
	      case 39: colour = mapcolour.yskull; shape = &skullkey_shape; break;
	      case 38: colour = mapcolour.rskull; shape = &skullkey_shape; break;
#endif
	      // Pillars
	      case 30: case 31: case 32: case 33: case 36: case 37:
		colour = mapcolour.pillar;
		shape = &hexagon_shape;
		break;
	      // Trees
	      case 41: case 43: case 47: case 48: case 54:
		colour = mapcolour.tree;
		shape = &hexagon_shape;
		break;
	      // Lighting
	      case 34: case 35: case 44: case 45: case 46: case 55: case 56:
	      case 57: case 70: case 85: case 86: case 2028:
		colour = mapcolour.light;
		shape = &hexagon_shape;
		break;
	      // Bodies
	      case 25: case 26: case 49: case 50: case 51: case 52: case 53:
	      case 59: case 60: case 61: case 62: case 63: case 73: case 74:
	      case 75: case 76: case 77: case 78:
		colour = mapcolour.body;
		shape = &hexagon_shape;
		break;
	      // Dead players & monsters
	      case 10: case 12: case 15: case 18: case 19: case 20: case 21:
	      case 22: case 23:
		colour = mapcolour.dead;
		shape = &hexagon_shape;
		break;
	      // Blood
	      case 24: case 79: case 80: case 81:
		colour = mapcolour.blood;
		shape = &hexagon_shape;
		break;
	      // Skulls
	      case 27: case 28: case 29: case 42:
		colour = mapcolour.skull;
		shape = &hexagon_shape;
		break;
	      // Music changer
	      case 14100:
		shape = &music_shape;
		break;
	      default:
		angle = t->angle;
	    }
	  }
	}
      }
      AM_drawLineCharacter (shape->shape, shape->numlines, 16<<FRACBITS,
			    angle, colour, t->x, t->y);
      t = t->snext;
    }
  }
}

/* ---------------------------------------------------------------------- */

static void AM_drawMarks(void)
{
  int i;
  for (i=0;i<markpointnum;i++)	// killough 2/22/98: remove automap mark limit
    if (markpoints[i].x != -1)
    {
      int w = 5;
      int h = 6;
      int fx = CXMTOF(markpoints[i].x);
      int fy = CYMTOF(markpoints[i].y);
      int j = i;

      do
      {
	int d = j % 10;
	if (d==1)		// killough 2/22/98: less spacing for '1'
	  fx++;

	if (fx >= f_x && fx < f_w - w && fy >= f_y && fy < f_h - h)
	  V_DrawPatch(fx, fy, FB, marknums[d]);
	fx -= w-1;		// killough 2/22/98: 1 space backwards
	j /= 10;
      }
      while (j>0);
    }
}

/* ---------------------------------------------------------------------- */

static void AM_drawCrosshair(int colour)
{
    fb[(f_w*(f_h+1))/2] = colour;	// single point for now

}

void AM_Drawer (void)
{
    if (!automapactive) return;

    AM_clearFB(mapcolour.back);		//jff 1/5/98 background default colour
    if (grid)
	AM_drawGrid(mapcolour.grid);	//jff 1/7/98 grid default colour
    AM_drawWalls();
    AM_drawPlayers();
    if (k_cheating)
	AM_drawkeys();
    else if (ddt_cheating==2)
	AM_drawThings();
    else if (ddt_cheating==3)
	AM_drawThingsDifferently();

    AM_drawCrosshair(mapcolour.hair);	//jff 1/7/98 default crosshair colour

    AM_drawMarks();

    V_MarkRect(f_x, f_y, f_w, f_h);
}

/* ---------------------------------------------------------------------- */

void AM_SetKeyColour (int locknum, int keynum, int r, int g, int b)
{
  unsigned char colour;
  unsigned char * palette;

  palette = W_CacheLumpName ("PLAYPAL", PU_STATIC);
  colour = AM_load_colour (r, g, b, palette);

  switch (locknum)
  {
    case 1:
      mapcolour.rcarddoor = colour;
      break;

    case 2:
      mapcolour.bcarddoor = colour;
      break;

    case 3:
      mapcolour.ycarddoor = colour;
      break;

    case 4:
      mapcolour.rskulldoor = colour;
      break;

    case 5:
      mapcolour.bskulldoor = colour;
      break;

    case 6:
      mapcolour.yskulldoor = colour;
      break;

    default:
      if (M_CheckParm ("-showunknown"))
	printf ("Unknown locknumber %u\n", locknum);
  }

  switch (keynum)
  {
    case 1:
      mapcolour.rcard = colour;
      break;

    case 2:
      mapcolour.bcard = colour;
      break;

    case 3:
      mapcolour.ycard = colour;
      break;

    case 4:
      mapcolour.rskull = colour;
      break;

    case 5:
      mapcolour.bskull = colour;
      break;

    case 6:
      mapcolour.yskull = colour;
      break;

    default:
      if (M_CheckParm ("-showunknown"))
	printf ("Unknown keynumber %u\n", keynum);
  }
}

/* ---------------------------------------------------------------------- */
