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
//	Here is a core component: drawing the floors and ceilings,
//	 while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: r_plane.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif

#include <stdlib.h>

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"

#include "r_local.h"
#include "r_sky.h"
#include "m_argv.h"


static boolean		showrplanestats;
planefunction_t		floorfunc;
planefunction_t		ceilingfunc;

//
// opening
//

// Here comes the obnoxious "visplane".
//#define MAXVISPLANES	0x300
//visplane_t		visplanes[MAXVISPLANES];

static unsigned int MAXVISPLANES;
visplane_t*		visplanes;
visplane_t*		lastvisplane;
visplane_t*		floorplane;
visplane_t*		ceilingplane;

// ?
unsigned int MAXOPENINGS;
dshort_t*		openings;
dshort_t*		lastopening;


//
// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
//
dshort_t		floorclip[MAXSCREENWIDTH];
dshort_t		ceilingclip[MAXSCREENWIDTH];

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
int			spanstart[MAXSCREENHEIGHT];
int			spanstop[MAXSCREENHEIGHT];

//
// texture mapping
//
lighttable_t**		planezlight;
fixed_t			planeheight;

fixed_t			yslope[MAXSCREENHEIGHT];
fixed_t			distscale[MAXSCREENWIDTH];
fixed_t			basexscale;
fixed_t			baseyscale;
fixed_t			skyiscale;

fixed_t			cachedheight[MAXSCREENHEIGHT];
fixed_t			cacheddistance[MAXSCREENHEIGHT];
fixed_t			cachedxstep[MAXSCREENHEIGHT];
fixed_t			cachedystep[MAXSCREENHEIGHT];
fixed_t			xoffs;
fixed_t			yoffs;


extern int numflats;
extern int * flatlumps;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
  MAXVISPLANES = 128;		// Start low
  visplanes = malloc (MAXVISPLANES * sizeof (visplane_t));
  if (visplanes == NULL)
    I_Error ("Failed to claim visplane memory\n");
  memset (visplanes, 0, MAXVISPLANES * sizeof (visplane_t));
  //printf ("visplanes = %X (%X)\n", visplanes, MAXVISPLANES * sizeof (visplane_t));


  MAXOPENINGS = 10000;		// Start low
  openings = malloc (MAXOPENINGS * sizeof (openings[0]));
  if (openings == NULL)
    I_Error ("Failed to claim openings memory\n");
  //memset (openings, 0, MAXOPENINGS * sizeof (openings[0]));

  showrplanestats = (boolean) M_CheckParm ("-showrplanes");
}

static pint R_IncreaseVisplanes (void)
{
  pint 		offset;
  pint 		qty_used;
  visplane_t*	new_visplanes;

  qty_used = lastvisplane - visplanes;
  MAXVISPLANES += 128;
  // printf ("MAXVISPLANES = %u\n", MAXVISPLANES);
  new_visplanes = realloc (visplanes, MAXVISPLANES * sizeof (visplane_t));
  if (new_visplanes == NULL)
    I_Error ("R_IncreaseVisplanes: no more room");

  offset = (pint) new_visplanes - (pint) visplanes;
  visplanes = new_visplanes;
  lastvisplane = visplanes + qty_used;
  memset (lastvisplane, 0, 128 * sizeof (visplane_t));

  /* Need to move the global variable pointers while we are here */

  if (floorplane)
    floorplane = (visplane_t*)((pint) floorplane + offset);

  if (ceilingplane)
    ceilingplane = (visplane_t*)((pint) ceilingplane + offset);

  return (offset);
}

// Test: Herian2.wad Level 23 exit room
pint R_IncreaseOpenings (void)
{
  pint 		offset;
  pint		qty_used;
  dshort_t*	new_openings;

  qty_used = lastopening - openings;
  MAXOPENINGS += 10000;
  // printf ("MAXOPENINGS = %u\n", MAXOPENINGS);
  new_openings = realloc (openings, MAXOPENINGS * sizeof (openings[0]));
  if (new_openings == NULL)
    I_Error ("R_IncreaseOpenings: no more room");

  offset = (pint) new_openings - (pint) openings;
  openings = new_openings;
  lastopening = openings + qty_used;
  //memset (lastvisplane, 0, 128 * sizeof (openings[0]));
  return (offset);
}

