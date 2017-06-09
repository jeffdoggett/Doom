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
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: r_draw.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif

#include "includes.h"

/* ------------------------------------------------------------------------------------------------ */

// status bar height at bottom of screen
#define SBARHEIGHT		32

#define R_ADDRESS(scrn, px, py) \
    (screens[scrn] + (viewwindowy + (py)) * SCREENWIDTH + (viewwindowx + (px)))
//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//


byte*		viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte*		dc_translation;
byte*		translationtables;

byte*		tinttab = NULL;
byte*		transtab = NULL;

int		translucency;

extern const char borderpatch_1 [];
extern const char borderpatch_2 [];

extern flatinfo_t * flatinfo;

#define USE_TINT_TABLES
/* ------------------------------------------------------------------------------------------------ */
#ifdef USE_TINT_TABLES

#define ADDITIVE	-1

#define R		1
#define W		2
#define G		4
#define B		8
#define X		16

static const byte general [256] =
{
    0,   X,   0,   0,   R|B, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 000 to 015
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 016 to 031
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 032 to 047
    W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W,   W, // 048 to 063
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   X,   X,   X, // 064 to 079
    R,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 080 to 095
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 096 to 111
    G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G,   G, // 112 to 127
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 128 to 143
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, // 144 to 159
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 160 to 175
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 176 to 191
    B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B,   B, // 192 to 207
    R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R,   R, // 208 to 223
    R|B, R|B, R,   R,   R,   R,   R,   R,   X,   X,   X,   X,   0,   0,   0,   0, // 224 to 239
    B,   B,   B,   B,   B,   B,   B,   B,   R,   R,   0,   0,   0,   0,   0,   0  // 240 to 255
};

#define ALL		0
#define REDS		R
#define WHITES		W
#define GREENS		G
#define BLUES		B
#define EXTRAS		X

/* ------------------------------------------------------------------------------------------------ */

static byte *GenerateTintTable (const byte *palette, int percent, const byte * filter, int colours)
{
    byte *result = Z_Malloc (65536, PU_STATIC, NULL);
    int	 foreground, background;

    for (foreground = 0; foreground < 256; ++foreground)
    {
	if ((filter[foreground] & colours) || colours == ALL)
	{
	    for (background = 0; background < 256; ++background)
	    {
		const byte *colour1 = palette + background * 3;
		const byte *colour2 = palette + foreground * 3;
		int     r, g, b;

		if (percent == ADDITIVE)
		{
		    if ((filter[background] & BLUES) && !(filter[foreground] & WHITES))
		    {
			r = ((int)colour1[0] * 25 + (int)colour2[0] * 75) / 100;
			g = ((int)colour1[1] * 25 + (int)colour2[1] * 75) / 100;
			b = ((int)colour1[2] * 25 + (int)colour2[2] * 75) / 100;
		    }
		    else
		    {
			r = colour1[0] + colour2[0]; if (r > 255) r = 255;
			g = colour1[1] + colour2[1]; if (g > 255) g = 255;
			b = colour1[2] + colour2[2]; if (b > 255) b = 255;
		    }
		}
		else
		{
		    r = ((int)colour1[0] * percent + (int)colour2[0] * (100 - percent)) / 100;
		    g = ((int)colour1[1] * percent + (int)colour2[1] * (100 - percent)) / 100;
		    b = ((int)colour1[2] * percent + (int)colour2[2] * (100 - percent)) / 100;
		}
		*(result + (background << 8) + foreground) = AM_load_colour (r, g, b, palette);
	    }
	}
	else
	    for (background = 0; background < 256; ++background)
		*(result + (background << 8) + foreground) = foreground;
    }

    return result;
}

/* ------------------------------------------------------------------------------------------------ */

void R_InitTranslucencyTables (void)
{
  byte * palette;

  if (transtab == NULL)
  {
    // printf ("\nBuilding Translucency tables\n");
    palette = W_CacheLumpName ("PLAYPAL", PU_STATIC);

    if (translucency < 0)
      translucency = 0;

    if (translucency > 100)
      translucency = 100;

    transtab = GenerateTintTable (palette, translucency, general, ALL);
  }
}

/* ------------------------------------------------------------------------------------------------ */

