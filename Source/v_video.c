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
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: v_video.c,v 1.5 1997/02/03 22:45:13 b1 Exp $";
#endif


#include "includes.h"

// Each screen is [SCREENWIDTH*SCREENHEIGHT];
byte*				screens[5];

int				dirtybox[4];



// Now where did these came from?
byte gammatable[5][256] =
{
    {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
     17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,
     33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
     49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,
     65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,
     81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,
     97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,
     113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
     128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
     144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
     160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
     176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
     192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
     208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
     224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
     240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255},

    {2,4,5,7,8,10,11,12,14,15,16,18,19,20,21,23,24,25,26,27,29,30,31,
     32,33,34,36,37,38,39,40,41,42,44,45,46,47,48,49,50,51,52,54,55,
     56,57,58,59,60,61,62,63,64,65,66,67,69,70,71,72,73,74,75,76,77,
     78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,
     99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,
     115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,129,
     130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,
     146,147,148,148,149,150,151,152,153,154,155,156,157,158,159,160,
     161,162,163,163,164,165,166,167,168,169,170,171,172,173,174,175,
     175,176,177,178,179,180,181,182,183,184,185,186,186,187,188,189,
     190,191,192,193,194,195,196,196,197,198,199,200,201,202,203,204,
     205,205,206,207,208,209,210,211,212,213,214,214,215,216,217,218,
     219,220,221,222,222,223,224,225,226,227,228,229,230,230,231,232,
     233,234,235,236,237,237,238,239,240,241,242,243,244,245,245,246,
     247,248,249,250,251,252,252,253,254,255},

    {4,7,9,11,13,15,17,19,21,22,24,26,27,29,30,32,33,35,36,38,39,40,42,
     43,45,46,47,48,50,51,52,54,55,56,57,59,60,61,62,63,65,66,67,68,69,
     70,72,73,74,75,76,77,78,79,80,82,83,84,85,86,87,88,89,90,91,92,93,
     94,95,96,97,98,100,101,102,103,104,105,106,107,108,109,110,111,112,
     113,114,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
     129,130,131,132,133,133,134,135,136,137,138,139,140,141,142,143,144,
     144,145,146,147,148,149,150,151,152,153,153,154,155,156,157,158,159,
     160,160,161,162,163,164,165,166,166,167,168,169,170,171,172,172,173,
     174,175,176,177,178,178,179,180,181,182,183,183,184,185,186,187,188,
     188,189,190,191,192,193,193,194,195,196,197,197,198,199,200,201,201,
     202,203,204,205,206,206,207,208,209,210,210,211,212,213,213,214,215,
     216,217,217,218,219,220,221,221,222,223,224,224,225,226,227,228,228,
     229,230,231,231,232,233,234,235,235,236,237,238,238,239,240,241,241,
     242,243,244,244,245,246,247,247,248,249,250,251,251,252,253,254,254,
     255},

    {8,12,16,19,22,24,27,29,31,34,36,38,40,41,43,45,47,49,50,52,53,55,
     57,58,60,61,63,64,65,67,68,70,71,72,74,75,76,77,79,80,81,82,84,85,
     86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,103,104,105,106,107,
     108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,
     125,126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,
     141,142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,
     155,156,157,158,159,160,160,161,162,163,164,165,165,166,167,168,169,
     169,170,171,172,173,173,174,175,176,176,177,178,179,180,180,181,182,
     183,183,184,185,186,186,187,188,189,189,190,191,192,192,193,194,195,
     195,196,197,197,198,199,200,200,201,202,202,203,204,205,205,206,207,
     207,208,209,210,210,211,212,212,213,214,214,215,216,216,217,218,219,
     219,220,221,221,222,223,223,224,225,225,226,227,227,228,229,229,230,
     231,231,232,233,233,234,235,235,236,237,237,238,238,239,240,240,241,
     242,242,243,244,244,245,246,246,247,247,248,249,249,250,251,251,252,
     253,253,254,254,255},

    {16,23,28,32,36,39,42,45,48,50,53,55,57,60,62,64,66,68,69,71,73,75,76,
     78,80,81,83,84,86,87,89,90,92,93,94,96,97,98,100,101,102,103,105,106,
     107,108,109,110,112,113,114,115,116,117,118,119,120,121,122,123,124,
     125,126,128,128,129,130,131,132,133,134,135,136,137,138,139,140,141,
     142,143,143,144,145,146,147,148,149,150,150,151,152,153,154,155,155,
     156,157,158,159,159,160,161,162,163,163,164,165,166,166,167,168,169,
     169,170,171,172,172,173,174,175,175,176,177,177,178,179,180,180,181,
     182,182,183,184,184,185,186,187,187,188,189,189,190,191,191,192,193,
     193,194,195,195,196,196,197,198,198,199,200,200,201,202,202,203,203,
     204,205,205,206,207,207,208,208,209,210,210,211,211,212,213,213,214,
     214,215,216,216,217,217,218,219,219,220,220,221,221,222,223,223,224,
     224,225,225,226,227,227,228,228,229,229,230,230,231,232,232,233,233,
     234,234,235,235,236,236,237,237,238,239,239,240,240,241,241,242,242,
     243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,
     251,252,252,253,254,254,255,255}
};