//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//
// BASIC PRIMITIVE
//
void
R_MapPlane
( int		y,
  int		x1,
  int		x2 )
{
    angle_t	angle;
    fixed_t	distance;
    fixed_t	length;
    unsigned	index;

#ifdef RANGECHECK
    if (x2 < x1
	|| x1<0
	|| x2>=viewwidth
	|| (unsigned)y>viewheight)
    {
	I_Error ("R_MapPlane: %i, %i at %i\n",x1,x2,y);
	//return;
    }
#endif

    if (planeheight != cachedheight[y])
    {
	cachedheight[y] = planeheight;
	distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
	ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
	ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
    }
    else
    {
	distance = cacheddistance[y];
	ds_xstep = cachedxstep[y];
	ds_ystep = cachedystep[y];
    }

    length = FixedMul (distance,distscale[x1]);
    angle = (viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
    ds_xfrac = viewx + FixedMul(finecosine[angle], length) + xoffs;
    ds_yfrac = -viewy - FixedMul(finesine[angle], length) + yoffs;

    if (fixedcolormap)
	ds_colormap = fixedcolormap;
    else
    {
	index = distance >> LIGHTZSHIFT;

	if (index >= MAXLIGHTZ )
	    index = MAXLIGHTZ-1;

	ds_colormap = planezlight[index];
    }

    ds_y = y;
    ds_x1 = x1;
    ds_x2 = x2;

    // high or low detail
    spanfunc ();
}


//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes (void)
{
    int		i;
    angle_t	angle;

    // opening / clipping determination
    for (i=0 ; i<viewwidth ; i++)
    {
	floorclip[i] = viewheight;
	ceilingclip[i] = -1;
    }

    lastvisplane = visplanes;
    lastopening = openings;

    // texture calculation
    memset (cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;

    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = FixedDiv (finecosine[angle],centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],centerxfrac);
}




//
// R_FindPlane
//
visplane_t*
R_FindPlane
( fixed_t	height,
  int		picnum,
  int		lightlevel,
  fixed_t	xoffs,
  fixed_t	yoffs)
{
    int x;
    pint offset;
    visplane_t*	check;

    if ((picnum == skyflatnum) || (picnum & PL_SKYFLAT))
    {
	height = 0;			// all skys map together
	lightlevel = 0;
    }

    for (check=visplanes; check<lastvisplane; check++)
    {
	if (height == check->height
	    && picnum == check->picnum
	    && lightlevel == check->lightlevel
	    && xoffs == check->xoffs
	    && yoffs == check->yoffs)
	{
	    break;
	}
    }


    if (check < lastvisplane)
	return check;

    if (lastvisplane - visplanes >= (MAXVISPLANES-1))
    {
      offset = R_IncreaseVisplanes ();
      check = (visplane_t*)((pint) check + offset);
    }

    lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;
    check->xoffs = xoffs;
    check->yoffs = yoffs;

    // memset (check->top,0xff,sizeof(check->top));
    for (x = 0; x < SCREENWIDTH; x++)
      check->top[x] = 0xffff;

    return check;
}


//
// R_CheckPlane
//
visplane_t*
R_CheckPlane
( visplane_t*	pl,
  int		start,
  int		stop )
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;
    pint	offset;

    if (start < pl->minx)
    {
	intrl = pl->minx;
	unionl = start;
    }
    else
    {
	unionl = pl->minx;
	intrl = start;
    }

    if (stop > pl->maxx)
    {
	intrh = pl->maxx;
	unionh = stop;
    }
    else
    {
	unionh = pl->maxx;
	intrh = stop;
    }

    for (x=intrl ; x<= intrh ; x++)
	if (pl->top[x] != 0xffff)
	    break;

    if (x > intrh)
    {
	pl->minx = unionl;
	pl->maxx = unionh;

	// use the same one
	return pl;
    }

    if (lastvisplane - visplanes >= (MAXVISPLANES-1))
    {
      offset = R_IncreaseVisplanes ();
      pl = (visplane_t*)((pint) pl + offset);
    }

    // make a new visplane
    lastvisplane->height = pl->height;
    lastvisplane->picnum = pl->picnum;
    lastvisplane->lightlevel = pl->lightlevel;
    lastvisplane->xoffs = pl->xoffs;
    lastvisplane->yoffs = pl->yoffs;

    pl = lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    // memset (pl->top,0xff,sizeof(pl->top));
    for (x = 0; x < SCREENWIDTH; x++)
      pl->top[x] = 0xffff;

    return pl;
}


//
// R_MakeSpans
//
static void
R_MakeSpans
( int		x,
  int		t1,
  int		b1,
  int		t2,
  int		b2 )
{
    while (t1 < t2 && t1<=b1)
    {
	R_MapPlane (t1,spanstart[t1],x-1);
	t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
	R_MapPlane (b1,spanstart[b1],x-1);
	b1--;
    }

    while (t2 < t1 && t2<=b2)
    {
	spanstart[t2] = x;
	t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
	spanstart[b2] = x;
	b2--;
    }
}



