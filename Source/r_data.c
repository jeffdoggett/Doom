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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//	Preparation of data for rendering,
//	generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: r_data.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif

#include "includes.h"
#if defined(LINUX) || defined(HAVE_ALLOCA)
#include  <alloca.h>
#else
extern void * alloca (unsigned int);
#endif

// #define MIN_SIZE_LUMP	12

int		numflats;
int		numspritelumps;

int		firstpatch;
int		lastpatch;
int		numpatches;

int		numtextures;
texture_t**	textures;

static int*		texturewidthmask;
// needed for texture pegging
fixed_t*		textureheight;
static int*		texturecompositesize;
static dshort_t** 	texturecolumnlump;
static unsigned int**	texturecolumnofs;
static byte**		texturecomposite;

// for global animation
int*		flattranslation;
int*		texturetranslation;

int*		flatlumps;

// needed for pre rendering
fixed_t*	spritewidth;
fixed_t*	spriteoffset;
fixed_t*	spritetopoffset;

lighttable_t	*colormaps = 0;
static lighttable_t	*colourmaps_a = 0;
static unsigned int	colourmap_size = 0;
int		firstcollump;
int		lastcollump;
int		prevcollump = 0;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//


/* use this in case a patch is undefined */
static struct dummy_patch_s
{
  patch_t patch;
  column_t column;
} dummyPatch;	/* initialized in R_InitDummyPatch */

//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
static void
R_DrawColumnInCache
( const column_t*	patch,
  byte*		cache,
  int		originy,
  int		cacheheight,
  byte*		marks
)
{
    int		count;
    int		position;
    byte*	source;
    byte*	dest;
    int		td;
    int		topdelta;
    int		lastlength;

    topdelta = -1;
    lastlength = 0;
    dest = (byte *)cache + 3;

    while ((td = patch->topdelta) != 0xff)
    {
	if (td < (topdelta+(lastlength-1)))		// Bodge for oversize patches
	{
	  topdelta += td;
	}
	else
	{
	  topdelta = td;
	}

	source = (byte *)patch + 3;
	lastlength = count = patch->length;
	position = originy + topdelta;

	if (position < 0)
	{
	    count += position;
	    position = 0;
	}

	if (position + count > cacheheight)
	    count = cacheheight - position;

	if (count > 0)
	{
	    memcpy (cache + position, source, count);
	    memset (marks + position, 0xff, count);
	}
	patch = (column_t *)(  (byte *)patch + lastlength + 4);
    }
}



//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
/* Medusa fix taken from Boom sources */
static void R_GenerateComposite (int texnum)
{
    byte*		block;
    texture_t*		texture;
    texpatch_t*		patch;
    patch_t*		realpatch;
    unsigned int	patchsize;
    int			x;
    int			x1;
    int			x2;
    int			i;
    column_t*		patchcol;
    dshort_t*		collump;
    unsigned int*	colofs;
    byte*		marks;
    byte*		source;

    texture = textures[texnum];

    block = Z_Malloc (texturecompositesize[texnum],
		      PU_STATIC,
		      (void **)&texturecomposite[texnum]);

    if ((marks = (byte*)calloc(texture->width, texture->height)) == NULL)
	I_Error("R_GenerateComposite: couldn't alloc marks");

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    /* Composite the columns together.*/
    patch = texture->patches;

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	if (patch->patch == -1)
	{
	  realpatch = &dummyPatch.patch;
	  patchsize = sizeof(dummyPatch);
	}
	else
	{
	  realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
	  patchsize = (unsigned int)(lumpinfo[patch->patch].size);
	}

	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1<0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
	{
	  /* Column has multiple patches?*/
	  if (collump[x] < 0)
	  {
	    unsigned int pcoloff = LONG(realpatch->columnofs[x-x1]);;

	    if (pcoloff <= patchsize)
	    {
	      patchcol = (column_t *)((byte *)realpatch + pcoloff);
	      R_DrawColumnInCache (patchcol,
				   block + colofs[x],
				   patch->originy,
				   texture->height,
				   marks + x * texture->height);
	    }
	  }
	}
    }

    if ((source = malloc(texture->height)) == NULL)
	I_Error("R_GenerateComposite: couldn't alloc source");
    for (i=0; i<texture->width; i++)
    {
	if (collump[i] == -1)
	{
	    column_t *col = (column_t*)(block + colofs[i] - 3);
	    const byte* mark = marks + i * texture->height;
	    int j = 0;

	    memcpy(source, (byte*)col + 3, texture->height);

	    while (1)
	    {
		while (j < texture->height && !mark[j]) j++;
		if (j >= texture->height)
		{
		    col->topdelta = 0xff;
		    break;
		}
		col->topdelta = (byte)j;
		for (col->length=0; j<texture->height && mark[j]; j++) col->length++;
		memcpy((byte*)col + 3, source + col->topdelta, col->length);
		col = (column_t*)((byte*)col + col->length + 4);
	    }
	}
    }
    free(source);
    free(marks);

    /* Now that the texture has been built in column cache,*/
    /*  it is purgable from zone memory.*/
    Z_ChangeTag (block, PU_CACHE);
}



