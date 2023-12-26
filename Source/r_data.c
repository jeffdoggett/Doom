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

//-----------------------------------------------------------------------------
// #define MIN_SIZE_LUMP	12
#define REQ_FLAT_SIZE		4096
//-----------------------------------------------------------------------------

int		numflats;
int		numspritelumps;

int		numtextures;

texture_info_t* textures;

flatinfo_t*	flatinfo;

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

/* ------------------------------------------------------------------------------------------------ */
//
// R_GetTextureColumn
//
byte *R_GetTextureColumn(rpatch_t *texpatch, int col)
{
    unsigned int width = texpatch->width;

    while (col < 0)
	col += width;

    if (width & (width - 1))		// If width is not a power of 2
      col %= width;			// then use mod instead.
    else
      col &= (width - 1);

    return texpatch->columns[col].pixels;
}

/* ------------------------------------------------------------------------------------------------ */

static int R_DuplicateTexture (const char * name, int qty)
{
  int pos;

  pos = 0;
  do
  {
    if (strncasecmp (textures[pos].texture->name, name, sizeof (textures[pos].texture->name)) == 0)
      return (1);
  } while (++pos < qty);

  return (0);
}

/* ------------------------------------------------------------------------------------------------ */

static void R_ReadTextures (int names_lump, int maptex_lump_1, int maptex_lump_2)
{
    const maptexture_t	*mtexture;
    texture_t		*texture;
    int			i, j;
    const int		*maptex1;
    const int		*maptex2;
    const char		*names; // cph -
    const char		*name_p;// const*'s
    int			*patchlookup;
    int			texbase;
    int			numtex;
    int			nummappatches;
    int			maxoff, maxoff2;
    int			numtextures1, numtextures2;
    const int		*directory;
    char		name[9];

    // Load the patch names from pnames.lmp.
    name[8] = '\0';
    names = W_CacheLumpNum(names_lump, PU_STATIC);
    nummappatches = LONG(*((const int *)names));
    name_p = names + 4;
    patchlookup = malloc(nummappatches * sizeof(*patchlookup));   // killough

    for (i = 0; i < nummappatches; i++)
    {
	strncpy(name, name_p + i * 8, 8);
	patchlookup[i] = W_CheckNumForName(name);
    }
    W_ReleaseLumpNum(names_lump);       // cph - release the lump

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex1 = W_CacheLumpNum(maptex_lump_1, PU_STATIC);
    numtextures1 = LONG(*maptex1);
    maxoff = W_LumpLength(maptex_lump_1);
    directory = maptex1 + 1;

    if (maptex_lump_2 != -1)
    {
	maptex2 = W_CacheLumpNum(maptex_lump_2, PU_STATIC);
	numtextures2 = LONG(*maptex2);
	maxoff2 = W_LumpLength(maptex_lump_2);
    }
    else
    {
	maptex2 = NULL;
	numtextures2 = 0;
	maxoff2 = 0;
    }

    texbase = numtextures;
    numtex = numtextures1 + numtextures2;
    numtextures += numtex;

    // killough 4/9/98: make column offsets 32-bit;
    // clean up malloc-ing to use sizeof
    textures = Z_Realloc (textures, (numtextures+1) * sizeof(*textures), PU_STATIC, 0);

    for (i = 0; i < numtex; ++directory)
    {
	const mappatch_t *mpatch;
	texpatch_t	*patch;
	int		offset;

	if (i == numtextures1)
	{
	    // Start looking in second texture file.
	    maptex1 = maptex2;
	    maxoff = maxoff2;
	    directory = maptex1 + 1;
	}

	if ((((byte *) directory) >= ((byte *)maptex1+maxoff))
	 || ((offset=LONG(*directory)) >= maxoff))
	{
	  printf ("R_InitTextures: bad texture directory %X/%X %d/%d\n",
		(uintptr_t)directory,((uintptr_t)maptex1+maxoff), offset, maxoff);
	  break;
	}

	mtexture = (const maptexture_t *)((const byte *)maptex1 + offset);

	if ((texbase)
	 && (R_DuplicateTexture (mtexture->name, texbase)))
	{
	  numtex--;
	  numtextures--;
	  numtextures1--;
	  continue;
	}

	texture = textures[texbase+i].texture = Z_Malloc(sizeof(texture_t) + sizeof(texpatch_t)
	    * (SHORT(mtexture->patchcount) - 1), PU_STATIC, 0);

	texture->width = SHORT(mtexture->width);
	texture->height = SHORT(mtexture->height);
	texture->patchcount = SHORT(mtexture->patchcount);
	// killough 1/31/98: Initialize texture hash table
	texture->index = -1;

	{
	    size_t      j;

	    for (j = 0; j < sizeof(texture->name); ++j)
		texture->name[j] = mtexture->name[j];
	}

#ifdef PADDED_STRUCTS
	mpatch = (mappatch_t *) ((char *) mtexture + 22);
#else
	mpatch = &mtexture->patches[0];
#endif
	patch = texture->patches;

	for (j = 0; j < texture->patchcount; ++j, ++patch)
	{
	    patch->originx = SHORT(mpatch->originx);
	    patch->originy = SHORT(mpatch->originy);
	    patch->patch = patchlookup[SHORT(mpatch->patch)];
	    if (patch->patch == -1)
		printf("Missing patch %d in texture %.8s.", SHORT(mpatch->patch),
		    texture->name);     // killough 4/17/98
#ifdef PADDED_STRUCTS
	    mpatch = (mappatch_t *)   ((unsigned char *) mpatch + 10);
#else
	    mpatch++;
#endif
	}

	if ((texture->width & (texture->width - 1))
	 && (M_CheckParm ("-showunknown")))
	  printf ("Texture %0.8s has non power of 2 width %u\n", texture->name, texture->width);

	textures[texbase+i].height = texture->height << FRACBITS;
	textures[texbase+i].pnames_lump = names_lump;
	i++;	// Done here so that 'continue' misses it out...
    }

    free(patchlookup);	  // killough

    // cph - release the TEXTUREx lumps
    W_ReleaseLumpNum (maptex_lump_1);
    if (maptex_lump_2 != -1)
      W_ReleaseLumpNum (maptex_lump_2);
}