int	usegamma;

extern int fuzzoffset [50];
extern int fuzzpos;
extern byte *tinttab;

/* ---------------------------------------------------------------------------- */
//
// V_MarkRect
//
void
V_MarkRect
( int		x,
  int		y,
  int		width,
  int		height )
{
    M_AddToBox (dirtybox, x, y);
    M_AddToBox (dirtybox, x+width-1, y+height-1);
}


/* ---------------------------------------------------------------------------- */
//
// V_CopyRect
//
void
V_CopyRect
( int		srcx,
  int		srcy,
  int		srcscrn,
  int		width,
  int		height,
  int		destx,
  int		desty,
  int		destscrn )
{
    byte*	src;
    byte*	dest;

#ifdef RANGECHECK
    if ((srcx < 0)
     || (srcy < 0)
     || (destx < 0)
     || (desty < 0)
     || ((unsigned)srcscrn>4)
     || ((unsigned)destscrn>4))
    {
	I_Error ("Bad V_CopyRect: %d, %d, %d, %d, %d, %d\n",
	                           srcx, srcy, width, height, destx, desty);
    }

    if ((srcx+width) > SCREENWIDTH)
      width = SCREENWIDTH - srcx;
    if ((srcy+height) > SCREENHEIGHT)
      height = SCREENHEIGHT - srcy;
    if ((destx+width) > SCREENWIDTH)
      width = SCREENWIDTH - destx;
    if ((desty+height) > SCREENHEIGHT)
      height = SCREENHEIGHT - desty;
#endif

    V_MarkRect (destx, desty, width, height);

    src = screens[srcscrn]+SCREENWIDTH*srcy+srcx;
    dest = screens[destscrn]+SCREENWIDTH*desty+destx;

    for ( ; height>0 ; height--)
    {
	memcpy (dest, src, width);
	src += SCREENWIDTH;
	dest += SCREENWIDTH;
    }
}

/* ---------------------------------------------------------------------------- */
// drawstyle bit 0 = flip
//	     bit 1 = fuzzy
//	     bit 2 = translucent

