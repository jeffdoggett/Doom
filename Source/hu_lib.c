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
// DESCRIPTION:  heads-up text and input code
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: hu_lib.c,v 1.3 1997/01/26 07:44:58 b1 Exp $";
#endif

#include "includes.h"

// boolean : whether the screen is always erased
#define noterased viewwindowx

extern boolean	automapactive;	// in AM_map.c

//-----------------------------------------------------------------------------

typedef struct
{
  char char1;
  char char2;
  unsigned char adjust;
} kern_t;

static const kern_t hu_kern [] =
{
  { '.', '1', 1 },
  { '.', '7', 1 },
  { ',', '1', 1 },
  { ',', '7', 1 },
  { ',', 'Y', 1 },
  { 'F', 'J', 1 },
  { 'F', '_', 1 },
  { 'L', 'T', 2 },
  { 'L', 'V', 1 },
  { 'L', 'Y', 1 },
  { 'L', '\\', 1 },
  { 'P', 'J', 1 },
  { 'P', '_', 2 },
  { 'P', '/', 1 },
  { 'T', 'J', 2 },
  { 'T', 'A', 1 },
  { 'T', '_', 1 },
  { 'T', '.', 1 },
  { 'T', ',', 1 },
  { 'V', 'J', 1 },
  { 'V', '_', 1 },
  { 'Y', '.', 1 },
  { 'Y', ',', 1 },
  { '/', 'J', 1 },
  { '/', '_', 1 },
  { 0, 0, 0 }
};


//-----------------------------------------------------------------------------

unsigned int HUlib_Kern (char this_one, char next_one)
{
  unsigned int rc;
  const kern_t * kern;

  kern = hu_kern;
  rc = kern -> adjust;

  do
  {
    if ((this_one == kern -> char1) && (next_one == kern -> char2))
      break;
    kern++;
  } while ((rc = kern -> adjust) != 0);

  return (rc);
}

//-----------------------------------------------------------------------------
#if 0
void HUlib_init(void)
{
}
#endif
//-----------------------------------------------------------------------------

void HUlib_clearTextLine(hu_textline_t* t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

//-----------------------------------------------------------------------------

void
HUlib_initTextLine
( hu_textline_t*	t,
  int			x,
  int			y,
  patch_t**		f,
  int			sc )
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

//-----------------------------------------------------------------------------

boolean
HUlib_addCharToTextLine
( hu_textline_t*	t,
  char			ch )
{
    if (t->len == HU_MAXLINELENGTH)
	return false;
    else
    {
	t->l[t->len++] = ch;
	t->l[t->len] = 0;
	t->needsupdate = 4;
	return true;
    }
}

//-----------------------------------------------------------------------------

boolean HUlib_delCharFromTextLine(hu_textline_t* t)
{
    if (!t->len) return false;
    else
    {
	t->l[--t->len] = 0;
	t->needsupdate = 4;
	return true;
    }
}

//-----------------------------------------------------------------------------
/*
  If the status bar has been scaled up then also scale up the text.
*/

#ifdef ALWAYS_SMALL_TEXT
#define HU_DrawPatchDirect(x,y,p) V_DrawPatchDirect(x,y,HU_FG,p)
#else

static void HU_DrawPatchDirect (int x, int y, patch_t * patch)
{
  int offset;

  offset = SHORT(patch->leftoffset);
  offset = (offset * sbarscale) >> FRACBITS;
  x -= offset;

  if (y)
  {
    if (y < 200)
    {
      y = (y * sbarscale) >> FRACBITS;
    }
    else
    {
      offset = ((SHORT(patch->height)) * (sbarscale - FRACUNIT)) >> FRACBITS;
      y -= offset;
    }
  }

  offset = SHORT(patch->topoffset);
  offset = (offset * sbarscale) >> FRACBITS;
  y -= offset;

  V_DrawPatchScaleFlip (x, y, HU_FG, patch, sbarscale, sbarscale, 0);
}
#endif
//-----------------------------------------------------------------------------

void
HUlib_drawTextLine
( hu_textline_t*	l,
  boolean		drawcursor )
{
    int			i;
    int			w;
    int			x;
    unsigned char	c;

    // draw the new stuff
    x = l->x;
    for (i=0;i<l->len;i++)
    {
	c = toupper(l->l[i]);
	if (c != ' '
	    && c >= l->sc
	    && c <= '_')
	{
	    w = (SHORT(l->f[c - l->sc]->width)) - HUlib_Kern (c, toupper(l->l[i+1]));
#ifndef ALWAYS_SMALL_TEXT
	    w = (w * sbarscale) >> FRACBITS;
#endif
	    if (x+w > SCREENWIDTH)
		break;
	    HU_DrawPatchDirect (x, l->y, l->f[c - l->sc]);
	    x += w;
	}
	else
	{
	    x += 4;
	    if (x >= SCREENWIDTH)
		break;
	}
    }

    // draw the cursor if requested
    if (drawcursor
	&& x + SHORT(l->f['_' - l->sc]->width) <= SCREENWIDTH)
    {
	HU_DrawPatchDirect (x, l->y, l->f['_' - l->sc]);
    }
}

//-----------------------------------------------------------------------------

// sorta called by HU_Erase and just better darn get things straight
void HUlib_eraseTextLine(hu_textline_t* l)
{
    int			lh;
    int			y;
    int			yoffset;
    static boolean	lastautomapactive = true;

    // Only erases when NOT in automap and the screen is reduced,
    // and the text must either need updating or refreshing
    // (because of a recent change back from the automap)

    if (!automapactive &&
	viewwindowx && l->needsupdate)
    {
	lh = SHORT(l->f[0]->height) + 1;
	for (y=l->y,yoffset=y*SCREENWIDTH ; y<l->y+lh ; y++,yoffset+=SCREENWIDTH)
	{
	    if (y < viewwindowy || y >= viewwindowy + viewheight)
		R_VideoErase(yoffset, SCREENWIDTH); // erase entire line
	    else
	    {
		R_VideoErase(yoffset, viewwindowx); // erase left border
		R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
		// erase right border
	    }
	}
    }

    lastautomapactive = automapactive;
    if (l->needsupdate) l->needsupdate--;
}

//-----------------------------------------------------------------------------

void
HUlib_initSText
( hu_stext_t*	s,
  int		x,
  int		y,
  int		h,
  patch_t**	font,
  int		startchar,
  boolean*	on )
{
    int i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i=0;i<h;i++)
	HUlib_initTextLine(&s->l[i],
			   x, y - i*(SHORT(font[0]->height)+1),
			   font, startchar);
}

