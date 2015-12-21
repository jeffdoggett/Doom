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
static int		spanstart[MAXSCREENHEIGHT];

//
// texture mapping
//
lighttable_t**		planezlight;
fixed_t			planeheight;

fixed_t			yslope[MAXSCREENHEIGHT];
fixed_t			distscale[MAXSCREENWIDTH];
fixed_t			basexscale;
fixed_t			baseyscale;

fixed_t			xoffs;
fixed_t			yoffs;


extern int numflats;
extern int * flatlumps;

#define PMARKER	0xFFFF

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

    // left to right mapping
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;

    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = FixedDiv (finecosine[angle],centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],centerxfrac);
}

//-----------------------------------------------------------------------------

static uintptr_t R_IncreaseVisplanes (void)
{
  uintptr_t 	offset;
  uintptr_t 	qty_used;
  visplane_t*	new_visplanes;

  qty_used = lastvisplane - visplanes;
  MAXVISPLANES += 128;
  // printf ("MAXVISPLANES = %u\n", MAXVISPLANES);
  new_visplanes = realloc (visplanes, MAXVISPLANES * sizeof (visplane_t));
  if (new_visplanes == NULL)
    I_Error ("R_IncreaseVisplanes: no more room");

  offset = (uintptr_t) new_visplanes - (uintptr_t) visplanes;
  visplanes = new_visplanes;
  lastvisplane = visplanes + qty_used;
  memset (lastvisplane, 0, 128 * sizeof (visplane_t));

  /* Need to move the global variable pointers while we are here */

  if (floorplane)
    floorplane = (visplane_t*)((uintptr_t) floorplane + offset);

  if (ceilingplane)
    ceilingplane = (visplane_t*)((uintptr_t) ceilingplane + offset);

  return (offset);
}

//-----------------------------------------------------------------------------
// Test: Herian2.wad Level 23 exit room
uintptr_t R_IncreaseOpenings (size_t need)
{
  uintptr_t 	offset;
  uintptr_t	qty_used;
  dshort_t*	new_openings;
  drawseg_t	*ds;		// jff 8/9/98 needed for fix from ZDoom

  MAXOPENINGS = need;
//printf ("MAXOPENINGS = %u\n", MAXOPENINGS);
  new_openings = realloc (openings, MAXOPENINGS * sizeof (openings[0]));
  if (new_openings == NULL)
    I_Error ("R_IncreaseOpenings: no more room");

  offset = (uintptr_t) new_openings - (uintptr_t) openings;
  // printf ("old = %X, new = %X, offset = %X\n", openings, new_openings, offset);

  // jff 8/9/98 borrowed fix for openings from ZDOOM1.14
  // [RH] We also need to adjust the openings pointers that
  //    were already stored in drawsegs.
  for (ds = drawsegs; ds < ds_p; ds++)
  {
    if (ds->maskedtexturecol + ds->x1 >= openings
     && ds->maskedtexturecol + ds->x1 <= lastopening)
    {
      ds->maskedtexturecol = (dshort_t*) ((uintptr_t) ds->maskedtexturecol + offset);
    }
    if (ds->sprtopclip + ds->x1 >= openings
     && ds->sprtopclip + ds->x1 <= lastopening)
    {
      ds->sprtopclip = (dshort_t*) ((uintptr_t) ds->sprtopclip + offset);
    }
    if (ds->sprbottomclip + ds->x1 >= openings
     && ds->sprbottomclip + ds->x1 <= lastopening)
    {
      ds->sprbottomclip = (dshort_t*) ((uintptr_t) ds->sprbottomclip + offset);
    }
  }

  qty_used = lastopening - openings;
  openings = new_openings;
  lastopening = openings + qty_used;
  //memset (lastvisplane, 0, 128 * sizeof (openings[0]));
  return (offset);
}

