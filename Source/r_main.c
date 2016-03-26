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
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: r_main.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"
#include <math.h>

/* -------------------------------------------------------------------------- */

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW		2048


static unsigned int	r_frame_count;
int			viewangleoffset;

// increment every time a check is made
int			validcount = 1;


lighttable_t*		fixedcolormap;
int			lightScaleShift;
extern lighttable_t**	walllights;

int			centerx;
int			centery;

fixed_t			centerxfrac;
fixed_t			centeryfrac;
fixed_t			projection;

// just for profiling purposes
int			framecount;

int			sscount;
int			linecount;
int			loopcount;

fixed_t			viewx;
fixed_t			viewy;
fixed_t			viewz;

angle_t			viewangle;

fixed_t			viewcos;
fixed_t			viewsin;

player_t*		viewplayer;

// 0 = high, 1 = low
int			detailshift;

//
// precalculated math tables
//
angle_t			clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int			viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t*		xtoviewangle;


// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.
// fixed_t		finetangent[FINEANGLES/2];

// fixed_t		finesine[5*FINEANGLES/4];
fixed_t*		finecosine = &finesine[FINEANGLES/4];


lighttable_t*		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*		scalelightfixed[MAXLIGHTSCALE];
lighttable_t*		zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int			extralight;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*spanfunc) (void);

/* -------------------------------------------------------------------------- */
//
// R_AddPointToBox
// Expand a given bbox
// so that it encloses a given point.
//
void
R_AddPointToBox
( int		x,
  int		y,
  fixed_t*	box )
{
    if (x< box[BOXLEFT])
	box[BOXLEFT] = x;
    if (x> box[BOXRIGHT])
	box[BOXRIGHT] = x;
    if (y< box[BOXBOTTOM])
	box[BOXBOTTOM] = y;
    if (y> box[BOXTOP])
	box[BOXTOP] = y;
}

/* -------------------------------------------------------------------------- */
//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int
R_PointOnSide
( fixed_t	x,
  fixed_t	y,
  node_t*	node )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    if (!node->dx)
    {
	if (x <= node->x)
	    return node->dy > 0;

	return node->dy < 0;
    }
    if (!node->dy)
    {
	if (y <= node->y)
	    return node->dx < 0;

	return node->dx > 0;
    }

    dx = (x - node->x);
    dy = (y - node->y);

    // Try to quickly decide by looking at sign bits.
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
	if  ( (node->dy ^ dx) & 0x80000000 )
	{
	    // (left is negative)
	    return 1;
	}
	return 0;
    }

    left = FixedMul ( node->dy>>FRACBITS , dx );
    right = FixedMul ( dy , node->dx>>FRACBITS );

    if (right < left)
    {
	// front side
	return 0;
    }
    // back side
    return 1;
}

/* -------------------------------------------------------------------------- */

int
R_PointOnSegSide
( fixed_t	x,
  fixed_t	y,
  seg_t*	line )
{
    fixed_t	lx;
    fixed_t	ly;
    fixed_t	ldx;
    fixed_t	ldy;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    lx = line->v1->x;
    ly = line->v1->y;

    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;

    if (!ldx)
    {
	if (x <= lx)
	    return ldy > 0;

	return ldy < 0;
    }
    if (!ldy)
    {
	if (y <= ly)
	    return ldx < 0;

	return ldx > 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    // Try to quickly decide by looking at sign bits.
    if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    {
	if  ( (ldy ^ dx) & 0x80000000 )
	{
	    // (left is negative)
	    return 1;
	}
	return 0;
    }

    left = FixedMul ( ldy>>FRACBITS , dx );
    right = FixedMul ( dy , ldx>>FRACBITS );

    if (right < left)
    {
	// front side
	return 0;
    }
    // back side
    return 1;
}


/* -------------------------------------------------------------------------- */
//
// R_PointToAngle
// To get a global angle from Cartesian coordinates,
// the coordinates are flipped until they are in the first octant of
// the coordinate system, then the y (<=x) is scaled and divided by x
// to get a tangent (slope) value which is looked up in the
// tantoangle[] table.

angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y)
{
    angle_t rc;
    double at;

    x -= x1;
    y -= y1;

    if (!x && !y)
	return 0;

    if (x > MAXINT / 4 || x < -MAXINT / 4 || y > MAXINT / 4 || y < -MAXINT / 4)
    {
	at = atan2(y, x) * ANG180 / M_PI;
	if (at < 0)			// Major bug in RiscOS here, barfs when
	{				// the number is negative!
	  at = -at;
	  rc = (angle_t) at;
	  rc = -rc;
	}
	else
	{
	  rc = (angle_t) at;
	}
	return (rc);
    }

    if (x >= 0)
    {
	if (y >= 0)
	    return (x > y ? tantoangle[SlopeDiv(y, x)] :
		ANG90 - 1 - tantoangle[SlopeDiv(x, y)]);
	else
	{
	    y = -y;
	    return (x > y ? -(int)tantoangle[SlopeDiv(y, x)] :
		ANG270 + tantoangle[SlopeDiv(x, y)]);
	}
    }
    else
    {
	x = -x;
	if (y >= 0)
	    return (x > y ? ANG180 - 1 - tantoangle[SlopeDiv(y, x)] :
		ANG90 + tantoangle[SlopeDiv(x, y)]);
	else
	{
	    y = -y;
	    return (x > y ? ANG180 + tantoangle[SlopeDiv(y, x)] :
		ANG270 - 1 - tantoangle[SlopeDiv(x, y)]);
	}
    }
}