//
// R_GenerateLookup
//
static void R_GenerateLookup (int texnum)
{
    texture_t*		texture;
    texpatch_t*		patch;
    patch_t*		realpatch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    dshort_t*		collump;
    unsigned int*	colofs;
    int			csize;
    struct {dushort_t patches, posts;} *patchcount;
    unsigned int	lumpsize;

    texture = textures[texnum];

    /* Composited texture not created yet.*/
    texturecomposite[texnum] = 0;

    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];

    /* Now count the number of columns*/
    /*  that are covered by more than one patch.*/
    /* Fill in the lump / offset, so columns*/
    /*  with only a single patch are all done.*/
    patchcount = calloc (sizeof(*patchcount), texture->width);

    if (patchcount == NULL)
	I_Error("R_GenerateLookup: Couldn't claim memory for patchcount");

    patch = texture->patches;

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	if (patch->patch == -1)
	{
	  realpatch = &dummyPatch.patch;
	  lumpsize = sizeof(dummyPatch);
	}
	else
	{
	  realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
	  lumpsize = (unsigned int)lumpinfo[patch->patch].size;
	}

	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1 < 0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;
	for ( ; x<x2 ; x++)
	{
	    /* to fix Medusa bug */
	    const column_t *col;
	    unsigned int ofs;

	    ofs = LONG(realpatch->columnofs[x-x1]);

	    collump[x] = patch->patch;
	    colofs[x] = ofs+3;

	    /* safety */
	    if (ofs < lumpsize)
	    {
		col = (column_t*)((byte*)realpatch + ofs);
		while ((ofs < lumpsize) && (col->topdelta != 0xff))
		{
		  patchcount[x].posts++;
		  ofs += col->length + 4;
		  col = (column_t*)((byte*)realpatch + ofs);
		}
		patchcount[x].patches++;
	    }
	}
    }

    texturecomposite[texnum] = 0;

    for (x=0, csize=0; x<texture->width ; x++)
    {
	int	pcnt;

	pcnt = patchcount[x].patches;
	if (!pcnt)
	{
	  char namet [10];
	  strncpy (namet, texture->name, 8);
	  namet [8] = 0;
	  // fprintf (stderr, "R_GenerateLookup: column without a patch (%s)\n", namet);
	  texturecompositesize[texnum] = csize;
	  free (patchcount);
	  return;
	}
	/* I_Error ("R_GenerateLookup: column without a patch");*/

	if (pcnt > 1)
	{
	    /* Use the cached block.*/
	    collump[x] = -1;
	    colofs[x] = csize + 3;
	    csize += 4*patchcount[x].posts + 1;
	    /* changed texturecolumnofs, texture may safely be > 64k now */
#if 0
	    if (texturecompositesize[texnum] > 0x10000-texture->height)
	    {
		fprintf (stderr,"R_GenerateLookup: texture %i (%s) is >64k\n", texnum, texture->name);
	    }
#endif
	    csize += texture->height;
	}
	texturecompositesize[texnum] = csize;
    }
    free (patchcount);
}

/* ------------------------------------------------------------------------------------------------ */

static byte dummyColumn[256];
//
// R_GetColumn
//
byte*
R_GetColumn
( int		tex,
  int		col )
{
    int 	lump;
    int 	ofs;

    if (((unsigned int) tex) >= numtextures)  tex = 0;

    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];
    ofs = texturecolumnofs[tex][col];

    if (lump > 0)
    {
      if (ofs >= lumpinfo[lump].size) return (dummyColumn+4);
      return (byte *)W_CacheLumpNum(lump,PU_CACHE)+ofs;
    }

    if (!texturecomposite[tex])
	R_GenerateComposite (tex);

    return texturecomposite[tex] + ofs;
}

/* ------------------------------------------------------------------------------------------------ */

static const char mask_ffffffff[] = { 255,255,255,255,255,255,255,255 };