//-----------------------------------------------------------------------------
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
static void R_MapPlane (int y, int x1, int x2)
{
    fixed_t     distance;
    int         dx, dy;
    unsigned	index;

    if (y == centery)
        return;

#ifdef RANGECHECK
    if (x2 < x1
	|| x1<0
	|| x2>=viewwidth
	|| (unsigned)y>viewheight)
    {
	//I_Error ("R_MapPlane: %i, %i at %i\n",x1,x2,y);
	printf ("R_MapPlane: %i, %i at %i\n",x1,x2,y);
	return;
    }
#endif

    distance = FixedMul(planeheight, yslope[y]);

    dx = x1 - centerx;
    dy = ABS(centery - y);
    ds_xstep = FixedMul(viewsin, planeheight) / dy;
    ds_ystep = FixedMul(viewcos, planeheight) / dy;

    ds_xfrac = viewx + xoffs + FixedMul(viewcos, distance) + dx * ds_xstep;
    ds_yfrac = -viewy + yoffs - FixedMul(viewsin, distance) + dx * ds_ystep;

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

    spanfunc();
}

//-----------------------------------------------------------------------------
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
    uintptr_t offset;
    visplane_t*	check;

    if ((picnum == skyflatnum) || (picnum & PL_SKYFLAT))
    {
	height = 0;			// all skies map together
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
	  return check;
	}
    }

    if (lastvisplane >= &visplanes[MAXVISPLANES])
    {
      offset = R_IncreaseVisplanes ();
      check = (visplane_t*)((uintptr_t) check + offset);
    }

    lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = viewwidth;		// Was SCREENWIDTH -- killough 11/98
    check->maxx = -1;
    check->xoffs = xoffs;
    check->yoffs = yoffs;

    // memset (check->top,0xff,sizeof(check->top));
    for (x = 0; x < ARRAY_SIZE(check->top); x++)
      check->top[x] = PMARKER;

    return check;
}

//-----------------------------------------------------------------------------
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
    uintptr_t	offset;

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

    for (x = intrl; (x <= intrh) && (pl->top[x+1] == PMARKER); ++x);

    // [crispy] fix HOM if ceilingplane and floorplane are the same
    // visplane (e.g. both skies)
    if (!(pl == floorplane && markceiling && floorplane == ceilingplane) && x > intrh)
    {
	pl->minx = unionl;
	pl->maxx = unionh;
    }
    else
    {
      visplane_t * new_pl;

      if (lastvisplane >= &visplanes[MAXVISPLANES])
      {
	offset = R_IncreaseVisplanes ();
	pl = (visplane_t*)((uintptr_t) pl + offset);
      }

      // make a new visplane
      new_pl = lastvisplane++;
      new_pl->height = pl->height;
      new_pl->picnum = pl->picnum;
      new_pl->lightlevel = pl->lightlevel;
      new_pl->xoffs = pl->xoffs;
      new_pl->yoffs = pl->yoffs;
      pl = new_pl;
      pl->minx = start;
      pl->maxx = stop;

      // memset (pl->top,0xff,sizeof(pl->top));
      for (x = 0; x < ARRAY_SIZE(pl->top); x++)
	pl->top[x] = PMARKER;
    }

    return (pl);
}

//-----------------------------------------------------------------------------
//
// R_MakeSpans
//
static void R_MakeSpans (visplane_t *pl)
{
  int x;

  for (x = pl->minx; x <= pl->maxx + 1; ++x)
  {
    dushort_t  t1 = pl->top[x];
    dushort_t  b1 = pl->bottom[x];
    dushort_t  t2 = pl->top[x+1];
    dushort_t  b2 = pl->bottom[x+1];

    for (; t1 < t2 && t1 <= b1; ++t1)
      R_MapPlane(t1, spanstart[t1], x - 1);
    for (; b1 > b2 && b1 >= t1; --b1)
      R_MapPlane(b1, spanstart[b1], x - 1);
    while (t2 < t1 && t2 <= b2)
      spanstart[t2++] = x;
    while (b2 > b1 && b2 >= t2)
      spanstart[b2--] = x;
  }
}