/* -------------------------------------------------------------------------- */
// Point of view (viewx, viewy) to point (x1, y1) angle.
angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
    return R_PointToAngle2(viewx, viewy, x, y);
}

/* -------------------------------------------------------------------------- */
// Point of view (viewx, viewy) to point (x1, y1) angle.
angle_t R_PointToAngleEx(fixed_t x, fixed_t y)
{
    return R_PointToAngleEx2(viewx, viewy, x, y);
}

/* -------------------------------------------------------------------------- */

angle_t R_PointToAngleEx2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y)
{
    // [crispy] fix overflows for very long distances
    int64_t     y_viewy = (int64_t)y - y1;
    int64_t     x_viewx = (int64_t)x - x1;

    // [crispy] the worst that could happen is e.g. MININT-MAXINT = 2*MININT
    if (x_viewx < MININT || x_viewx > MAXINT || y_viewy < MININT || y_viewy > MAXINT)
    {
	// [crispy] preserving the angle by halving the distance in both directions
	x = (int)(x_viewx / 2 + x1);
	y = (int)(y_viewy / 2 + y1);
    }

    return R_PointToAngle2(x1, y1, x, y);
}

/* -------------------------------------------------------------------------- */

fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y )
{
    int		angle;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	temp;
    fixed_t	dist;
    fixed_t	i;

    dx = abs(x - viewx);
    dy = abs(y - viewy);

    if (dy>dx)
    {
	temp = dx;
	dx = dy;
	dy = temp;
    }

    if (dx == 0)			// Avoid divide by zero
    {
      i = SLOPERANGE;
    }
    else
    {
      i = FixedDiv (dy,dx) >> DBITS;
      if (i > SLOPERANGE)
	i = SLOPERANGE;
    }

    angle = (tantoangle [i]+ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    dist = FixedDiv (dx, finesine[angle] );

    return dist;
}

/* -------------------------------------------------------------------------- */
//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int	i;
    long	t;
    float	f;
//
// slope (tangent) to angle lookup
//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
	f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
	t = 0xffffffff*f;
	tantoangle[i] = t;
    }
#endif
}

/* -------------------------------------------------------------------------- */
//
// R_InitTables
//
void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int		i;
    float	a;
    float	fv;
    int		t;

    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
	fv = FRACUNIT*tan (a);
	t = fv;
	finetangent[i] = t;
    }

    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
	// OPTIMIZE: mirror...
	a = (i+0.5)*PI*2/FINEANGLES;
	t = FRACUNIT*sin (a);
	finesine[i] = t;
    }
#endif

}

/* -------------------------------------------------------------------------- */

angle_t R_GetVertexViewAngle (vertex_t *v)
{
  if (v->angletime != r_frame_count)
  {
    v->angletime = r_frame_count;
    return (v->viewangle = R_PointToAngle (v->x, v->y));
  }

  return (v->viewangle);
}

/* -------------------------------------------------------------------------- */
//
// R_InitTextureMapping
//
void R_InitTextureMapping (void)
{
    int			i;
    int			x;
    int			t;
    fixed_t		focallength;

    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv (centerxfrac,
			    finetangent[FINEANGLES/4+FIELDOFVIEW/2] );

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	if (finetangent[i] > FRACUNIT*2)
	    t = -1;
	else if (finetangent[i] < -FRACUNIT*2)
	    t = viewwidth+1;
	else
	{
	    t = FixedMul (finetangent[i], focallength);
	    t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

	    if (t < -1)
		t = -1;
	    else if (t>viewwidth+1)
		t = viewwidth+1;
	}
	viewangletox[i] = t;
    }

    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.
    for (x=0;x<=viewwidth;x++)
    {
	i = 0;
	while (viewangletox[i]>x)
	    i++;
	xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
	t = FixedMul (finetangent[i], focallength);
	t = centerx - t;

	if (viewangletox[i] == -1)
	    viewangletox[i] = 0;
	else if (viewangletox[i] == viewwidth+1)
	    viewangletox[i]  = viewwidth;
    }

    clipangle = xtoviewangle[0];
}