static void R_InitTintTables (void)
{
  byte * palette;

  // printf ("\nBuilding Tint tables\n");
  palette = W_CacheLumpName ("PLAYPAL", PU_STATIC);
  tinttab = GenerateTintTable (palette, ADDITIVE, general, ALL);
}

/* ------------------------------------------------------------------------------------------------ */
/*
   Only bother to build the tint tables if the MF_TRANSLUCENT bit is set.
*/

static int R_TintTableRequired (void)
{
  unsigned int c;
  mobjinfo_t * m;
  state_t * s;

  m = mobjinfo;
  c = NUMMOBJTYPES;
  do
  {
    if (m->flags & MF_TRANSLUCENT)
      return (1);
    m++;
  } while (--c);

  s = states;
  c = NUMSTATES;
  do
  {
    if (s -> frame & FF_TRANSLUCENT)
      return (1);
    s++;
  } while (--c);

  return (0);
}

#endif
/* ------------------------------------------------------------------------------------------------ */
//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to grey, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
    int		i;

    translationtables = Z_Malloc (256*3, PU_STATIC, NULL);

    // translate just the 16 green colours
    for (i=0 ; i<256 ; i++)
    {
	if (i >= 0x70 && i<= 0x7f)
	{
	    // map green ramp to grey, brown, red
	    translationtables[i] = 0x60 + (i&0xf);
	    translationtables [i+256] = 0x40 + (i&0xf);
	    translationtables [i+512] = 0x20 + (i&0xf);
	}
	else
	{
	    // Keep all other colours as is.
	    translationtables[i] = translationtables[i+256] = translationtables[i+512] = i;
	}
    }
#ifdef USE_TINT_TABLES
    if (R_TintTableRequired ())
      R_InitTintTables ();
#endif
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t*		dc_colormap;
int			dc_x;
int			dc_yl;
int			dc_yh;
int			dc_ylim;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;
fixed_t			dc_texturefrac;
// first pixel in a column (possibly virtual)
byte*			dc_source;

// just for profiling
// static int		dccount;

/* ------------------------------------------------------------------------------------------------ */

fixed_t R_CalcFrac (void)
{
    fixed_t frac;

    frac = dc_texturemid + (dc_yl-centery)*dc_iscale;
    if ((unsigned) frac >= (unsigned) dc_ylim)
    {
      fixed_t mask;

      mask = dc_ylim - 1;
      if ((dc_ylim & mask) == 0)	/* Power of 2 height? */
      {
	frac &= mask;			/* Yes. Can just mask off. */
      }
      else if (frac < 0)
      {
	frac = -frac;
	frac = (unsigned) frac % (unsigned) dc_ylim;
	frac = dc_ylim - frac;
      }
      else
      {
	frac = (unsigned) frac % (unsigned) dc_ylim;
      }
    }

    return (frac);
}