void V_DrawPatchScaleFlip (int x, int y, int scrn,
		patch_t * patch, fixed_t xscale, fixed_t yscale, int drawstyle)
{
  int w;
  column_t* column;
  byte* desttop;
  byte* dest;
  byte* source;
  byte* scrnlimit;
  fixed_t xiscale;
  fixed_t yiscale;
  fixed_t col;
  fixed_t row;
  int xpos;
  int td;
  int topdelta;
  int lastlength;
  int translucent;

  if (xscale == FRACUNIT)
    xiscale = xscale;
  else
    xiscale = FixedDiv (FRACUNIT, xscale);

  if (yscale == FRACUNIT)
    yiscale = yscale;
  else
    yiscale = FixedDiv (FRACUNIT, yscale);

  if (!scrn)
  {
    V_MarkRect (x, y, (SHORT(patch->width)*xscale)>>FRACBITS, (SHORT(patch->height)*yscale)>>FRACBITS);
  }

  w = SHORT(patch->width);
  col = 0;

  if ((xpos = x) < 0)
  {
    col = xiscale * -xpos;
    if ((col >> FRACBITS) >= w)
      return;
    xpos = 0;
  }

  desttop = screens[scrn]+(y*SCREENWIDTH)+xpos;
  scrnlimit = (screens[scrn] + (SCREENHEIGHT*SCREENWIDTH)) -1;
  do
  {
    if (xpos >= SCREENWIDTH)
      break;

    if (drawstyle & 1)
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[w-1-(col>>FRACBITS)]));
    else
      column = (column_t *)((byte *)patch + LONG(patch->columnofs[col>>FRACBITS]));

    topdelta = -1;
    lastlength = 0;

    // step through the posts in a column
    while ((td = column->topdelta) != 0xff)
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
      if (lastlength)
      {
	//dest = desttop + (topdelta*SCREENWIDTH*(scale>>FRACBITS));
	dest = desttop + (SCREENWIDTH*((topdelta * yscale) >> FRACBITS));
#if 0
	if (y+((topdelta+lastlength)*xscale) > SCREENHEIGHT)
	{
	  if (SCREENHEIGHT <= (y+topdelta))
	    count = 0;
	  else
	    count = (SCREENHEIGHT - (y+topdelta)) / xscale;
	}
#endif

	row = 0;

#if 0
	if (drawstyle & 4)				// Shadow?
                  shadow = dest + SCREENWIDTH + 2;
                  if (!flag || (*shadow != 47 && *shadow != 191))
                  *shadow = tinttab50[*shadow];
#endif

	if (drawstyle & 2)				// Fuzzy?
	{
	  do
	  {
	    if (dest > scrnlimit)
	      break;

	    if (dest >= screens[scrn])
	    {
	      // Lookup framebuffer, and retrieve
	      //  a pixel that is either one column
	      //  left or right of the current one.
	      // Add index from colormap to index.
	      source = dest+(fuzzoffset[fuzzpos]*SCREENWIDTH);
	      if (source < screens[scrn])
		source = screens[scrn];
	      if (source > scrnlimit)
		source = scrnlimit;

	      *dest = colormaps[6*256+*source];

	      // Clamp table lookup index.
	      if (++fuzzpos >= ARRAY_SIZE (fuzzoffset))
		fuzzpos = 0;
	    }

	    dest += SCREENWIDTH;
	    row += yiscale;
          } while ((row >> FRACBITS) < lastlength);
	}
	else if (drawstyle & 4)				// Translucent?
	{
	  translucent = (x + y) & 1;
	  source = (byte *)column + 3;
	  do
	  {
	    if (dest > scrnlimit)
	      break;
	    if (dest >= screens[scrn])
	    {
	      if (tinttab)
		*dest = tinttab[(*dest << 8) + source [row >> FRACBITS]];
	      else if (translucent == 0)
		*dest = source [row >> FRACBITS];
	    }
	    translucent ^= 1;
	    dest += SCREENWIDTH;
	    row += yiscale;
          } while ((row >> FRACBITS) < lastlength);
	}
	else
	{
	  source = (byte *)column + 3;
	  do
	  {
	    if (dest > scrnlimit)
	      break;
	    if (dest >= screens[scrn])
	      *dest = source [row >> FRACBITS];
	    dest += SCREENWIDTH;
	    row += yiscale;
          } while ((row >> FRACBITS) < lastlength);
	}
      }      
      column = (column_t *)((byte *)column + lastlength + 4);
    }
    xpos++;
    desttop++;
    col += xiscale;
  } while ((col >> FRACBITS) < w);
}

/* ---------------------------------------------------------------------------- */
//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//
#ifndef V_DrawPatch
void
V_DrawPatch
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch )
{
  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
  V_DrawPatchScaleFlip (x, y, scrn, patch, FRACUNIT, FRACUNIT, 0);
}
#endif

/* ---------------------------------------------------------------------------- */
//
// V_DrawPatchFlippedScaled
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void V_DrawPatchFlippedScaled (int x, int y, int scrn, patch_t* patch, int drawstyle)
{
  fixed_t xscale;
  fixed_t yscale;

  if ((SHORT(patch->width) > ORIGSCREENWIDTH)
   || (SHORT(patch->height) > ORIGSCREENHEIGHT))
  {
    xscale = FixedDiv (SCREENWIDTH << FRACBITS, SHORT(patch->width) << FRACBITS);
    yscale = FixedDiv (SCREENHEIGHT << FRACBITS, SHORT(patch->height) << FRACBITS);
  }
  else
  {
    xscale = (SCREENWIDTH << FRACBITS) / ORIGSCREENWIDTH;
    yscale = (SCREENHEIGHT << FRACBITS) / ORIGSCREENHEIGHT;
  }

  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);

  y = (y * yscale) >> FRACBITS;
  x = (x * xscale) >> FRACBITS;

  //y += (SCREENHEIGHT - (yscale * 200))/2;
  //x += (SCREENWIDTH  - (xscale * 320))/2;
  // printf ("xscale = %X, yscale = %X\n", xscale, yscale);

  V_DrawPatchScaleFlip (x, y, scrn, patch, xscale, yscale, drawstyle);
}