/* -------------------------------------------------------------------------- */
//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP		2

void R_InitLightTables (void)
{
    int		i;
    int		j;
    int		level;
    int		startmap;
    int		scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-LIGHTBRIGHT-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTZ ; j++)
	{
	    scale = FixedDiv ((SCREENWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
	    scale >>= lightScaleShift;
	    level = startmap - scale/DISTMAP;

	    if (level < 0)
		level = 0;

	    if (NUMCOLORMAPS > 32)
	    {
	      level = level / 6;
	      if (level > 31)
		level = 31;
	    }
	    else
	    {
	      if (level >= NUMCOLORMAPS)
		level = NUMCOLORMAPS-1;
	    }
	    zlight[i][j] = colormaps + level*256;
	}
    }
}

/* -------------------------------------------------------------------------- */
//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
int		setsizeneeded;
int		setblocks;
int		setdetail;


void R_SetViewSize (int flag, int blocks, int detail)
{
    setsizeneeded = flag;
    setblocks = blocks;
    setdetail = detail;
}

/* -------------------------------------------------------------------------- */
//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize (void)
{
    fixed_t	cosadj;
    fixed_t	dy;
    fixed_t	sis;
    int		i;
    int		j;
    int		level;
    int		startmap;

    if (setsizeneeded)
    {
      sbarscale = (FRACUNIT*SCREENWIDTH)/320;
      if ((stbar_scale < 2) && (SCREENWIDTH > 320))
	sbarscale = sbarscale / 2;

      if (hutext_scale > 1)
	hutextscale = (FRACUNIT*SCREENWIDTH)/320;
      else if (SCREENWIDTH > 800)
	hutextscale = 2 << FRACBITS;
      else
	hutextscale = 1 << FRACBITS;

      ST_createWidgets ();
      HU_createWidgets (1);
      if (automapactive)
	AM_Start (1);
      else
	AM_LevelInit ();
    }

    switch (setblocks)
    {
      case 11:				// Full screen - no status bar
	scaledviewwidth = SCREENWIDTH;
	viewheight = SCREENHEIGHT;
        break;

      case 10:				// Fill width - status bar visible.
	scaledviewwidth = SCREENWIDTH;
	viewheight = SCREENHEIGHT-((32*sbarscale)>>FRACBITS);
        break;

      default:				// Small screen - border visible.
	scaledviewwidth = ((setblocks*SCREENWIDTH)/10)&~7;
	viewheight = ((setblocks*(SCREENHEIGHT-ST_HEIGHT))/10)&~7;
    }

    detailshift = setdetail;
    viewwidth = scaledviewwidth>>detailshift;

    centery = viewheight/2;
    centerx = viewwidth/2;
    centerxfrac = centerx<<FRACBITS;
    centeryfrac = centery<<FRACBITS;
    projection = centerxfrac;

    if (!detailshift)
    {
	colfunc = basecolfunc = R_DrawColumn;
	fuzzcolfunc = R_DrawFuzzColumn;
	spanfunc = R_DrawSpan;
    }
    else
    {
	colfunc = basecolfunc = R_DrawColumnLow;
	fuzzcolfunc = R_DrawFuzzColumn;
	spanfunc = R_DrawSpanLow;
    }

    R_InitBuffer (scaledviewwidth, viewheight);

    R_InitTextureMapping ();

    sis = (FRACUNIT*SCREENWIDTH/viewwidth)>>detailshift;
    if (SCREENHEIGHT > 200)
    {
      sis = (sis * 200) / SCREENHEIGHT;
    }
    skyiscale = sis;

    // psprite scales
    // Try to fix for different aspect ratios, otherwise the sky looks really weird
    // in non-320:200 modes. Uncomment the above lines and remove the following
    // 10 lines to get the original behaviour.
    pspritescale = (FRACUNIT*viewwidth)/320;
    pspriteiscale = (FRACUNIT*viewheight)/200;
    if (pspritescale >= pspriteiscale)
    {
      pspriteiscale = (FRACUNIT*320)/viewwidth;
    }
    else
    {
      pspritescale = pspriteiscale;
      pspriteiscale = (FRACUNIT*200)/viewheight;
    }
    if (weaponscale < 2)
    {
      pspritescale = pspritescale / 2;
      pspriteiscale = pspriteiscale * 2;
    }


    // thing clipping
    for (i=0 ; i<viewwidth ; i++)
	screenheightarray[i] = viewheight;

    // planes
    for (i=0 ; i<viewheight ; i++)
    {
	dy = ((i-viewheight/2)<<FRACBITS)+FRACUNIT/2;
	dy = abs(dy);
	yslope[i] = FixedDiv ( (viewwidth<<detailshift)/2*FRACUNIT, dy);
    }

    for (i=0 ; i<viewwidth ; i++)
    {
	cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
	distscale[i] = FixedDiv (FRACUNIT,cosadj);
    }

    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
	startmap = ((LIGHTLEVELS-LIGHTBRIGHT-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
	for (j=0 ; j<MAXLIGHTSCALE ; j++)
	{
	    level = startmap - j*SCREENWIDTH/(viewwidth<<detailshift)/DISTMAP;

	    if (level < 0)
		level = 0;

	    if (NUMCOLORMAPS > 32)
	    {
	      level = level / 6;
	      if (level > 31)
		level = 31;
	    }
	    else
	    {
	      if (level >= NUMCOLORMAPS)
		level = NUMCOLORMAPS-1;
	    }
	    scalelight[i][j] = colormaps + level*256;
	}
    }

    if (setsizeneeded > 1)
      players[consoleplayer].message = HU_printf ("Scale: Bar %u, Weapon %u, Text %u", stbar_scale, weaponscale, hutext_scale);

    setsizeneeded = 0;
}

/* -------------------------------------------------------------------------- */
//
// R_Init
//
//extern int	detailLevel;		// Not currently used
extern int	screenblocks;


void R_Init (void)
{
    if (M_CheckParm ("-nomapinfo") == 0)
      Load_Mapinfo ();

    xtoviewangle = calloc (SCREENWIDTH+1, sizeof (*xtoviewangle));

    R_InitData ();
    // printf ("\nR_InitData");
    R_InitPointToAngle ();
    // printf ("\nR_InitPointToAngle");
    R_InitTables ();
    // viewwidth / viewheight / detailLevel are set by the defaults
    // printf ("\nR_InitTables");

    R_SetViewSize (1, screenblocks, 0 /* detailLevel */);
    R_InitPlanes ();
    // printf ("\nR_InitPlanes");
    R_InitLightTables ();
    // printf ("\nR_InitLightTables");
    R_InitSkyMap ();
    // printf ("\nR_InitSkyMap");
    R_InitTranslationTables ();
    // printf ("\nR_InitTranslationsTables");

    framecount = 0;
}

/* -------------------------------------------------------------------------- */
//
// R_PointInSubsector
//
subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y )
{
    node_t*	node;
    int		side;
    int		nodenum;

    // single subsector is a special case
    if (!numnodes)
	return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
	node = &nodes[nodenum];
	side = R_PointOnSide (x, y, node);
	nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

/* -------------------------------------------------------------------------- */
//
// R_SetupFrame
//
void R_SetupFrame (player_t* player)
{
    int		i;

    viewplayer = player;
    viewx = player->mo->x;
    viewy = player->mo->y;
    viewangle = player->mo->angle + viewangleoffset;
    extralight = player->extralight * WFLASHBRIGHT * LIGHTBRIGHT;

    viewz = player->viewz;

    viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

    sscount = 0;

    if (player->fixedcolormap)
    {
	fixedcolormap =
	    colormaps
	    + player->fixedcolormap*256*sizeof(lighttable_t);

	walllights = scalelightfixed;

	for (i=0 ; i<MAXLIGHTSCALE ; i++)
	    scalelightfixed[i] = fixedcolormap;
    }
    else
	fixedcolormap = 0;

    framecount++;
    validcount++;
}

/* -------------------------------------------------------------------------- */
//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{
    r_frame_count++;
    R_SetupFrame (player);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();

    // check for new console commands.
    NetUpdate ();

    // The head node is the last node output.
    R_RenderBSPNode (numnodes-1);

    // Check for new console commands.
    NetUpdate ();

    R_DrawPlanes ();

    // Check for new console commands.
    NetUpdate ();

    R_DrawMasked ();

    // Check for new console commands.
    NetUpdate ();
}

/* -------------------------------------------------------------------------- */
