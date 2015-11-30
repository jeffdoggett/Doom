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
//	All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: r_segs.c,v 1.3 1997/01/29 20:10:19 b1 Exp $";
#endif

#include "includes.h"

// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
boolean		segtextured;

// False if the back side is the same plane.
boolean		markfloor;
boolean		markceiling;

static boolean	maskedtexture;
static int	toptexture;
static int	bottomtexture;
static int	midtexture;


angle_t		rw_normalangle;
// angle to line origin
int		rw_angle1;

//
// regular wall
//
static int	rw_x;
static int	rw_stopx;
static angle_t	rw_centerangle;
static fixed_t	rw_offset;
static fixed_t	rw_distance;
static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

static int	worldtop;
static int	worldbottom;
static int	worldhigh;
static int	worldlow;

static fixed_t	pixhigh;
static fixed_t	pixlow;
static fixed_t	pixhighstep;
static fixed_t	pixlowstep;

static fixed_t	topfrac;
static fixed_t	topstep;

static fixed_t	bottomfrac;
static fixed_t	bottomstep;


lighttable_t**	walllights;

static dshort_t* maskedtexturecol;


//
// R_FixWiggle()
// Dynamic wall/texture rescaler, AKA "WiggleHack II"
//  by Kurt "kb1" Baumgardner ("kb") and Andrey "Entryway" Budko ("e6y")
//
//  [kb] When the rendered view is positioned, such that the viewer is
//   looking almost parallel down a wall, the result of the scale
//   calculation in R_ScaleFromGlobalAngle becomes very large. And, the
//   taller the wall, the larger that value becomes. If these large
//   values were used as-is, subsequent calculations would overflow,
//   causing full-screen HOM, and possible program crashes.
//
//  Therefore, vanilla Doom clamps this scale calculation, preventing it
//   from becoming larger than 0x400000 (64*FRACUNIT). This number was
//   chosen carefully, to allow reasonably-tight angles, with reasonably
//   tall sectors to be rendered, within the limits of the fixed-point
//   math system being used. When the scale gets clamped, Doom cannot
//   properly render the wall, causing an undesirable wall-bending
//   effect that I call "floor wiggle". Not a crash, but still ugly.
//
//  Modern source ports offer higher video resolutions, which worsens
//   the issue. And, Doom is simply not adjusted for the taller walls
//   found in many PWADs.
//
//  This code attempts to correct these issues, by dynamically
//   adjusting the fixed-point math, and the maximum scale clamp,
//   on a wall-by-wall basis. This has 2 effects:
//
//  1. Floor wiggle is greatly reduced and/or eliminated.
//  2. Overflow is no longer possible, even in levels with maximum
//     height sectors (65535 is the theoretical height, though Doom
//     cannot handle sectors > 32767 units in height.
//
//  The code is not perfect across all situations. Some floor wiggle can
//   still be seen, and some texture strips may be slightly misaligned in
//   extreme cases. These effects cannot be corrected further, without
//   increasing the precision of various renderer variables, and,
//   possibly, creating a noticable performance penalty.
//

static int	max_rwscale = 64 * FRACUNIT;
static int	heightbits = 12;
static int	heightunit = (1 << 12);
static int	invhgtbits = 4;

typedef struct
{
  int clamp;
  int heightbits;
} scale_values_t;

static const scale_values_t scale_values [] =
{
  {2048 * FRACUNIT, 12}, {1024 * FRACUNIT, 12},
  {1024 * FRACUNIT, 11}, { 512 * FRACUNIT, 11},
  { 512 * FRACUNIT, 10}, { 256 * FRACUNIT, 10},
  { 256 * FRACUNIT,  9}, { 128 * FRACUNIT,  9}
};

static void R_FixWiggle (sector_t *sector)
{
  static int		lastheight = 0;
  int			height = (sector->ceilingheight - sector->floorheight) >> FRACBITS;
  int			scaleindex;
  const scale_values_t *svp;

  // disallow negative heights. using 1 forces cache initialization
  if (height < 1)
    height = 1;

  // early out?
  if (height != lastheight)
  {
    lastheight = height;

    // initialize, or handle moving sector
    if (height != sector->cachedheight)
    {
      sector->cachedheight = height;
      scaleindex = 0;
      height >>= 7;

      // calculate adjustment
      while (height >>= 1)
	scaleindex++;

      sector->scaleindex = scaleindex;
    }

    // fine-tune renderer for this wall
    svp = &scale_values[sector->scaleindex];
    max_rwscale = svp -> clamp;
    heightbits = svp -> heightbits;
    heightunit = (1 << heightbits);
    invhgtbits = FRACBITS - heightbits;
  }
}