/* ------------------------------------------------------------------------------------------------ */

static void R_ReadTextureSet (int names_lump, int maptex_lump_1, int maptex_lump_2)
{
  if (M_CheckParm ("-showtextureset"))
    printf ("\nR_ReadTextureSet (%d, %d, %d)", names_lump, maptex_lump_1, maptex_lump_2);

  if (maptex_lump_1 == -1)			// Some PWADS only have a TEXTURES2 and no TEXTURES1
  {
    maptex_lump_1 = maptex_lump_2;
    maptex_lump_2 = -1;
  }
  if (maptex_lump_1 != -1)
  {
    if (names_lump == -1)
      names_lump = W_GetNumForName ("PNAMES");
    R_ReadTextures (names_lump, maptex_lump_1, maptex_lump_2);
  }
}

/* ------------------------------------------------------------------------------------------------ */
/*
   Sigil.wad has two sets of PNAMES,TEXTURES1 & TEXTURES2 so just load them all.
*/

static void R_MergeTextures (void)
{
  int lump;
  int names_lump;
  int maptex_lump_1;
  int maptex_lump_2;
  FILE * handle;

  if (M_CheckParm ("-alwaysmergetextures") == 0)
  {
    // Read the three lumps first in the standard way.
    names_lump    = W_CheckNumForName ("PNAMES");
    maptex_lump_1 = W_CheckNumForName ("TEXTURE1");
    maptex_lump_2 = W_CheckNumForName ("TEXTURE2");
    R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
  }

  if (M_CheckParm ("-nomergetextures"))
    return;

  // Then use a merge system.
  lump = numlumps;
  names_lump = -1;
  maptex_lump_1 = -1;
  maptex_lump_2 = -1;
  handle = lumpinfo[numlumps-1].handle;
  do
  {
    lump--;
    if (lumpinfo[lump].handle != handle)
    {
      R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
      names_lump = -1;
      maptex_lump_1 = -1;
      maptex_lump_2 = -1;
      handle = lumpinfo[lump].handle;
    }

    if (strcasecmp (lumpinfo[lump].name, "PNAMES") == 0)
    {
      if (names_lump != -1)	// Already seen one?
      {
        R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
        names_lump = -1;
        maptex_lump_1 = -1;
        maptex_lump_2 = -1;
      }
      names_lump = lump;
    }
    else if (strncasecmp (lumpinfo[lump].name, "TEXTURE1", 8) == 0)
    {
      if (maptex_lump_1 != -1)
      {
        R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
        names_lump = -1;
        maptex_lump_1 = -1;
        maptex_lump_2 = -1;
      }
      maptex_lump_1 = lump;
    }
    else if (strncasecmp (lumpinfo[lump].name, "TEXTURE2", 8) == 0)
    {
      if (maptex_lump_2 != -1)
      {
        R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
        names_lump = -1;
        maptex_lump_1 = -1;
        maptex_lump_2 = -1;
      }
      maptex_lump_2 = lump;
    }
  } while (lump);

  R_ReadTextureSet (names_lump, maptex_lump_1, maptex_lump_2);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
static void R_InitTextures (void)
{
    int i;

    textures = NULL;
    numtextures = 0;

    R_MergeTextures ();

    textures = Z_Realloc (textures, (numtextures+1) * sizeof(*textures), PU_STATIC, 0);

    for (i = 0; i < numtextures; ++i)
	textures[i].translation = i;

    while (--i >= 0)
    {
	int j;

	j = W_LumpNameHash(textures[i].texture->name) % (unsigned int)numtextures;
	textures[i].texture->next = textures[j].texture->index; // Prepend to chain
	textures[j].texture->index = i;
    }
#if 0
    // [BH] Initialize partially fullbright textures.
    texturefullbright = Z_Malloc(numtextures * sizeof(*texturefullbright), PU_STATIC, 0);

    memset(texturefullbright, 0, numtextures * sizeof(*texturefullbright));
    if (r_brightmaps)
    {
	i = 0;
	while (fullbright[i].colormask)
	{
	    if (fullbright[i].texture)
	    {
		int num = R_CheckTextureNumForName(fullbright[i].texture);

		if (num != -1)
		    texturefullbright[num] = fullbright[i].colormask;
		i++;
	    }
	}
    }
#endif
}

/* ------------------------------------------------------------------------------------------------ */

#define R_NameCompare(a,b)		\
	((name1 [a] == name2 [b])	\
	&& (((c1 = name1 [a+1]) == '0')	\
	 || ((c2 = name2 [b+1]) == '0')	\
	 || (c1 == c2)))

/* ------------------------------------------------------------------------------------------------ */

static int R_CanRemove (const char * name1, const char * name2)
{
  int rc;
  char c1,c2;

  if ((name1 [5] == 0)
   || (name2 [5] == 0))
  {
    if (name1 [4] == name2 [4])
      return (3);
    return (0);
  }

  rc = 0;

  if ((R_NameCompare (4,4))
   || ((name2 [6]) && (R_NameCompare (4,6))))
    rc |= 1;

  if ((name1 [6])
   && ((R_NameCompare (6,4))
   || ((name2 [6]) && (R_NameCompare (6,6)))))
    rc |= 2;

  return (rc);
}

/* ------------------------------------------------------------------------------------------------ */

// #define SHOW_SPRITE_MATCHING

static void R_RemoveDuplicateSprites (const char * name, int sprlump)
{
  int pos;
  int loading;
  int lump;
  lumpinfo_t* lump_ptr;

  lump = 0;
  loading = 0;
  lump_ptr = &lumpinfo[0];

  while (lump < sprlump)
  {
    if (loading == 0)
    {
      if (strncasecmp (lump_ptr->name, "S_START", 8) == 0)
	loading = 1;
    }
    else if ((strcasecmp (lump_ptr->name, "S_END") == 0)
	  || (lump_ptr->handle != (lump_ptr-1)->handle))
    {
      loading = 0;
    }
    else
    {
      if (strncasecmp (lump_ptr -> name, name, 4) == 0)
      {
	pos = R_CanRemove (lump_ptr -> name, name);
#ifdef SHOW_SPRITE_MATCHING
	if (pos)
	  printf ("Result = %X (%s)(%s)", pos, lump_ptr -> name, name);
#endif
	switch (pos)
	{
	  case 1:
	    if (lump_ptr -> name [4] && lump_ptr -> name [6])
	    {
	      lump_ptr -> name [4] = '@';
	    }
	    else
	    {
	      lump_ptr -> name [0] = 0;
	    }
	    break;

	  case 2:
	    if (lump_ptr -> name [4] == '@')
	    {
	      lump_ptr -> name [0] = 0;
	    }
	    else
	    {
	      lump_ptr -> name [6] = 0;
	      lump_ptr -> name [7] = 0;
	    }
	    break;

	  case 3:
	    lump_ptr -> name [0] = 0;
	    break;
	}
#ifdef SHOW_SPRITE_MATCHING
	if (pos)
	  printf (" To (%s)\n", lump_ptr -> name);
#endif
      }
    }
    lump_ptr++;
    lump++;
  }
}

/* ------------------------------------------------------------------------------------------------ */
/* Voe.wad has a sprite that is a PNG. For now we just ignore it! */

static boolean R_SpriteValid (lumpinfo_t* lump_ptr)
{
  FILE*	handle;

  if (lump_ptr -> size < 4)
    return (false);

  handle = lump_ptr -> handle;
  if (handle)
  {
    fseek (handle, lump_ptr->position, SEEK_SET);
    if ((fgetc (handle) == 0x89)
     && (fgetc (handle) == 'P')
     && (fgetc (handle) == 'N')
     && (fgetc (handle) == 'G'))
    {
      if (M_CheckParm ("-showunknown"))
	printf ("Sprite %.8s is a PNG\n", lump_ptr -> name);
      return (false);
    }
  }

  return (true);
}

/* ------------------------------------------------------------------------------------------------ */
/*
   See if there is another lump by this name in a higher slot.
*/

static boolean R_DuplicateName (lumpinfo_t* lump_ptr, const char * start, const char * end)
{
  boolean checking;
  lumpinfo_t* lump_ptr2;

  checking = true;
  lump_ptr2 = lump_ptr;
  do
  {
    lump_ptr2++;
    if (checking)
    {
      if (strncasecmp (lump_ptr->name, lump_ptr2->name, 8) == 0)
        return (true);
      if ((strncasecmp (lump_ptr2->name, end, 8) == 0)
       || (lump_ptr2->handle != (lump_ptr2-1)->handle))
        checking = false;
    }
    else if (strncasecmp (lump_ptr2->name, start, 8) == 0)
    {
      checking = true;
    }
  } while (lump_ptr2 < &lumpinfo[numlumps-1]);

  return (false);
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

static int R_CountEntities (const char * start, const char * end, int doing_sprites)
{
  int lump;
  int total;
  int loading;
  int valid;
  lumpinfo_t* lump_ptr;

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
      if (strncasecmp (lump_ptr->name, end, 8) == 0)
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, start, 8) == 0)
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
       || (R_DuplicateName (lump_ptr, start, end))) /* If there's another one at a higher pos */
      {
	valid = false;
      }
      else if (doing_sprites)
      {
	if (R_SpriteValid (lump_ptr) == false)
	  valid = false;
	else
	  R_RemoveDuplicateSprites (lump_ptr->name, lump);
      }

      if (valid == true)		/* If this is the first one */
      {
	total++;
	if ((total & 127) == 0)
	  putchar ('.');
      }
      else
      {
	// printf ("Lump %u removed (%s)\n", lump, lump_ptr->name);
	lump_ptr->name[0] = 0;		/* It's a duplicate, so destroy it! */
      }
    }
  } while (lump);

  // printf ("Found %d unique entities between %s and %s or %s and %s\n", total, start, end, sstart, eend);
  return (total);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitFlats
//
static void R_InitFlats (void)
{
  int		i;
  unsigned int	lump;
  unsigned int	loading;
  lumpinfo_t*	lump_ptr;
  flatinfo_t*   flatinfo_ptr;

  numflats = R_CountEntities ("F_START", "F_END", 0);

  // Create translation table for global animation.
  flatinfo = Z_Calloc ((numflats+1)*sizeof(*flatinfo), PU_STATIC, 0);

  i = 0;
  lump = 0;
  loading = 0;
  lump_ptr = lumpinfo;
  flatinfo_ptr = flatinfo;
  do
  {
    if (loading == 0)
    {
      if (strncasecmp (lump_ptr->name, "F_START", 8) == 0)
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, "F_END", 8) == 0)
	  || (lump_ptr->handle != (lump_ptr-1)->handle))
    {
      loading = 0;
    }
    else
    if ((lump_ptr->name[0]))
#ifdef MIN_SIZE_LUMP
     && (lump_ptr->size >= MIN_SIZE_LUMP)
#endif
    {
      if (!(i&63))
	putchar ('.');
      if (i < numflats)
      {
	memcpy (flatinfo_ptr->name, lump_ptr->name, sizeof (flatinfo_ptr->name));
	flatinfo_ptr->lump = lump;
	flatinfo_ptr->translation = i;
	flatinfo_ptr->index = -1;

        // We destroy the original entry in the lump list as some wads have the same
	// lump name for walls and flats. However, ncc1701.wad has walls in the floor
	// section so we cannot remove them.
	// Would have been better to detect this earlier but this would break save game
	// compatibility.
#ifdef REQ_FLAT_SIZE
        if (lump_ptr->size == REQ_FLAT_SIZE)
#endif
	  memset (lump_ptr -> name, 0, sizeof (lump_ptr -> name));

	flatinfo_ptr++;
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

  while (--i >= 0)
  {
	int j;

	j = W_LumpNameHash(flatinfo[i].name) % (unsigned int)numflats;
	flatinfo[i].next = flatinfo[j].index; // Prepend to chain
	flatinfo[j].index = i;
  }
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
static void R_InitSpriteLumps (void)
{
  unsigned int	i;
  unsigned int	lump;
  unsigned int	loading;
  patch_t	*patch;
  lumpinfo_t*	lump_ptr;

  numspritelumps = R_CountEntities ("S_START", "S_END", 1);

  spritewidth = Z_Calloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
  spriteoffset = Z_Calloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);
  spritetopoffset = Z_Calloc (numspritelumps*sizeof(fixed_t), PU_STATIC, NULL);

  i = 0;
  lump = 0;
  loading = 0;
  lump_ptr = lumpinfo;

  do
  {
    if (loading == 0)
    {
      if (strncasecmp (lump_ptr->name, "S_START", 8) == 0)
	loading = 1;
    }
    else if ((strncasecmp (lump_ptr->name, "S_END", 8) == 0)
	  || (lump_ptr->handle != (lump_ptr-1)->handle))
    {
      loading = 0;
    }
    else
    if ((lump_ptr->name[0]))
#ifdef MIN_SIZE_LUMP
     && (lump_ptr -> size >= MIN_SIZE_LUMP)
#endif
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

/* ------------------------------------------------------------------------------------------------ */
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
      colormaps = (byte *)( ((uintptr_t)colourmaps_a + 255)&~0xff);
    }
    W_ReadLump (lump,colormaps);
  }
}