static unsigned int R_merge_pnames (pint * patchlookup)
{
  int lump;
  unsigned int pl_pos;
  unsigned int total;
  unsigned int qty_this_lump;
  char* names;
  char* name_p;
  char name[9];

  lump = 0;
  total = 0;
  pl_pos = 0;
  name [8] = 0;
  do
  {
    lump = W_CheckNumForNameMasked ("PNAMES", (char *) mask_ffffffff, lump - 1);
    if (lump == -1)
      break;

    names = W_CacheLumpNum (lump, PU_STATIC);
    if (names)
    {
      qty_this_lump = LONG ( *((int *)names) );
      total += qty_this_lump;
      if (qty_this_lump)
      {
	total += 2;
	if (patchlookup)
	{
	  patchlookup[pl_pos++] = qty_this_lump;
	  patchlookup[pl_pos++] = (pint)lumpinfo[lump].handle;
	  name_p = names + 4;
	  do
	  {
	    strncpy (name,name_p, 8);
	    patchlookup[pl_pos++] = W_CheckNumForName (name);
	    name_p += 8;
	  } while (--qty_this_lump);
	}
      }
      Z_Free (names);
    }
  } while (lump != 0);

  return (total);
}

/* ------------------------------------------------------------------------------------------------ */

static unsigned int R_read_textures (unsigned int* maptex, unsigned int pos,
			unsigned int maxoff, unsigned int nummappatches, pint *patchlookup)
{
  int k,p;
  unsigned int j;
  unsigned int offset;
  unsigned int qty_this_lump;
  //unsigned int totalwidth;
  unsigned int * directory;
  maptexture_t* mtexture;
  texture_t* texture;
  mappatch_t* mpatch;
  texpatch_t* patch;
  char name [sizeof(texture->name)+1];


  //totalwidth = 0;
  qty_this_lump = LONG(*maptex);
  directory = maptex+1;
  name [sizeof(texture->name)] = 0;
  do
  {
    if (!(pos&63))
      putchar ('.');

    offset = LONG(*directory);
    directory++;

    if (offset > maxoff)
      I_Error ("R_InitTextures: bad texture directory");

    mtexture = (maptexture_t *) ( (byte *)maptex + offset);

    strncpy (name, mtexture->name, sizeof(texture->name));
    p = R_CheckTextureNumForName (name);
    if (p != -1)		// Already seen this one?
    {
      // printf ("Already seen texture %u (%s) at position %u\n", pos, name, p);
#if 0
      textures[pos] = textures[p];
      texturecolumnlump[pos] = texturecolumnlump[p];
      texturecolumnofs[pos] = texturecolumnofs[p];
      texturecomposite[pos] = texturecomposite[p];
      texturecompositesize[pos] = texturecompositesize[p];
      texturewidthmask[pos] = texturewidthmask[p];
      textureheight[pos] = textureheight[p];
      pos++;
#endif
      continue;
    }
    // printf ("Adding texture %s at pos %u\n", name, pos);

    texture = textures[pos] = Z_Calloc (sizeof(texture_t)
				+ sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1),
				PU_STATIC, 0);

    texture->width = SHORT(mtexture->width);
    texture->height = SHORT(mtexture->height);
    texture->patchcount = SHORT(mtexture->patchcount);

    strncpy (texture->name, mtexture->name, sizeof(texture->name));
    patch = &texture->patches[0];
#ifdef PADDED_STRUCTS
    mpatch = (mappatch_t *) ((char *) mtexture + 22);
#else
    mpatch = &mtexture->patches[0];
#endif

    for (j=0 ; j<texture->patchcount ; j++, patch++)
    {
      patch->originx = SHORT(mpatch->originx);
      patch->originy = SHORT(mpatch->originy);
      k = SHORT(mpatch->patch);
      if ((k > nummappatches) || (k < 0))
      {
	I_Error ("R_InitTextures: Bad patch number %u/%u in texture %s",
			k, nummappatches, texture->name);
      }
      patch->patch = patchlookup[k];
      if (patch->patch == -1)
      {
	I_Error ("R_InitTextures: Missing patch in texture %s",
			texture->name);
      }
#ifdef PADDED_STRUCTS
      mpatch = (mappatch_t *)   ((unsigned char *) mpatch + 10);
#else
      mpatch++;
#endif
    }
    texturecolumnlump[pos] = Z_Calloc (texture->width*sizeof(dshort_t), PU_STATIC,0);
    texturecolumnofs[pos] = Z_Calloc (texture->width*sizeof(unsigned int), PU_STATIC,0);

    j = 1;
    while (j*2 <= texture->width)
      j<<=1;

    texturewidthmask[pos] = j-1;
    textureheight[pos] = texture->height<<FRACBITS;

    // totalwidth += texture->width;

    pos++;
  } while (--qty_this_lump);

  return (pos);
}