//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
static fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
    fixed_t		scale;
    int			anglea;
    int			angleb;
    int			sinea;
    int			sineb;
    fixed_t		num;
    int			den;

    // UNUSED
#if 0
{
    fixed_t		dist;
    fixed_t		z;
    fixed_t		sinv;
    fixed_t		cosv;

    sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
    dist = FixedDiv (rw_distance, sinv);
    cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
    z = abs(FixedMul (dist, cosv));
    scale = FixedDiv(projection, z);
    return scale;
}
#endif

    anglea = ANG90 + (visangle-viewangle);
    angleb = ANG90 + (visangle-rw_normalangle);

    // both sines are allways positive
    sinea = finesine[anglea>>ANGLETOFINESHIFT];
    sineb = finesine[angleb>>ANGLETOFINESHIFT];
    num = FixedMul(projection,sineb)<<detailshift;
    den = FixedMul(rw_distance,sinea);

    if (den > (num >> 16))
    {
      scale = FixedDiv(num, den);

      // [kb] When this evaluates True, the scale is clamped,
      //  and there will be some wiggling.
      if (scale > max_rwscale)
	scale = max_rwscale;
      else if (scale < 256)
	scale = 256;
    }
    else
      scale = max_rwscale;

    return (scale);
}


//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
    unsigned	index;
    column_t*	col;
    int		lightnum;
    int		texnum;
    unsigned int tex;

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;
    tex = curline->sidedef->midtexture;
    if (tex >= numtextures) tex = 0;
    texnum = texturetranslation[tex];

    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (curline->v1->y == curline->v2->y)
	lightnum -= LIGHTBRIGHT;
    else if (curline->v1->x == curline->v2->x)
	lightnum += LIGHTBRIGHT;

    if (lightnum < 0)
	walllights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	walllights = scalelight[LIGHTLEVELS-1];
    else
	walllights = scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->scalestep;
    spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
    mfloorclip = ds->sprbottomclip;
    mceilingclip = ds->sprtopclip;

    // find positioning
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
    {
	dc_texturemid = frontsector->floorheight > backsector->floorheight
	    ? frontsector->floorheight : backsector->floorheight;
	dc_texturemid = dc_texturemid + textureheight[texnum] - viewz;
    }
    else
    {
	dc_texturemid =frontsector->ceilingheight<backsector->ceilingheight
	    ? frontsector->ceilingheight : backsector->ceilingheight;
	dc_texturemid = dc_texturemid - viewz;
    }
    dc_texturemid += curline->sidedef->rowoffset;

    if (fixedcolormap)
	dc_colormap = fixedcolormap;

    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
	// calculate lighting
	if (maskedtexturecol[dc_x] != MAXDSHORT)
	{
	    if (!fixedcolormap)
	    {
		index = spryscale>>lightScaleShift;

		if (index >=  MAXLIGHTSCALE )
		    index = MAXLIGHTSCALE-1;

		dc_colormap = walllights[index];
	    }


	    // killough 3/2/98:
	    //
	    // This calculation used to overflow and cause crashes in Doom:
	    //
	    // sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
	    //
	    // This code fixes it, by using double-precision intermediate
	    // arithmetic and by skipping the drawing of 2s normals whose
	    // mapping to screen coordinates is totally out of range:

	    {
	      int64_t t = ((int64_t) centeryfrac << FRACBITS) -
		(int64_t) dc_texturemid * spryscale;

	      if (t + (int64_t) textureheight[texnum] * spryscale < 0 ||
		  t > (int64_t) SCREENHEIGHT << FRACBITS*2)
		continue;	// skip if the texture is out of screen's range

	      sprtopscreen = (fixed_t)(t >> FRACBITS);
	    }

	    dc_iscale = 0xffffffffu / (unsigned)spryscale;

	    // draw the texture
	    col = (column_t *)(
		(byte *)R_GetColumn(texnum,maskedtexturecol[dc_x],false) -3);

	    R_DrawMaskedColumn (col);
	    maskedtexturecol[dc_x] = MAXDSHORT;
	}
	spryscale += rw_scalestep;
    }

}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