/* ------------------------------------------------------------------------------------------------ */
#ifndef PADDED_STRUCTS
static void R_CheckStructs (void)
{
  if (sizeof (mappatch_t) != 10)
  {
    I_Error ("Your compiler has padded structures. Please recompile with -DPADDED_STRUCTS");
  }
}
#endif

/* ------------------------------------------------------------------------------------------------ */
//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
#ifndef PADDED_STRUCTS
    R_CheckStructs ();
#endif
    W_Find_Start_Ends ();
    printf ("\nInitFlats");		// Must be first.
    R_InitFlats ();
    printf ("\nInitTextures");
    R_InitTextures ();
    printf ("\nInitSprites");
    R_InitSpriteLumps ();
    //printf ("\nInitColormaps");
    R_InitColormaps (0);
    printf ("\n");
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_CheckFlatNumForName
// Returns -1 if name not found.
//

int R_CheckFlatNumForName (const char* name)
{
  int 	i;

  i = flatinfo[W_LumpNameHash(name) % (unsigned int)numflats].index;
  while (i >= 0 && strncasecmp(flatinfo[i].name, name, 8))
    i = flatinfo[i].next;

  return i;
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (const char* name)
{
  int 	i;

  i = R_CheckFlatNumForName (name);

  if (i == -1)
    i = skyflatnum;

  return i;
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName (const char *name)
{
    int i = 0; //NO_TEXTURE;

    if (*name != '-')
    {
	i = textures[W_LumpNameHash(name) % (unsigned int)numtextures].texture->index;
	while (i >= 0 && strncasecmp(textures[i].texture->name, name, 8))
	    i = textures[i].texture->next;
    }
    return i;
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//

int R_TextureNumForName (const char* name)
{
  int 	i;

  i = R_CheckTextureNumForName (name);
  if (i == -1)
    i = 1;

  return i;
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//

void R_PrecacheLevel (void)
{
  int 		f;
  int 		i;
  int 		j;
  int 		k;
  int 		s;
  int 		t;
  int 		lump;
  byte*		hitlist;
  texture_t*	texture;
  thinker_t*	th;
  spriteframe_t* sf;

  if (demoplayback)
      return;

  s = numtextures;
  if (numflats > s)   s = numflats;
  if (NUMSPRITES > s) s = NUMSPRITES;
  hitlist = malloc (s);
  if (hitlist == NULL)
    I_Error ("R_PrecacheLevel: out of memory\n");

  // Precache flats.
  memset (hitlist,0,numflats);

  for (i=0 ; i<numsectors ; i++)
  {
    f = sectors[i].floorpic;
    if ((unsigned)f < numflats)
      hitlist[f] = 1;
    f = sectors[i].ceilingpic;
    if ((unsigned)f < numflats)
      hitlist[f] = 1;
  }

  for (i=0 ; i<numflats ; i++)
  {
    if (hitlist[i])
    {
      lump = flatinfo [i].lump;
      W_CacheLumpNum(lump, PU_CACHE);
    }
  }

  // Precache textures.
  memset (hitlist,0, numtextures);

  for (i=0 ; i<numsides ; i++)
  {
    t = sides[i].toptexture;
    if ((unsigned)t < numtextures)
      hitlist[t] = 1;
    t = sides[i].midtexture;
    if ((unsigned)t < numtextures)
      hitlist[t] = 1;
    t = sides[i].bottomtexture;
    if ((unsigned)t < numtextures)
      hitlist[t] = 1;
  }

  // Sky texture is always present.
  // Note that F_SKY1 is the name used to
  //	indicate a sky floor/ceiling as a flat,
  //	while the sky texture is stored like
  //	a wall texture, with an episode dependent
  //	name.

  t = skytexture;
  if ((unsigned)t < numtextures)
    hitlist[t] = 1;

  for (i=0 ; i<numtextures ; i++)
  {
    if (hitlist[i])
    {
      texture = textures[i].texture;

      for (j=0 ; j<texture->patchcount ; j++)
      {
	lump = texture->patches[j].patch;
	W_CacheLumpNum(lump , PU_CACHE);
      }
    }
  }

  // Precache sprites.
  memset (hitlist,0, NUMSPRITES);

  for (th = thinker_head ; th != NULL ; th=th->next)
  {
    if (th->function.acp1 == (actionf_p1)P_MobjThinker)
    {
      s = ((mobj_t *)th)->sprite;
      if ((unsigned)s < NUMSPRITES)
	hitlist[s] = 1;
    }
  }

  for (i=0 ; i<NUMSPRITES ; i++)
  {
    if (hitlist[i])
    {
      for (j=0 ; j<sprites[i].numframes ; j++)
      {
	sf = &sprites[i].spriteframes[j];
	for (k=0 ; k<8 ; k++)
	{
	  lump = sf->lump[k];
	  if (lump != -1)
	    W_CacheLumpNum (lump, PU_CACHE);
	}
      }
    }
  }

  free (hitlist);
}

/* ------------------------------------------------------------------------------------------------ */