//-----------------------------------------------------------------------------
//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes(void)
{
  int i;
  visplane_t *pl;

#ifdef RANGECHECK
  if (ds_p > &drawsegs[MAXDRAWSEGS])
      I_Error ("R_DrawPlanes: drawsegs overflow (%i)",
	       ds_p - drawsegs);

  if (lastvisplane > &visplanes[MAXVISPLANES])
      I_Error ("R_DrawPlanes: visplane overflow (%i)",
	       lastvisplane - visplanes);

  if (lastopening > &openings[MAXOPENINGS])
      I_Error ("R_DrawPlanes: opening overflow (%i)",
	       lastopening - openings);
#endif

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
  }

  for (pl = visplanes ; pl < lastvisplane ; pl++)
  {
    if (pl->minx <= pl->maxx)
    {
      int picnum = pl->picnum;

      // sky flat
      if (picnum == skyflatnum || (picnum & PL_SKYFLAT))
      {
	int	 x;
	int	 texture;
	int	 offset;
	angle_t an, flip;

	// killough 10/98: allow skies to come from sidedefs.
	// Allows scrolling and/or animated skies, as well as
	// arbitrary multiple skies per level without having
	// to use info lumps.
	an = viewangle;

	if (picnum & PL_SKYFLAT)
	{
	  // Sky Linedef
	  const line_t *l = &lines[picnum & ~PL_SKYFLAT];

	  // Sky transferred from first sidedef
	  const side_t *s = *l->sidenum + sides;

	  // Texture comes from upper texture of reference sidedef
	  texture = texturetranslation[s->toptexture];

	  // Horizontal offset is turned into an angle offset,
	  // to allow sky rotation as well as careful positioning.
	  // However, the offset is scaled very small, so that it
	  // allows a long-period of sky rotation.
	  an += s->textureoffset;

	  // Vertical offset allows careful sky positioning.
	  dc_texturemid = s->rowoffset - 28 * FRACUNIT;

	  // We sometimes flip the picture horizontally.
	  //
	  // DOOM always flipped the picture, so we make it optional,
	  // to make it easier to use the new feature, while to still
	  // allow old sky textures to be used.
	  flip = l->special==272 ? 0u : ~0u;
	}
	else	// Normal DOOM sky, only one allowed per level
	{
	  dc_texturemid = skytexturemid;	// Default y-offset
	  texture = skytexture;			// Default texture
	  flip = 0;				// DOOM flips it
	}

	// Sky is always drawn full bright,
	//  i.e. colormaps[0] is used.
	// Because of this hack, sky is not affected
	//  by INVUL inverse mapping.
	dc_colormap = (fixedcolormap ? fixedcolormap : colormaps);

	dc_ylim = textureheight[texture];
	dc_iscale = skyiscale;

	offset = skycolumnoffset >> FRACBITS;

	for (x = pl->minx; x <= pl->maxx; x++)
	{
	    dc_yl = pl->top[x+1];
	    dc_yh = pl->bottom[x+1];

	    if (dc_yl <= dc_yh)
	    {
		dc_x = x;
		dc_source = R_GetColumn(texture, (((an + xtoviewangle[x]) ^ flip)
		    >> ANGLETOSKYSHIFT) + offset, false);
		colfunc();
	    }
	}
      }
      else
      {
	 // regular flat
	 int light;

	 if ((unsigned)picnum >= (unsigned)numflats)
	 {
	   // Emergency fix up - Eternal.wad Level 29. JAD 7/12/98
	   // Added (unsigned) for Freedoom2 Level 19. JAD 1/03/14
	   i = R_CheckFlatNumForName ("FLOOR5_3");
	   if (i == -1)
	     i = 1;
	 }
	 else
	 {
	   i = flatlumps [flattranslation [picnum]];
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

	 pl->top[pl->maxx+2] = PMARKER;
	 pl->top[pl->minx+0] = PMARKER;

	 R_MakeSpans (pl);

	 Z_ChangeTag (ds_source, PU_CACHE);
      }
    }
  }
}

//----------------------------------------------------------------------------