/* ------------------------------------------------------------------------------------------------ */
/*
   Ideally we want the PNAMES file in the same WAD file as the TEXTURES but
   this may not always be possible!
*/

static pint * R_find_matching_pnames (FILE * handle, pint *patchlookup)
{
  pint * pl;

  pl = patchlookup;

  while (pl [0])
  {
    if (pl [1] == (pint)handle)
      return (pl);

    pl = pl + pl [0] + 2;
  }

  return (patchlookup);
}

/* ------------------------------------------------------------------------------------------------ */

static unsigned int R_merge_textures (char * name, unsigned int total, pint *patchlookup)
{
  int lump;
  unsigned int maxoff;
  unsigned int qty_this_lump;
  unsigned int nummappatches;
  unsigned int* maptex;

  lump = 0;
  do
  {
    lump = W_CheckNumForNameMasked (name, (char *) mask_ffffffff, lump - 1);
    if (lump == -1)
      break;

    maptex = W_CacheLumpNum (lump, PU_STATIC);
    if (maptex)
    {
      if (patchlookup == NULL)
      {
	// printf ("Textures in lump %s = %u\n", name, qty_this_lump);
	qty_this_lump = LONG(*maptex);
	total += qty_this_lump;
      }
      else
      {
	maxoff = W_LumpLength (lump);
	patchlookup = R_find_matching_pnames (lumpinfo[lump].handle, patchlookup);
	nummappatches = patchlookup [0];
	total = R_read_textures (maptex, total, maxoff, nummappatches, patchlookup+2);
      }
      Z_Free (maptex);
    }
  } while (lump != 0);
  return (total);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures (void)
{
  unsigned int i,j;
  pint*	patchlookup;
  unsigned int	nummappatches;

  // Load the patch names from pnames.lmp.

  nummappatches = R_merge_pnames (NULL);
  // printf ("R_InitTextures: nummappatches = %u\n", nummappatches);
  patchlookup = malloc ((nummappatches+1)*sizeof(*patchlookup));
  if (patchlookup == NULL)
    I_Error("R_InitTextures: Couldn't claim memory for patchlookup");
  R_merge_pnames (patchlookup);
  patchlookup [nummappatches] = 0;

  j = R_merge_textures ("TEXTURE1", 0, NULL);
  numtextures = R_merge_textures ("TEXTURE2", j, NULL);
  // printf ("numtextures = %u\n", numtextures);

  textures = Z_Calloc (numtextures*sizeof(texture_t*), PU_STATIC, NULL);
  texturecolumnlump = Z_Calloc (numtextures*sizeof(dshort_t*), PU_STATIC, NULL);
  texturecolumnofs = Z_Calloc (numtextures*sizeof(unsigned int*), PU_STATIC, NULL);
  texturecomposite = Z_Calloc (numtextures*sizeof(byte*), PU_STATIC, NULL);
  texturecompositesize = Z_Calloc (numtextures*sizeof(int), PU_STATIC, NULL);
  texturewidthmask = Z_Calloc (numtextures*sizeof(int), PU_STATIC, NULL);
  textureheight = Z_Calloc (numtextures*sizeof(fixed_t), PU_STATIC, NULL);

  j = R_merge_textures ("TEXTURE1", 0, patchlookup);
  j = R_merge_textures ("TEXTURE2", j, patchlookup);
  // printf ("numtextures = %u -> %u\n", numtextures, j);

  if (j > numtextures)
    I_Error ("R_InitTextures: Too many textures!");

  if (j < numtextures)
  {
    numtextures = j;
    textures = Z_Realloc (textures,numtextures*sizeof(texture_t*), PU_STATIC, NULL);
    texturecolumnlump = Z_Realloc (texturecolumnlump,numtextures*sizeof(dshort_t*), PU_STATIC, NULL);
    texturecolumnofs = Z_Realloc (texturecolumnofs,numtextures*sizeof(unsigned int*), PU_STATIC, NULL);
    texturecomposite = Z_Realloc (texturecomposite,numtextures*sizeof(byte*), PU_STATIC, NULL);
    texturecompositesize = Z_Realloc (texturecompositesize,numtextures*sizeof(int), PU_STATIC, NULL);
    texturewidthmask = Z_Realloc (texturewidthmask,numtextures*sizeof(int), PU_STATIC, NULL);
    textureheight = Z_Realloc (textureheight,numtextures*sizeof(fixed_t), PU_STATIC, NULL);
  }

  // Precalculate whatever possible.
  for (i=0 ; i<numtextures ; i++)
  {
    if ((i & 255) == 0)
      putchar ('.');
    R_GenerateLookup (i);
  }

  // Create translation table for global animation.
  texturetranslation = Z_Malloc ((numtextures+1)*sizeof(int), PU_STATIC, NULL);

  for (i=0 ; i<numtextures ; i++)
    texturetranslation[i] = i;

  free (patchlookup);
}

/* ------------------------------------------------------------------------------------------------ */
/* Count the number of unique lumps between start and end
** We have to discard duplicates. In the case of sprites this
** is really tricky because of flipped ones.
** For instance in osiris/wad there's a sprite called SARGH2H8
** which replaces both SARGH2 and SARGH8.
** In fact for completeness, it would also replace SARGH8H2.
**
** So when I parse SARG A1B2 I must also check for SARG xxA1,
** SARG A1xx, SARG xxB2 and SARG B2xx.
*/
static const char mask_ffff00ff[] = { 255,255,255,255,0,0,255,255 };
static const char mask_fffff300[] = { 255,255,255,255,255,0x30,0,0 };
static const char mask_ffffff00[] = { 255,255,255,255,255,255,0,0 };

static int R_CountEntities (char * start, char * end, int doing_sprites)
{
  int lump;
  int total;
  int loading;
  int valid;
  int i;
  lumpinfo_t* lump_ptr;
  char sstart [12];
  char eend  [12];
  char sprname [12];

  /* Some pwads have names with the first letter duplicated */
  /* e.g. S_START becomes SS_START */
  sstart [0] = start [0];
  strcpy (&sstart[1], start);

  eend [0] = end [0];
  strcpy (&eend[1], end);

  total = 0;
  lump = numlumps;
  loading = 0;
  lump_ptr = &lumpinfo[lump];

  do
  {
    lump--;
    lump_ptr--;

    if (loading == 0)
    {
      if ((strncasecmp (lump_ptr->name, end, 8) == 0)
       || (strncasecmp (lump_ptr->name, eend, 8) == 0))
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, start, 8) == 0)
	  || (strncasecmp (lump_ptr->name, sstart, 8) == 0)
	  || (lump_ptr->handle != (lump_ptr+1)->handle))
    {
      loading = 0;
    }
    else
    {
      valid = true;   /* Assume ok */

      if ((lump_ptr->name[0] == 0)		/* If this hasn't already been invalidated */
#ifdef MIN_SIZE_LUMP
       || (lump_ptr->size < MIN_SIZE_LUMP)
#endif
       || (strncasecmp (lump_ptr->name, end, 8) == 0)	/* Nested? */
       || (strncasecmp (lump_ptr->name, eend, 8) == 0)
       || (W_CheckNumForNameLinear (lump_ptr->name) != lump)) /* If there's another one at a higher pos */
      {
	valid = false;
      }
      else if (doing_sprites)
      {
	if (lump_ptr->name[6])		/* Does it have a flipped section ? */
	{
	  memcpy (sprname, lump_ptr->name, 8);
	  sprname [8] = 0;
	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffff00ff, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  if (sprname [5] == '0')	/* If this a non-rotated sprite then */
	  {				/* Remove all rotated sprites */
    	    i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, lump - 1);
    	    while (i != -1)
    	    {
     	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
    	      i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, i - 1);
    	    }
	  }
	  else				/* Having found a rotated sprite, remove */
	  {				/* any non-rotated ones below */
	    sprname [5] = '0';
	    i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	    if (i != -1)
	    {
	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	    }
	    memcpy (sprname, lump_ptr->name, 8);
	  }

	  sprname [4] = lump_ptr->name [6];
	  sprname [5] = lump_ptr->name [7];
	  sprname [6] = lump_ptr->name [4];
	  sprname [7] = lump_ptr->name [5];

	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffff00ff, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  if (sprname [5] == '0')	/* If this a non-rotated sprite then */
	  {				/* Remove all rotated sprites */
    	    i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, lump - 1);
    	    while (i != -1)
    	    {
     	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
    	      i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, i - 1);
    	    }
	  }
	  else				/* Having found a rotated sprite, remove */
	  {				/* any non-rotated ones below */
	    sprname [5] = '0';
	    i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	    if (i != -1)
	    {
	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	    }
	  }
	}
	else
	{
	  memcpy (sprname, lump_ptr->name, 8);
	  sprname [8] = 0;
	  sprname [6] = lump_ptr->name [4];
	  sprname [7] = lump_ptr->name [5];

	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffff00ff, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	  if (i != -1)
	  {
	    lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	  }

	  if (sprname [5] == '0')	/* If this a non-rotated sprite then */
	  {				/* Remove all rotated sprites */
    	    i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, lump - 1);
    	    while (i != -1)
    	    {
     	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
    	      i = W_CheckNumForNameMasked (sprname, (char *) mask_fffff300, i - 1);
    	    }
	  }
	  else				/* Having found a rotated sprite, remove */
	  {				/* any non-rotated ones below */
	    sprname [5] = '0';
	    i = W_CheckNumForNameMasked (sprname, (char *) mask_ffffff00, lump - 1);
	    if (i != -1)
	    {
	      lumpinfo[i].name[0] = 0;	/* Lose the lower one */
	    }
	  }
	}
      }

      if (valid == true)		/* If this is the first one */
      {
	total++;
	if ((total & 127) == 0)
	  putchar ('.');
      }
      else
      {
	lump_ptr->name[0] = 0;		/* It's a duplicate, so destroy it! */
      }
    }
  } while (lump);

  // printf ("Found %d unique entities between %s and %s or %s and %s\n", total, start, end, sstart, eend);
  return (total);
}