//
// R_DrawPlanes
// At the end of each frame.
//

void R_DrawPlanes (void)
{
    visplane_t*		pl;
    int			light;
    int			i,x;
    int			stop;
    int			flip;
    int			angle;
    int			texnum;

#ifdef RANGECHECK
    if (ds_p - drawsegs > MAXDRAWSEGS)
	I_Error ("R_DrawPlanes: drawsegs overflow (%i)",
		 ds_p - drawsegs);

    if (lastvisplane - visplanes > MAXVISPLANES)
	I_Error ("R_DrawPlanes: visplane overflow (%i)",
		 lastvisplane - visplanes);

    if (lastopening - openings > MAXOPENINGS)
	I_Error ("R_DrawPlanes: opening overflow (%i)",
		 lastopening - openings);


    if (showrplanestats)
    {
#ifdef __riscos
      extern void _kernel_oswrch (int);
      _kernel_oswrch (31);
      _kernel_oswrch (0);
      _kernel_oswrch (0);
#endif
      printf ("Drawsegs = %u/%u, Visplanes = %u/%u, Openings = %u/%u\n",
		ds_p - drawsegs, MAXDRAWSEGS,
		lastvisplane - visplanes, MAXVISPLANES,
		lastopening - openings, MAXOPENINGS);
#endif
    }

    for (pl = visplanes ; pl < lastvisplane ; pl++)
    {
	if (pl->minx > pl->maxx)
	    continue;

	x = pl->picnum;

	// sky flat
	if ((x == skyflatnum) || (pl->picnum & PL_SKYFLAT))
	{
	    dc_iscale = skyiscale;
	    // Sky is allways drawn full bright,
	    //  i.e. colormaps[0] is used.
	    // Because of this hack, sky is not affected
	    //  by INVUL inverse mapping.
		  // BRAD: So I fix it....
	    // See http://doom.wikia.com/wiki/Invulnerability_colormap_bug
	    //dc_colormap = colormaps;
	    dc_colormap = (fixedcolormap ? fixedcolormap : colormaps);
	    dc_texturemid = skytexturemid;
	    if (x & PL_SKYFLAT)
	    {
	      // Sky Linedef
	      const line_t *l = &lines[x & ~PL_SKYFLAT];

	      // Sky transferred from first sidedef
	      const side_t *s = *l->sidenum + sides;

	      // Texture comes from upper texture of reference sidedef
	      texnum = texturetranslation[s->toptexture];

	      // We sometimes flip the picture horizontally.
	      //
	      // Doom always flipped the picture, so we make it optional,
	      // to make it easier to use the new feature, while to still
	      // allow old sky textures to be used.
	      
	      flip = l->special==272 ? 0u : ~0u;
	    }
	    else
	    {
	      texnum = skytexture;
	      flip = 0;
	    }
	    for (x=pl->minx ; x <= pl->maxx ; x++)
	    {
		dc_yl = pl->top[x];
		dc_yh = pl->bottom[x];

		if (dc_yl <= dc_yh)
		{
		    angle = ((viewangle + xtoviewangle[x])^flip)>>ANGLETOSKYSHIFT;
		    dc_x = x;
		    dc_source = R_GetColumn(texnum, angle);
		    colfunc ();
		}
	    }
	    continue;
	}

	// regular flat

	if ((unsigned)x >= (unsigned)numflats)
	{
	   // Emergency fix up - Eternal.wad Level 29.  JAD 7/12/98
	   // Added (unsigned) for Freedoom2 Level 19.	JAD 1/03/14
	  i = R_CheckFlatNumForName ("FLOOR5_3");
	  if (i == -1)
	    i = 1;
	}
	else
	{
	  i = flatlumps [flattranslation [x]];
	}

	ds_source = W_CacheLumpNum (i, PU_STATIC);

	xoffs = pl->xoffs;
	yoffs = pl->yoffs;
	planeheight = abs(pl->height-viewz);
	light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;

	if (light >= LIGHTLEVELS)
	    light = LIGHTLEVELS-1;

	if (light < 0)
	    light = 0;

	planezlight = zlight[light];

	pl->top[pl->maxx+1] = 0xffff;
	pl->top[pl->minx-1] = 0xffff;

	stop = pl->maxx + 1;

	for (x=pl->minx ; x<= stop ; x++)
	{
	    R_MakeSpans(x,pl->top[x-1],
			pl->bottom[x-1],
			pl->top[x],
			pl->bottom[x]);
	}

	Z_ChangeTag (ds_source, PU_CACHE);
    }
}

//----------------------------------------------------------------------------