/* ---------------------------------------------------------------------------- */
//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
#ifndef V_DrawPatchFlipped
void
V_DrawPatchFlipped
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch )
{
  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
  V_DrawPatchScaleFlip (x, y, scrn, patch, FRACUNIT, FRACUNIT, 1);
}
#endif


/* ---------------------------------------------------------------------------- */
//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//
#ifndef V_DrawPatchDirect
void
V_DrawPatchDirect
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch )
{
  y -= SHORT(patch->topoffset);
  x -= SHORT(patch->leftoffset);
  V_DrawPatchScaleFlip (x, y, scrn, patch, FRACUNIT, FRACUNIT, 0);
}
#endif


/* ---------------------------------------------------------------------------- */
//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void
V_DrawBlock
( int		x,
  int		y,
  int		scrn,
  int		width,
  int		height,
  byte*		src )
{
    byte*	dest;

#ifdef RANGECHECK
    if (x<0
	||x+width >SCREENWIDTH
	|| y<0
	|| y+height>SCREENHEIGHT
	|| (unsigned)scrn>4 )
    {
	I_Error ("Bad V_DrawBlock");
    }
#endif

    V_MarkRect (x, y, width, height);

    dest = screens[scrn] + y*SCREENWIDTH+x;

    while (height--)
    {
	memcpy (dest, src, width);
	src += width;
	dest += SCREENWIDTH;
    }
}

/* ---------------------------------------------------------------------------- */
//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
void
V_GetBlock
( int		x,
  int		y,
  int		scrn,
  int		width,
  int		height,
  byte*		dest )
{
    byte*	src;

#ifdef RANGECHECK
    if (x<0
	||x+width >SCREENWIDTH
	|| y<0
	|| y+height>SCREENHEIGHT
	|| (unsigned)scrn>4 )
    {
	I_Error ("Bad V_DrawBlock");
    }
#endif

    src = screens[scrn] + y*SCREENWIDTH+x;

    while (height--)
    {
	memcpy (dest, src, width);
	src += SCREENWIDTH;
	dest += width;
    }
}

/* ---------------------------------------------------------------------------- */

void V_DrawPixel (int x, int y, int screen, int colour, fixed_t xscale, fixed_t yscale, boolean shadow)
{
  int x1,y1,yoff;
  fixed_t xiscale;
  fixed_t yiscale;
  byte *dest;


  if (xscale == FRACUNIT)
    xiscale = xscale;
  else
    xiscale = FixedDiv (FRACUNIT, xscale);

  if (yscale == FRACUNIT)
    yiscale = yscale;
  else
    yiscale = FixedDiv (FRACUNIT, yscale);

  dest = screens[screen] + x + (y * SCREENWIDTH);

  x1 = x << FRACBITS;
  do
  {
    y1 = y << FRACBITS;
    yoff = 0;
    do
    {
      if ((colour == 251) || (colour == -5))
      {
#if 0
        if (shadow)
        {
	  dest [yoff] = tinttable50[dest[yoff]];
        }
#endif
      }
      else if ((colour) && (colour != 32))
      {
        dest [yoff] = colour;
      }

      yoff += SCREENWIDTH;
      y1 += yiscale;
    } while ((y1 >> FRACBITS) == y);
    dest++;
    x1 += xiscale;
  } while ((x1 >> FRACBITS) == x);
}

/* ---------------------------------------------------------------------------- */

void V_DrawPixelScaled (int x, int y, int screen, int colour, boolean shadow)
{
  fixed_t xscale;
  fixed_t yscale;

  if (((unsigned)x < ORIGSCREENWIDTH) && ((unsigned)y < ORIGSCREENHEIGHT))
  {
    xscale = (SCREENWIDTH << FRACBITS) / ORIGSCREENWIDTH;
    yscale = (SCREENHEIGHT << FRACBITS) / ORIGSCREENHEIGHT;

    y = (y * yscale) >> FRACBITS;
    x = (x * xscale) >> FRACBITS;

    V_DrawPixel (x, y, screen, colour, xscale, yscale, shadow);
  }
}

/* ---------------------------------------------------------------------------- */
//
// V_Init
//
void V_Init (void)
{
    int		i;
    byte*	base;

    // stick these in low dos memory on PCs

    base = I_AllocLow (SCREENWIDTH*SCREENHEIGHT*4);

    for (i=0 ; i<4 ; i++)
	screens[i] = base + i*SCREENWIDTH*SCREENHEIGHT;
}

/* ---------------------------------------------------------------------------- */