//
// R_InitFlats
//
void R_InitFlats (void)
{
  unsigned int	i;
  unsigned int	lump;
  unsigned int	loading;
  lumpinfo_t*	lump_ptr;

  numflats = R_CountEntities ("F_START", "F_END", 0);

  // Create translation table for global animation.
  flattranslation = Z_Malloc ((numflats+1)*sizeof(int), PU_STATIC, 0);

  // Create translation table for flat lumps.
  flatlumps = Z_Malloc ((numflats+1)*sizeof(int), PU_STATIC, 0);

  i = 0;
  lump = 0;
  loading = 0;
  lump_ptr = lumpinfo;
  do
  {
    if (loading == 0)
    {
      if ((strncasecmp (lump_ptr->name, "F_START", 8) == 0)
       || (strncasecmp (lump_ptr->name, "FF_START", 8) == 0))
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, "F_END", 8) == 0)
	  || (strncasecmp (lump_ptr->name, "FF_END", 8) == 0)
	  || (lump_ptr->handle != (lump_ptr-1)->handle))
    {
      loading = 0;
    }
    else
    if ((lump_ptr->name[0])
#ifdef MIN_SIZE_LUMP
     && (lump_ptr->size >= MIN_SIZE_LUMP)
#endif
     && (strncasecmp (lump_ptr->name, "F_START", 8))
     && (strncasecmp (lump_ptr->name, "FF_START", 8)))
    {
      if (!(i&63))
	putchar ('.');
      if (i < numflats)
      {
	flattranslation[i] = i;
	flatlumps[i] = lump;
      }
      i++;
    }
    lump_ptr++;
  } while (++lump < numlumps);

  if (numflats != i)
  {
    printf ("\nnumflats = %u, found = %u\n", numflats, i);
    if (i < numflats)
      numflats = i;
  }
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps (void)
{
  unsigned int	i;
  unsigned int	lump;
  unsigned int	loading;
  patch_t	*patch;
  lumpinfo_t*	lump_ptr;

  numspritelumps = R_CountEntities ("S_START", "S_END", 1);

  spritewidth = Z_Malloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
  spriteoffset = Z_Malloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
  spritetopoffset = Z_Malloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);

  i = 0;
  lump = 0;
  loading = 0;
  lump_ptr = lumpinfo;

  do
  {
    if (loading == 0)
    {
      if ((strncasecmp (lump_ptr->name, "S_START", 8) == 0)
       || (strncasecmp (lump_ptr->name, "SS_START", 8) == 0))
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, "S_END", 8) == 0)
	  || (strncasecmp (lump_ptr->name, "SS_END", 8) == 0)
	  || (lump_ptr->handle != (lump_ptr-1)->handle))
    {
      loading = 0;
    }
    else
    if ((lump_ptr->name[0])
#ifdef MIN_SIZE_LUMP
     && (lump_ptr -> size >= MIN_SIZE_LUMP)
#endif
     && (strncasecmp (lump_ptr->name, "S_START", 8))
     && (strncasecmp (lump_ptr->name, "SS_START", 8)))
    {
      if (!(i&63))
	putchar ('.');
      patch = W_CacheLumpNum (lump, PU_CACHE);
      if (i < numspritelumps)
      {
	spritewidth[i] = SHORT(patch->width)<<FRACBITS;
	spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
	spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;
      }
      i++;
    }
    lump_ptr++;
  } while (++lump < numlumps);

  if (numspritelumps != i)
  {
    printf ("\nnumspritelumps = %u, found = %u\n", numspritelumps, i);
    if (i < numspritelumps)
      numspritelumps = i;
  }
}