void R_RenderSegLoop (void)
{
    angle_t		angle;
    unsigned		index;
    int			yl;
    int			yh;
    int			mid;
    fixed_t		texturecolumn;
    int			top;
    int			bottom;

    //texturecolumn = 0;				// shut up compiler warning

    for ( ; rw_x < rw_stopx ; rw_x++)
    {
	// mark floor / ceiling areas
	yl = (topfrac+heightunit-1)>>heightbits;

	// no space above wall?
	if (yl < ceilingclip[rw_x]+1)
	    yl = ceilingclip[rw_x]+1;

	if (markceiling)
	{
	    top = ceilingclip[rw_x]+1;
	    bottom = yl-1;

	    if (bottom >= floorclip[rw_x])
		bottom = floorclip[rw_x]-1;

	    if (top <= bottom)
	    {
		ceilingplane->top[rw_x] = top;
		ceilingplane->bottom[rw_x] = bottom;
	    }
	}

	yh = bottomfrac>>heightbits;

	if (yh >= floorclip[rw_x])
	    yh = floorclip[rw_x]-1;

	if (markfloor)
	{
	    top = yh+1;
	    bottom = floorclip[rw_x]-1;
	    if (top <= ceilingclip[rw_x])
		top = ceilingclip[rw_x]+1;
	    if (top <= bottom)
	    {
		floorplane->top[rw_x] = top;
		floorplane->bottom[rw_x] = bottom;
	    }
	}

	// texturecolumn and lighting are independent of wall tiers
	if (segtextured)
	{
	    // calculate texture offset
	    angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
	    texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
	    texturecolumn >>= FRACBITS;
	    // calculate lighting
	    index = rw_scale>>lightScaleShift;

	    if (index >=  MAXLIGHTSCALE )
		index = MAXLIGHTSCALE-1;

	    dc_colormap = walllights[index];
	    dc_x = rw_x;
	    dc_iscale = 0xffffffffu / (unsigned)rw_scale;
	}

	// draw the wall tiers
	if (midtexture)
	{
	    // single sided line
	    dc_yl = yl;
	    dc_yh = yh;
	    dc_texturemid = rw_midtexturemid;
	    dc_source = R_GetColumn(midtexture,texturecolumn,true);
	    colfunc ();
	    ceilingclip[rw_x] = viewheight;
	    floorclip[rw_x] = -1;
	}
	else
	{
	    // two sided line
	    if (toptexture)
	    {
		// top wall
		mid = pixhigh>>heightbits;
		pixhigh += pixhighstep;

		if (mid >= floorclip[rw_x])
		    mid = floorclip[rw_x]-1;

		if (mid >= yl)
		{
		    dc_yl = yl;
		    dc_yh = mid;
		    dc_texturemid = rw_toptexturemid;
		    dc_source = R_GetColumn(toptexture,texturecolumn,true);
		    colfunc ();
		    ceilingclip[rw_x] = mid;
		}
		else
		    ceilingclip[rw_x] = yl-1;
	    }
	    else
	    {
		// no top wall
		if (markceiling)
		    ceilingclip[rw_x] = yl-1;
	    }

	    if (bottomtexture)
	    {
		// bottom wall
		mid = (pixlow+heightunit-1)>>heightbits;
		pixlow += pixlowstep;

		// no space above wall?
		if (mid <= ceilingclip[rw_x])
		    mid = ceilingclip[rw_x]+1;

		if (mid <= yh)
		{
		    dc_yl = mid;
		    dc_yh = yh;
		    dc_texturemid = rw_bottomtexturemid;
		    dc_source = R_GetColumn(bottomtexture,texturecolumn,true);
		    colfunc ();
		    floorclip[rw_x] = mid;
		}
		else
		    floorclip[rw_x] = yh+1;
	    }
	    else
	    {
		// no bottom wall
		if (markfloor)
		    floorclip[rw_x] = yh+1;
	    }

	    if (maskedtexture)
	    {
		// save texturecol
		//  for backdrawing of masked mid texture
		maskedtexturecol[rw_x] = texturecolumn;
	    }
	}

	rw_scale += rw_scalestep;
	topfrac += topstep;
	bottomfrac += bottomstep;
    }
#ifdef RANGECHECK_REMOVED
    if ((dc_yl < 0)
     || (dc_yh >= SCREENHEIGHT))
	I_Error ("R_RenderSegLoop: %i to %i", dc_yl, dc_yh);
#endif
}