/* ------------------------------------------------------------------------------------------------ */
//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
void R_DrawColumn (void)
{
    int			count;
    byte*		dest;
    byte*		scrnlimit;
    fixed_t		frac;
    fixed_t		fracstep;

    count = (dc_yh - dc_yl) + 1;

    // Zero length, column does not exceed a pixel.
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH)
    {
	printf ("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
	return;
    }
#endif

    dest = R_ADDRESS(0, dc_x, dc_yl);
    scrnlimit = (screens[0] + (SCREENHEIGHT*SCREENWIDTH)) -1;

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturefrac;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    do
    {
	if (dest > scrnlimit)
	  break;

	if (dest >= screens[0])
	{
	  // Re-map color indices from wall texture column
	  //  using a lighting/special effects LUT.
	  *dest = dc_colormap[dc_source[frac>>FRACBITS]];
	}

	dest += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */
// UNUSED.
// Loop unrolled.
#if 0
void R_DrawColumn (void)
{
    int			count;
    byte*		source;
    byte*		dest;
    byte*		colormap;

    unsigned		frac;
    unsigned		fracstep;
    unsigned		fracstep2;
    unsigned		fracstep3;
    unsigned		fracstep4;

    count = dc_yh - dc_yl + 1;

    source = dc_source;
    colormap = dc_colormap;
    dest = R_ADDRESS(0, dc_x, dc_yl);

    fracstep = dc_iscale<<9;
    frac = (dc_texturemid + (dc_yl-centery)*dc_iscale)<<9;

    fracstep2 = fracstep+fracstep;
    fracstep3 = fracstep2+fracstep;
    fracstep4 = fracstep3+fracstep;

    while (count >= 8)
    {
	dest[0] = colormap[source[frac>>25]];
	dest[SCREENWIDTH] = colormap[source[(frac+fracstep)>>25]];
	dest[SCREENWIDTH*2] = colormap[source[(frac+fracstep2)>>25]];
	dest[SCREENWIDTH*3] = colormap[source[(frac+fracstep3)>>25]];

	frac += fracstep4;

	dest[SCREENWIDTH*4] = colormap[source[frac>>25]];
	dest[SCREENWIDTH*5] = colormap[source[(frac+fracstep)>>25]];
	dest[SCREENWIDTH*6] = colormap[source[(frac+fracstep2)>>25]];
	dest[SCREENWIDTH*7] = colormap[source[(frac+fracstep3)>>25]];

	frac += fracstep4;
	dest += SCREENWIDTH*8;
	count -= 8;
    }

    while (count > 0)
    {
	*dest = colormap[source[frac>>25]];
	dest += SCREENWIDTH;
	frac += fracstep;
	count--;
    }
}
#endif

/* ------------------------------------------------------------------------------------------------ */

void R_DrawColumnLow (void)
{
    int			count;
    byte*		dest;
    byte*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;

    count = (dc_yh - dc_yl) + 1;

    // Zero length.
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {

	I_Error ("R_DrawColumnLow: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
    //	dccount++;
#endif
    // Blocky mode, need to multiply by 2.
    dc_x <<= 1;

    dest = R_ADDRESS(0, dc_x, dc_yl);
    dest2 = dest + 1;

    fracstep = dc_iscale;
    frac = dc_texturefrac;

    do
    {
	// Hack. Does not work corretly.
	*dest2 = *dest = dc_colormap[dc_source[frac>>FRACBITS]];
	dest += SCREENWIDTH;
	dest2 += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}


/* ------------------------------------------------------------------------------------------------ */
//
// Spectre/Invisibility.
//
#define FUZZOFF	(1)

int fuzzoffset [] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
    FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
    FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
    FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

int fuzzpos = 0;


/* ------------------------------------------------------------------------------------------------ */
//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn (void)
{
    int			fp;
    int			count;
    byte*		dest;
    byte*		source;
    byte*		lim_lo;
    byte*		lim_hi;
    fixed_t		frac;
    fixed_t		fracstep;

    // Adjust borders. Low...
    if (!dc_yl)
	dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1)
	dc_yh = viewheight - 2;

    count = (dc_yh - dc_yl) + 1;

    // Zero length.
    if (count < 1)
	return;


#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);
    }
#endif


    // Keep till detailshift bug in blocky mode fixed,
    //  or blocky mode removed.
    /* WATCOM code
    if (detailshift)
    {
	if (dc_x & 1)
	{
	    outpw (GC_INDEX,GC_READMAP+(2<<8) );
	    outp (SC_INDEX+1,12);
	}
	else
	{
	    outpw (GC_INDEX,GC_READMAP);
	    outp (SC_INDEX+1,3);
	}
	dest = destview + dc_yl*80 + (dc_x>>1);
    }
    else
    {
	outpw (GC_INDEX,GC_READMAP+((dc_x&3)<<8) );
	outp (SC_INDEX+1,1<<(dc_x&3));
	dest = destview + dc_yl*80 + (dc_x>>2);
    }*/


    lim_lo = screens[0];
    lim_hi = lim_lo + (SCREENWIDTH*SCREENHEIGHT);

    // Does not work with blocky mode.
    dest = R_ADDRESS(0, dc_x, dc_yl);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;
    fp = fuzzpos;
    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
	// Lookup framebuffer, and retrieve
	//  a pixel that is either one column
	//  left or right of the current one.
	// Add index from colormap to index.
	source = dest+(fuzzoffset[fp]*SCREENWIDTH);
	if (source < lim_lo)
	  source = lim_lo;
	if (source >= lim_hi)
	  source = lim_hi - 1;

	*dest = colormaps[6*256+*source];

	// Clamp table lookup index.
	if (++fp >= ARRAY_SIZE (fuzzoffset))
	    fp = 0;

	dest += SCREENWIDTH;
	frac += fracstep;
    } while (--count);
    fuzzpos = fp;
}

/* ------------------------------------------------------------------------------------------------ */

void R_DrawTranslucentColumn (void)
{
    int			count;
    byte*		dest;
    byte*		scrnlimit;
    fixed_t		frac;
    fixed_t		fracstep;
#ifndef USE_TINT_TABLES
    int			translucent;
#else
    if (tinttab == NULL)		// Just in case!
      R_InitTintTables ();
#endif

    count = (dc_yh - dc_yl) + 1;

    // Zero length, column does not exceed a pixel.
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH)
    {
	printf ("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
	return;
    }
#endif

    dest = R_ADDRESS(0, dc_x, dc_yl);
    scrnlimit = (screens[0] + (SCREENHEIGHT*SCREENWIDTH)) -1;

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturefrac;

#ifndef USE_TINT_TABLES
    translucent = (dc_x + dc_yl) & 1;
#endif

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    do
    {
	if (dest > scrnlimit)
	  break;

	if (dest >= screens[0])
	{
#ifdef USE_TINT_TABLES
	  *dest = tinttab[(*dest << 8) + dc_colormap[dc_source[frac >> FRACBITS]]];
#else
	  if (translucent == 0)
	    *dest = dc_colormap[dc_source[frac>>FRACBITS]];
#endif
	}

#ifndef USE_TINT_TABLES
	translucent ^= 1;
#endif
	dest += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */

void R_DrawTranslucent50Column (void)
{
    int			count;
    byte*		dest;
    byte*		scrnlimit;
    fixed_t		frac;
    fixed_t		fracstep;
#ifndef USE_TINT_TABLES
    int			translucent;
#else
    int			lump;
    byte*		tranmap;


    lump = curline->linedef->tranlump;
    if ((unsigned)lump >= numlumps)
    {
      if ((tranmap = transtab) == NULL)		// Just in case!
      {
	R_InitTranslucencyTables ();
	tranmap = transtab;
      }
    }
    else
    {
      tranmap = W_CacheLumpNum (lump, PU_LEVEL);
    }
#endif

    count = (dc_yh - dc_yl) + 1;

    // Zero length, column does not exceed a pixel.
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH)
    {
	printf ("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
	return;
    }
#endif

    dest = R_ADDRESS(0, dc_x, dc_yl);
    scrnlimit = (screens[0] + (SCREENHEIGHT*SCREENWIDTH)) -1;

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturefrac;

#ifndef USE_TINT_TABLES
    translucent = (dc_x + dc_yl) & 1;
#endif

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    do
    {
	if (dest > scrnlimit)
	  break;

	if (dest >= screens[0])
	{
#ifdef USE_TINT_TABLES
	  *dest = tranmap[(*dest << 8) + dc_colormap[dc_source[frac >> FRACBITS]]];
#else
	  if (translucent == 0)
	    *dest = dc_colormap[dc_source[frac>>FRACBITS]];
#endif
	}

#ifndef USE_TINT_TABLES
	translucent ^= 1;
#endif
	dest += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//

void R_DrawTranslatedColumn (void)
{
    int			count;
    byte*		dest;
    byte*		scrnlimit;
    fixed_t		frac;
    fixed_t		fracstep;

    count = (dc_yh - dc_yl) + 1;
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH)
    {
	printf ( "R_DrawTranslatedColumn: %i to %i at %i\n",
		  dc_yl, dc_yh, dc_x);
	return;
    }

#endif


    // WATCOM VGA specific.
    /* Keep for fixing.
    if (detailshift)
    {
	if (dc_x & 1)
	    outp (SC_INDEX+1,12);
	else
	    outp (SC_INDEX+1,3);

	dest = destview + dc_yl*80 + (dc_x>>1);
    }
    else
    {
	outp (SC_INDEX+1,1<<(dc_x&3));

	dest = destview + dc_yl*80 + (dc_x>>2);
    }*/


    // FIXME. As above.
    dest = R_ADDRESS(0, dc_x, dc_yl);
    scrnlimit = (screens[0] + (SCREENHEIGHT*SCREENWIDTH)) -1;

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturefrac;

    // Here we do an additional index re-mapping.
    do
    {
	if (dest > scrnlimit)
	  break;

	if (dest >= screens[0])
	{
	  // Translation tables are used
	  //  to map certain colorramps to other ones,
	  //  used with PLAY sprites.
	  // Thus the "green" ramp of the player 0 sprite
	  //  is mapped to grey, red, black/indigo.
	  *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
	}
	dest += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */

void R_DrawTranslatedTranslucentColumn (void)
{
    int			count;
    byte*		dest;
    byte*		scrnlimit;
    fixed_t		frac;
    fixed_t		fracstep;
#ifndef USE_TINT_TABLES
    int			translucent;
#endif

    count = (dc_yh - dc_yl) + 1;

    // Zero length, column does not exceed a pixel.
    if (count < 1)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH)
    {
	printf ("R_DrawColumn: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
	return;
    }
#endif

    dest = R_ADDRESS(0, dc_x, dc_yl);
    scrnlimit = (screens[0] + (SCREENHEIGHT*SCREENWIDTH)) -1;

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturefrac;

#ifndef USE_TINT_TABLES
    translucent = (dc_x + dc_yl) & 1;
#endif

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    do
    {
	if (dest > scrnlimit)
	  break;

	if (dest >= screens[0])
	{
#ifdef USE_TINT_TABLES
	  *dest = tinttab[(*dest << 8) + dc_colormap[dc_translation[dc_source[frac >> FRACBITS]]]];
#else
	  if (translucent == 0)
	    *dest = dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]];
#endif
	}

#ifndef USE_TINT_TABLES
	translucent ^= 1;
#endif
	dest += SCREENWIDTH;
	frac += fracstep;

	if ((unsigned) frac >= (unsigned) dc_ylim)
	  frac -= dc_ylim;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int			ds_y;
int			ds_x1;
int			ds_x2;

lighttable_t*		ds_colormap;

fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;

// start of a 64*64 tile image
byte*			ds_source;

// just for profiling
int			dscount;


//
// Draws the actual span.
void R_DrawSpan (void)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    byte*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++;
#endif


    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    dest = R_ADDRESS(0, ds_x1, ds_y);

    // We do not check for zero spans here?
    count = (ds_x2 - ds_x1) + 1;

    do
    {
	// Current texture index in u,v.
	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);

	// Lookup pixel from flat texture tile,
	//  re-index using light/colormap.
	*dest++ = ds_colormap[ds_source[spot]];

	// Next step in u,v.
	xfrac += ds_xstep;
	yfrac += ds_ystep;

    } while (--count);
}


/* ------------------------------------------------------------------------------------------------ */
// UNUSED.
// Loop unrolled by 4.
#if 0
void R_DrawSpan (void)
{
    unsigned	position, step;

    byte*	source;
    byte*	colormap;
    byte*	dest;

    unsigned	count;
    usingned	spot;
    unsigned	value;
    unsigned	temp;
    unsigned	xtemp;
    unsigned	ytemp;

    position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
    step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);

    source = ds_source;
    colormap = ds_colormap;
    dest = R_ADDRESS(0, ds_x1, ds_y);
    count = ds_x2 - ds_x1 + 1;

    while (count >= 4)
    {
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[0] = colormap[source[spot]];

	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[1] = colormap[source[spot]];

	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[2] = colormap[source[spot]];

	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	dest[3] = colormap[source[spot]];

	count -= 4;
	dest += 4;
    }
    while (count > 0)
    {
	ytemp = position>>4;
	ytemp = ytemp & 4032;
	xtemp = position>>26;
	spot = xtemp | ytemp;
	position += step;
	*dest++ = colormap[source[spot]];
	count--;
    }
}
#endif


/* ------------------------------------------------------------------------------------------------ */
//
// Again..
//
void R_DrawSpanLow (void)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    byte*		dest;
    int			count;
    int			spot;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpanLow: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++;
#endif

    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    // Blocky mode, need to multiply by 2.
    ds_x1 <<= 1;
    ds_x2 <<= 1;

    dest = R_ADDRESS(0, ds_x1, ds_y);


    count = (ds_x2 - ds_x1) + 1;
    do
    {
	spot = ((yfrac>>(16-6))&(63*64)) + ((xfrac>>16)&63);
	// Lowres/blocky mode does it twice,
	//  while scale is adjusted appropriately.
	*dest++ = ds_colormap[ds_source[spot]];
	*dest++ = ds_colormap[ds_source[spot]];

	xfrac += ds_xstep;
	yfrac += ds_ystep;

    } while (--count);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitBuffer
// Creates lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int		width,
  int		height )
{
    int		sbarheight;

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    viewwindowx = (SCREENWIDTH-width) >> 1;

    // Samw with base row offset.
    if (width == SCREENWIDTH)
    {
	viewwindowy = 0;
    }
    else
    {
      sbarheight = (SBARHEIGHT * sbarscale) >> FRACBITS;
      viewwindowy = ((SCREENHEIGHT-sbarheight-height)+1) >> 1;
    }
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void)
{
    byte*	src;
    byte*	dest;
    int		x;
    int		y;
    int		lump;
    int		sbarheight;
    patch_t*	patch;
    const char*	name;
    map_dests_t * map_info_ptr;

    sbarheight = (SBARHEIGHT * sbarscale) >> FRACBITS;

    // printf ("Fill Back Screen - scaled view = %d,%d\n", scaledviewwidth,viewheight);

    if (viewheight <= (SCREENHEIGHT-sbarheight))
      ST_ClearSbarSides ();

    if (scaledviewwidth == SCREENWIDTH)
      return;

    map_info_ptr = G_Access_MapInfoTab (gameepisode, gamemap);
    name = map_info_ptr -> bordertexture;
    lump = R_CheckFlatNumForName (name);
    if (lump == -1)
    {
      if (gamemode == commercial)
	name = borderpatch_1;
      else
	name = borderpatch_2;

      lump = R_FlatNumForName (name);
    }

    src = W_CacheLumpNum (flatinfo[lump].lump, PU_CACHE);

    dest = screens[1];

    for (y=0 ; y<SCREENHEIGHT-sbarheight ; y++)
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

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
	V_DrawPatch (viewwindowx+x,viewwindowy-8,1,patch);
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<scaledviewwidth ; x+=8)
	V_DrawPatch (viewwindowx+x,viewwindowy+viewheight,1,patch);
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
	V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
	V_DrawPatch (viewwindowx+scaledviewwidth,viewwindowy+y,1,patch);


    // Draw beveled edge.
    V_DrawPatch (viewwindowx-8,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
		 viewwindowy-8,
		 1,
		 W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
		 viewwindowy+viewheight,
		 1,
		 W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+scaledviewwidth,
		 viewwindowy+viewheight,
		 1,
		 W_CacheLumpName ("brdr_br",PU_CACHE));
}


/* ------------------------------------------------------------------------------------------------ */
//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned	ofs,
  int		count )
{
  // LFB copy.
  // This might not be a good idea if memcpy
  //  is not optiomal, e.g. byte by byte on
  //  a 32bit CPU, as GNU GCC/Linux libc did
  //  at one point.
    memcpy (screens[0]+ofs, screens[1]+ofs, count);
}


/* ------------------------------------------------------------------------------------------------ */
//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void
V_MarkRect
( int		x,
  int		y,
  int		width,
  int		height );

void R_DrawViewBorder (void)
{
    int		top;
    int		side;
    int		ofs;
    int		i;
    int		sbarheight;

    if (scaledviewwidth == SCREENWIDTH)
	return;

    sbarheight = (SBARHEIGHT * sbarscale) >> FRACBITS;
    top = ((SCREENHEIGHT-sbarheight)-viewheight)/2;
    side = (SCREENWIDTH-scaledviewwidth)/2;

    // copy top and one line of left side
    R_VideoErase (0, top*SCREENWIDTH+side);

    // copy one line of right side and bottom
    ofs = (viewheight+top)*SCREENWIDTH-side;
    R_VideoErase (ofs, top*SCREENWIDTH+side);

    // copy sides using wraparound
    ofs = top*SCREENWIDTH + SCREENWIDTH-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
    {
	R_VideoErase (ofs, side);
	ofs += SCREENWIDTH;
    }

    // ?
    V_MarkRect (0,0,SCREENWIDTH, SCREENHEIGHT-sbarheight);
}

/* ------------------------------------------------------------------------------------------------ */