//
// R_InitColormaps
//
void R_InitColormaps (int lump)
{
  int length;

  if (lump == 0)
  {
    lump = W_GetNumForName("COLORMAP");
    firstcollump = W_CheckNumForName ("C_START");
    lastcollump = W_CheckNumForName ("C_END");
  }

  if ((lump != prevcollump) || (colourmap_size == 0))
  {
    prevcollump = lump;

    length = W_LumpLength (lump) + 255;
    if (length > colourmap_size)
    {
      colourmap_size = length + 0x200;

      if (colourmaps_a)
	Z_Free (colourmaps_a);

      // Load in the light tables,
      colourmaps_a = Z_Malloc (colourmap_size, PU_STATIC, 0);
      // 256 byte align tables.
      colormaps = (byte *)( ((pint)colourmaps_a + 255)&~0xff);
    }
    W_ReadLump (lump,colormaps);
  }
}

#ifndef PADDED_STRUCTS
static void R_CheckStructs (void)
{
  if (sizeof (mappatch_t) != 10)
  {
    I_Error ("Your compiler has padded structures. Please recompile with -DPADDED_STRUCTS");
  }
}
#endif


static void R_InitDummyPatch(void)
{
    unsigned int i, off;
    patch_t *patch = &dummyPatch.patch;
    column_t *column = &dummyPatch.column;

    /* the dummy patch must pretend to be in external endianness! */
    patch->width = SHORT(1);
    patch->height = SHORT(1);
    patch->leftoffset = SHORT(0);
    patch->topoffset = SHORT(0);
    off = (byte*)(&dummyPatch.column) - (byte*)(&dummyPatch.patch);
    for (i=0; i<8; i++)
	dummyPatch.patch.columnofs[i] = LONG(off);

    column->topdelta = 0xff;
    column->length = 0x00;

    /*printf("Size: %d\n", sizeof(dummyPatch));
    for (i=0; i<sizeof(dummyPatch); i++)
      printf("%2d: %2x\n", i, ((byte*)&dummyPatch)[i]);*/
}