//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void R_StoreWallRange(int start, int stop)
{
    unsigned int	pos;
    unsigned int	tex;
    int			timesofar;
    int64_t     dx, dy, dx1, dy1, len;

    pos = (unsigned int) (ds_p - drawsegs);

    // don't overflow and crash
    if (pos >= MAXDRAWSEGS)
    {
	return;
    }

    if ((pos & 0xFF) == 0)
    {
      timesofar = I_GetTime ();
      if ((timesofar - drawsegstarttime) > 5)
      {
	// printf ("R_StoreWallRange timed out at %u\n", ds_p-drawsegs);
	return;
      }
    }

    linedef = curline->linedef;

    // mark the segment as visible for automap
    linedef->flags |= ML_MAPPED;

    // [BH] if in automap, we're done now that line is mapped
    if (automapactive)
	return;

    sidedef = curline->sidedef;

    // calculate rw_distance for scale calculation
    rw_normalangle = curline->angle + ANG90;

    // [Linguica] Fix long wall error
    // shift right to avoid possibility of int64 overflow in rw_distance calculation
    dx = ((int64_t)curline->v2->x - curline->v1->x) >> 1;
    dy = ((int64_t)curline->v2->y - curline->v1->y) >> 1;
    dx1 = ((int64_t)viewx - curline->v1->x) >> 1;
    dy1 = ((int64_t)viewy - curline->v1->y) >> 1;
    len = curline->length >> 1;
    rw_distance = (fixed_t)((dy * dx1 - dx * dy1) / len) << 1;

    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop + 1;

    // killough 1/6/98, 2/1/98: remove limit on openings
    {
	size_t	  opos = lastopening - openings;
	size_t	  need = (rw_stopx - start) * sizeof(*lastopening) + opos;

	if (need > MAXOPENINGS)
	  R_IncreaseOpenings (need);
    }

    worldtop = frontsector->ceilingheight - viewz;
    worldbottom = frontsector->floorheight - viewz;

    R_FixWiggle (frontsector);

    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[start]);

    if (stop > start)
    {
	ds_p->scale2 = R_ScaleFromGlobalAngle(viewangle + xtoviewangle[stop]);
	ds_p->scalestep = rw_scalestep = (ds_p->scale2 - rw_scale) / (stop - start);
    }
    else
	ds_p->scale2 = ds_p->scale1;

    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    midtexture = 0;
    toptexture = 0;
    bottomtexture = 0;
    maskedtexture = false;
    ds_p->maskedtexturecol = NULL;

    if (!backsector)
    {
	// single sided line
	tex = sidedef->midtexture;
	if (tex >= numtextures) tex = 0;
	midtexture = texturetranslation[tex];
//      midtexheight = textureheight[midtexture] >> FRACBITS;
//      midtexfullbright = texturefullbright[midtexture];

	// a single sided line is terminal, so it must mark ends
	markfloor = markceiling = true;

	if (linedef->flags & ML_DONTPEGBOTTOM)
	    // bottom of texture at bottom
	    rw_midtexturemid = frontsector->floorheight + textureheight[sidedef->midtexture]
		- viewz + sidedef->rowoffset;
	else
	    // top of texture at top
	    rw_midtexturemid = worldtop + sidedef->rowoffset;

	{
	    // killough 3/27/98: reduce offset
	    fixed_t     h = textureheight[midtexture];

	    if (h & (h - FRACUNIT))
		rw_midtexturemid %= h;
	}

	ds_p->silhouette = SIL_BOTH;
	ds_p->sprtopclip = screenheightarray;
	ds_p->sprbottomclip = negonearray;
	ds_p->bsilheight = MAXINT;
	ds_p->tsilheight = MININT;
    }
    else
    {
	// two sided line
	ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
	ds_p->silhouette = 0;

	if (frontsector->floorheight > backsector->floorheight)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = frontsector->floorheight;
	}
	else if (backsector->floorheight > viewz)
	{
	    ds_p->silhouette = SIL_BOTTOM;
	    ds_p->bsilheight = MAXINT;
	}

	if (frontsector->ceilingheight < backsector->ceilingheight)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = frontsector->ceilingheight;
	}
	else if (backsector->ceilingheight < viewz)
	{
	    ds_p->silhouette |= SIL_TOP;
	    ds_p->tsilheight = MININT;
	}

	// killough 1/17/98: this test is required if the fix
	// for the automap bug (r_bsp.c) is used, or else some
	// sprites will be displayed behind closed doors. That
	// fix prevents lines behind closed doors with dropoffs
	// from being displayed on the automap.
	//
	// killough 4/7/98: make doorclosed external variable
	{
	    extern int doorclosed;

	    if (doorclosed || backsector->ceilingheight <= frontsector->floorheight)
	    {
		ds_p->sprbottomclip = negonearray;
		ds_p->bsilheight = MAXINT;
		ds_p->silhouette |= SIL_BOTTOM;
	    }

	    if (doorclosed || backsector->floorheight >= frontsector->ceilingheight)
	    {
		ds_p->sprtopclip = screenheightarray;
		ds_p->tsilheight = MININT;
		ds_p->silhouette |= SIL_TOP;
	    }
	}

	worldhigh = backsector->ceilingheight - viewz;
	worldlow = backsector->floorheight - viewz;

	// hack to allow height changes in outdoor areas
	if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
	    worldtop = worldhigh;

	markfloor = (boolean)(worldlow != worldbottom
	    || backsector->floorpic != frontsector->floorpic
	    || backsector->lightlevel != frontsector->lightlevel

	    // killough 3/7/98: Add checks for (x,y) offsets
	    || backsector->floor_xoffs != frontsector->floor_xoffs
	    || backsector->floor_yoffs != frontsector->floor_yoffs

	    // killough 4/15/98: prevent 2s normals
	    // from bleeding through deep water
	    || frontsector->heightsec != -1

	    // killough 4/17/98: draw floors if different light levels
	    || backsector->floorlightsec != frontsector->floorlightsec);

	markceiling = (boolean)(worldhigh != worldtop
	    || backsector->ceilingpic != frontsector->ceilingpic
	    || backsector->lightlevel != frontsector->lightlevel

	    // killough 3/7/98: Add checks for (x,y) offsets
	    || backsector->ceiling_xoffs != frontsector->ceiling_xoffs
	    || backsector->ceiling_yoffs != frontsector->ceiling_yoffs

	    // killough 4/15/98: prevent 2s normals
	    // from bleeding through fake ceilings
	    || (frontsector->heightsec != -1 && frontsector->ceilingpic != skyflatnum)

	    // killough 4/17/98: draw ceilings if different light levels
	    || backsector->ceilinglightsec != frontsector->ceilinglightsec);

	if (backsector->ceilingheight <= frontsector->floorheight
	    || backsector->floorheight >= frontsector->ceilingheight)
	    // closed door
	    markceiling = markfloor = true;

	if (worldhigh < worldtop)
	{
	  int toptexheight;

	    // top texture
	    tex = sidedef->toptexture;
	    if (tex >= numtextures) tex = 0;
	    toptexture = texturetranslation[tex];
	    toptexheight = textureheight[toptexture] >> FRACBITS;
//	    toptexfullbright = texturefullbright[toptexture];

	    if (linedef->flags & ML_DONTPEGTOP)
		// top of texture at top
		rw_toptexturemid = worldtop;
	    else
		// bottom of texture
		rw_toptexturemid = backsector->ceilingheight + toptexheight - viewz;

	    rw_toptexturemid += sidedef->rowoffset;

	    // killough 3/27/98: reduce offset
	    {
		fixed_t     h = textureheight[toptexture];

		if (h & (h - FRACUNIT))
		    rw_toptexturemid %= h;
	    }
	}

	if (worldlow > worldbottom)
	{
	    // bottom texture
	    tex = sidedef->bottomtexture;
	    if (tex >= numtextures) tex = 0;
	    bottomtexture = texturetranslation[tex];
//	    bottomtexheight = textureheight[bottomtexture] >> FRACBITS;
//	    bottomtexfullbright = texturefullbright[bottomtexture];

	    if (linedef->flags & ML_DONTPEGBOTTOM)
		// bottom of texture at bottom, top of texture at top
		rw_bottomtexturemid = worldtop;
	    else	// top of texture at top
		rw_bottomtexturemid = worldlow;

	    rw_bottomtexturemid += sidedef->rowoffset;

	    // killough 3/27/98: reduce offset
	    {
		fixed_t     h = textureheight[bottomtexture];

		if (h & (h - FRACUNIT))
		    rw_bottomtexturemid %= h;
	    }
	}

	// allocate space for masked texture tables
	if (sidedef->midtexture)
	{
	    // masked midtexture
	    maskedtexture = true;
	    ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
	    lastopening += rw_stopx - rw_x;
	}
    }

    // calculate rw_offset (only needed for textured lines)
    segtextured = (boolean)(midtexture | toptexture | bottomtexture | maskedtexture);

    if (segtextured)
    {
	rw_offset = (fixed_t)(((dx * dx1 + dy * dy1) / len) << 1) + sidedef->textureoffset
	    + curline->offset;

	rw_centerangle = ANG90 + viewangle - rw_normalangle;

	// calculate light table
	//  use different light tables
	//  for horizontal / vertical / diagonal
	if (!fixedcolormap)
	{
	    int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT) + extralight * LIGHTBRIGHT;

	    if (frontsector->ceilingpic != skyflatnum)
	    {
		if (curline->v1->y == curline->v2->y)
		    lightnum -= LIGHTBRIGHT;
		else if (curline->v1->x == curline->v2->x)
		    lightnum += LIGHTBRIGHT;
	    }

	    if (lightnum < 0)
		walllights = scalelight[0];
	    else if (lightnum >= LIGHTLEVELS)
		walllights = scalelight[LIGHTLEVELS-1];
	    else
		walllights = scalelight[lightnum];
	}
    }

    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    // killough 3/7/98: add deep water check
    if (frontsector->heightsec == -1)
    {
	if (frontsector->floorheight >= viewz)
	    markfloor = false;	  // above view plane

	if (frontsector->ceilingheight <= viewz && frontsector->ceilingpic != skyflatnum)
	    markceiling = false;	// below view plane
    }

    // calculate incremental stepping values for texture edges
    worldtop >>= invhgtbits;
    worldbottom >>= invhgtbits;

    topstep = -FixedMul(rw_scalestep, worldtop);
    topfrac = (fixed_t)(((int64_t)centeryfrac >> invhgtbits) - (((int64_t)worldtop * rw_scale) >> FRACBITS));

    bottomstep = -FixedMul(rw_scalestep, worldbottom);
    bottomfrac = (fixed_t)(((int64_t)centeryfrac >> invhgtbits)
	- (((int64_t)worldbottom * rw_scale) >> FRACBITS));

    if (backsector)
    {
	worldhigh >>= invhgtbits;
	worldlow >>= invhgtbits;

	if (worldhigh < worldtop)
	{
	    pixhigh = (fixed_t)(((int64_t)centeryfrac >> invhgtbits)
		- (((int64_t)worldhigh * rw_scale) >> FRACBITS));
	    pixhighstep = -FixedMul(rw_scalestep, worldhigh);
	}

	if (worldlow > worldbottom)
	{
	    pixlow = (fixed_t)(((int64_t)centeryfrac >> invhgtbits)
		- (((int64_t)worldlow * rw_scale) >> FRACBITS));
	    pixlowstep = -FixedMul(rw_scalestep, worldlow);
	}
    }

    // render it
    // Orig code: Dangling 'else' indicates possible error
    if (markceiling)
    {
	if (ceilingplane)   // killough 4/11/98: add NULL ptr checks
	    ceilingplane = R_CheckPlane(ceilingplane, rw_x, rw_stopx - 1);
	else
	    markceiling = false;
    }

    if (markfloor)
    {
	if (floorplane)     // killough 4/11/98: add NULL ptr checks
	    floorplane = R_CheckPlane(floorplane, rw_x, rw_stopx - 1);
	else
	    markfloor = false;
    }

    R_RenderSegLoop();

    // save sprite clipping info
    if (((ds_p->silhouette & SIL_TOP) || maskedtexture) && !ds_p->sprtopclip)
    {
	memcpy(lastopening, ceilingclip + start, sizeof(*lastopening) * (rw_stopx - start));
	ds_p->sprtopclip = lastopening - start;
	lastopening += rw_stopx - start;
    }

    if (((ds_p->silhouette & SIL_BOTTOM) || maskedtexture) && !ds_p->sprbottomclip)
    {
	memcpy(lastopening, floorclip + start, sizeof(*lastopening) * (rw_stopx - start));
	ds_p->sprbottomclip = lastopening - start;
	lastopening += rw_stopx - start;
    }

    if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
    {
	ds_p->silhouette |= SIL_TOP;
	ds_p->tsilheight = MININT;
    }
    if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
    {
	ds_p->silhouette |= SIL_BOTTOM;
	ds_p->bsilheight = MAXINT;
    }
    ++ds_p;
}