//-----------------------------------------------------------------------------

void HUlib_addLineToSText(hu_stext_t* s)
{
    int i;

    // add a clear line
    if (++s->cl == s->h)
	s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i=0 ; i<s->h ; i++)
	s->l[i].needsupdate = 4;
}

//-----------------------------------------------------------------------------

void
HUlib_addMessageToSText
( hu_stext_t*	s,
  char*		prefix,
  char*		msg )
{
  HUlib_addLineToSText(s);
  if (prefix)
  {
	while (*prefix)
	    HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));
	// HUlib_addCharToTextLine(&s->l[s->cl], ':');
	// HUlib_addCharToTextLine(&s->l[s->cl], ' ');
  }

  while (*msg)
	HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

//-----------------------------------------------------------------------------

void HUlib_drawSText(hu_stext_t* s)
{
    int i, idx;
    hu_textline_t *l;

    if (!*s->on)
	return; // if not on, don't draw

    // draw everything
    for (i=0 ; i<s->h ; i++)
    {
	idx = s->cl - i;
	if (idx < 0)
	    idx += s->h; // handle queue of lines

	l = &s->l[idx];

	// need a decision made here on whether to skip the draw
	HUlib_drawTextLine(l, false); // no cursor, please
    }
}

//-----------------------------------------------------------------------------

void HUlib_eraseSText(hu_stext_t* s)
{
    int i;

    for (i=0 ; i<s->h ; i++)
    {
	if (s->laston && !*s->on)
	    s->l[i].needsupdate = 4;
	HUlib_eraseTextLine(&s->l[i]);
    }
    s->laston = *s->on;
}

//-----------------------------------------------------------------------------

void
HUlib_initIText
( hu_itext_t*	it,
  int		x,
  int		y,
  patch_t**	font,
  int		startchar,
  boolean*	on )
{
    it->lm = 0; // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}

//-----------------------------------------------------------------------------

// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t* it)
{
    if (it->l.len != it->lm)
	HUlib_delCharFromTextLine(&it->l);
}

//-----------------------------------------------------------------------------

void HUlib_eraseLineFromIText(hu_itext_t* it)
{
    while (it->lm != it->l.len)
	HUlib_delCharFromTextLine(&it->l);
}

//-----------------------------------------------------------------------------
// Resets left margin as well
void HUlib_resetIText(hu_itext_t* it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

//-----------------------------------------------------------------------------

void
HUlib_addPrefixToIText
( hu_itext_t*	it,
  char*		str )
{
    while (*str)
	HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

//-----------------------------------------------------------------------------
// wrapper function for handling general keyed input.
// returns true if it ate the key
boolean
HUlib_keyInIText
( hu_itext_t*	it,
  unsigned char ch )
{

    if (ch >= ' ' && ch <= '_')
  	HUlib_addCharToTextLine(&it->l, (char) ch);
    else
	if (ch == KEY_BACKSPACE)
	    HUlib_delCharFromIText(it);
	else
	    if (ch != KEY_ENTER)
		return false; // did not eat key

    return true; // ate the key
}

//-----------------------------------------------------------------------------

void HUlib_drawIText(hu_itext_t* it)
{

    hu_textline_t *l = &it->l;

    if (!*it->on)
	return;
    HUlib_drawTextLine(l, true); // draw the line w/ cursor
}

//-----------------------------------------------------------------------------

void HUlib_eraseIText(hu_itext_t* it)
{
    if (it->laston && !*it->on)
	it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}

//-----------------------------------------------------------------------------