//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
    R_InitDummyPatch();
#ifndef PADDED_STRUCTS
    R_CheckStructs ();
#endif
    printf ("\nInitTextures");
    R_InitTextures ();
    printf ("\nInitFlats");
    R_InitFlats ();
    printf ("\nInitSprites");
    R_InitSpriteLumps ();
    //printf ("\nInitColormaps");
    R_InitColormaps (0);
    memset (dummyColumn, 0xff, sizeof(dummyColumn));
    printf ("\n");
}


//
// R_CheckFlatNumForName
// Returns -1 if name not found.
//

int R_CheckFlatNumForName (const char* name)
{
    union {
	char	s[8];
	int	x[2];
    } name8;

    int 	v1;
    int 	v2;
    unsigned int c;
    unsigned int p,cc;
    char *	ptr_d;

    lumpinfo_t* lump_p;

    // make the name into two integers for easy compares

    name8.x[0] = 0;
    name8.x[1] = 0;

    p = 8;
    ptr_d = name8.s;
    do
    {
      cc = *name++;
      if (cc == 0)
	break;
      cc = toupper (cc);	// case insensitive
      *ptr_d++ = cc;
    } while (--p);

    v1 = name8.x[0];
    v2 = name8.x[1];

    c = 0;
    do
    {
	lump_p = &lumpinfo[flatlumps[c]];
	/* printf ("  Comparing %s %X %X with %X %X %s (%d, %d)\n", name8.s, v1, v2, *(int *) lump_p->name, *(int *) &lump_p->name[4], lump_p->name, c, flatlumps[c]);	*/
	if ( *(int *)lump_p->name == v1
	     && *(int *)&lump_p->name[4] == v2)
	{
	    return c;
	}
    }
    while (++c < numflats);

    // TFB. Not found.
    return -1;
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (const char* name)
{
  int 	i;
  char	namet[9];

  i = R_CheckFlatNumForName (name);

  /* Emergency check for broken pwads! */
  if (i == -1)
    i = W_CheckNumForName (name);

  if (i == -1)
  {
    if (M_CheckParm ("-ignorebadtextures"))
      return (0);

    namet[8] = 0;
    memcpy (namet, name,8);
    I_Error ("R_FlatNumForName: %s not found",namet);
  }
  return i;
}




//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName (const char *name)
{
  int 	i;
  texture_t**	ptr_1;
  texture_t*	ptr_2;

  // "NoTexture" marker.
  if (name[0] == '-')
    return 0;

  i = 0;
  ptr_1 = textures;

  do
  {
    ptr_2 = *ptr_1;
    if ((ptr_2)
     && (strncasecmp (ptr_2->name, name, 8) == 0))
      return (i);
    ptr_1++;
  } while (++i < numtextures);

  return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int R_TextureNumForName (const char* name)
{
  int 	i;
  int	j;
  char	namet[9];

  i = R_CheckTextureNumForName (name);
  if (i == -1)
  {
    j = W_CheckNumForName (name);	// Allow for broken pwads
    if (j != -1)
    {
#if 0
      if ((j > firstcollump)
       && (j < lastcollump))
      {
	if (M_CheckParm ("-noaltcolourmaps") == 0)
	{
	  R_InitColormaps (j);
	  R_InitLightTables ();
	}
      }
#endif
      return (j);
    }

    if ((M_CheckParm ("-ignorebadtextures") == 0)
     && (name [0] > ' '))
    {
      namet[8] = 0;
      memcpy (namet, name,8);
      printf ("R_TextureNumForName: %s not found\n", namet);
    }
    i = 0;
  }
  return i;
}




//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
int		flatmemory;
int		texturememory;
int		spritememory;

void R_PrecacheLevel (void)
{
    char*		flatpresent;
    char*		texturepresent;
    char*		spritepresent;

    int 		f;
    int 		i;
    int 		j;
    int 		k;
    int 		s;
    int 		t;
    int 		lump;

    texture_t*		texture;
    thinker_t*		th;
    spriteframe_t*	sf;

    if (demoplayback)
	return;

    // Precache flats.
    flatpresent = alloca(numflats);
    memset (flatpresent,0,numflats);

    for (i=0 ; i<numsectors ; i++)
    {
      f = sectors[i].floorpic;
      if (f < numflats)
	flatpresent[f] = 1;
      f = sectors[i].ceilingpic;
      if (f < numflats)
	flatpresent[f] = 1;
    }

    flatmemory = 0;

    for (i=0 ; i<numflats ; i++)
    {
	if (flatpresent[i])
	{
	    lump = flatlumps [i];
	    flatmemory += lumpinfo[lump].size;
	    W_CacheLumpNum(lump, PU_CACHE);
	}
    }

    // Precache textures.
    texturepresent = alloca(numtextures);
    memset (texturepresent,0, numtextures);

    for (i=0 ; i<numsides ; i++)
    {
      t = sides[i].toptexture;
      if (t < numtextures)
	texturepresent[t] = 1;
      t = sides[i].midtexture;
      if (t < numtextures)
	texturepresent[t] = 1;
      t = sides[i].bottomtexture;
      if (t < numtextures)
	texturepresent[t] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //	indicate a sky floor/ceiling as a flat,
    //	while the sky texture is stored like
    //	a wall texture, with an episode dependent
    //	name.

    t = skytexture;
    if (t < numtextures)
      texturepresent[t] = 1;

    texturememory = 0;
    for (i=0 ; i<numtextures ; i++)
    {
	if (!texturepresent[i])
	    continue;

	texture = textures[i];

	for (j=0 ; j<texture->patchcount ; j++)
	{
	    lump = texture->patches[j].patch;
	    texturememory += lumpinfo[lump].size;
	    W_CacheLumpNum(lump , PU_CACHE);
	}
    }

    // Precache sprites.
    spritepresent = alloca(numsprites);
    memset (spritepresent,0, numsprites);

    for (th = thinker_head ; th != NULL ; th=th->next)
    {
	if (th->function.acp1 == (actionf_p1)P_MobjThinker)
	{
	  s = ((mobj_t *)th)->sprite;
	  if (s < numsprites)
	    spritepresent[s] = 1;
	}
    }

    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
	if (!spritepresent[i])
	    continue;

	for (j=0 ; j<sprites[i].numframes ; j++)
	{
	    sf = &sprites[i].spriteframes[j];
	    for (k=0 ; k<8 ; k++)
	    {
		lump = sf->lump[k];
		spritememory += lumpinfo[lump].size;
		W_CacheLumpNum(lump , PU_CACHE);
	    }
	}
    }
}




