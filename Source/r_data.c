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

//-----------------------------------------------------------------------------
// #define MIN_SIZE_LUMP	12
//-----------------------------------------------------------------------------

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
static unsigned int**	texturecolumnofs2;
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

/* ------------------------------------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------------------------------------ */
//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
static void R_GenerateComposite (int texnum)
{
    // [crispy] initialize composite background to black (index 0)
    byte	 *block = (byte *)Z_Calloc(texturecompositesize[texnum], PU_STATIC,
						(void **)&texturecomposite[texnum]);
    texture_t    *texture = textures[texnum];
    // Composite the columns together.
    texpatch_t   *patch = texture->patches;
    dshort_t	 *collump = texturecolumnlump[texnum];
    unsigned int *colofs = texturecolumnofs[texnum]; // killough 4/9/98: make 32-bit
    int		 i;
    // killough 4/9/98: marks to identify transparent regions in merged textures
    byte	 *source;
    byte	 *marks = (byte *)calloc(texture->width, texture->height);

    if (marks == NULL)
      I_Error ("R_GenerateComposite: Failed to allocate memory\n");

    for (i = texture->patchcount; --i >= 0; patch++)
    {
	patch_t   *realpatch = (patch_t *)W_CacheLumpNum(patch->patch, PU_CACHE);
	int       x, x1 = patch->originx, x2 = x1 + SHORT(realpatch->width);
	const int *cofs = realpatch->columnofs - x1;

	if (x1 < 0)
	    x1 = 0;
	if (x2 > texture->width)
	    x2 = texture->width;
	for (x = x1; x < x2 ; x++)
	    // [crispy] generate composites for single-patched textures as well
	    // if (collump[x] == -1)		// Column has multiple patches?
		// killough 1/25/98, 4/9/98: Fix medusa bug.
		R_DrawColumnInCache((column_t *)((byte *)realpatch + LONG(cofs[x])),
				    block + colofs[x], patch->originy,
				    texture->height, marks + x * texture->height);
    }

    // killough 4/9/98: Next, convert multipatched columns into true columns,
    // to fix Medusa bug while still allowing for transparent regions.

    source = (byte *)malloc(texture->height);	// temporary column
    if (source == NULL)
      I_Error ("R_GenerateComposite: Failed to allocate memory\n");

    for (i = 0; i < texture->width; i++)
	if (collump[i] == -1)			// process only multipatched columns
	{
	    column_t *col = (column_t *)(block + colofs[i] - 3);	// cached column
	    const byte *mark = marks + i * texture->height;
	    int j = 0;


	    // save column in temporary so we can shuffle it around
	    memcpy(source, (byte *)col + 3, texture->height);

	    while (1)		// reconstruct the column by scanning transparency marks
	    {
		unsigned int len;			// killough 12/98

		while (j < texture->height && !mark[j])	// skip transparent cells
		    j++;

		if (j >= texture->height)		// if at end of column
		{
		    col->topdelta = -1;			// end-of-column marker
		    break;
		}

		col->topdelta = j;			// starting offset of post

		// killough 12/98:
		// Use 32-bit len counter, to support tall 1s multipatched textures

		for (len = 0; j < texture->height && mark[j]; j++)
		    len++;				// count opaque cells

		col->length = len;	// killough 12/98: intentionally truncate length

		// copy opaque cells from the temporary back into the column
		memcpy((byte *)col + 3, source + col->topdelta, len);
		col = (column_t *)((byte *)col + len + 4); // next post
	    }
	}
    free(source);	// free temporary column
    free(marks);	// free transparency marks

    // Now that the texture has been built in column cache,
    // it is purgable from zone memory.

    Z_ChangeTag(block, PU_CACHE);
}

/* ------------------------------------------------------------------------------------------------ */
//
// R_GenerateLookup
//
static void R_GenerateLookup (int texnum)
{
  texture_t	*texture = textures[texnum];
  byte		*patchcount = Z_Calloc(texture->width, PU_STATIC, (void**)&patchcount);
  byte		*postcount = Z_Calloc(texture->width, PU_STATIC, (void**)&postcount);
  texpatch_t	*patch;
  int		x;
  int		i;
  dshort_t	*collump;
  unsigned int	*colofs;	// killough 4/9/98: make 32-bit
  unsigned int	*colofs2;	// [crispy] original column offsets
  int		csize = 0;	// killough 10/98

  texturecomposite[texnum] = 0;// Composited texture not created yet.

  texturecompositesize[texnum] = 0;
  collump = texturecolumnlump[texnum];
  colofs = texturecolumnofs[texnum];
  colofs2 = texturecolumnofs2[texnum]; // [crispy] original column offsets

  // Now count the number of columns
  //  that are covered by more than one patch.
  // Fill in the lump / offset, so columns
  //  with only a single patch are all done.

  for (i = 0, patch = texture->patches; i < texture->patchcount; ++i, ++patch)
  {
      patch_t	*realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
      int	x1 = patch->originx;
      int	x2 = MIN(x1 + SHORT(realpatch->width), texture->width);

      x = ((x1 < 0) ? 0 : x1);

      for (; x < x2; x++)
      {
	  patchcount[x]++;
	  collump[x] = patch->patch;
	  colofs[x] = colofs2[x] = LONG(realpatch->columnofs[x - x1]) + 3;
      }
  }

  // killough 4/9/98: keep a count of the number of posts in column,
  // to fix Medusa bug while allowing for transparent multipatches.
  //
  // killough 12/98:
  // Post counts are only necessary if column is multipatched,
  // so skip counting posts if column comes from a single patch.
  // This allows arbitrarily tall textures for 1s walls.
  //
  // If texture is >= 256 tall, assume it's 1s, and hence it has
  // only one post per column. This avoids crashes while allowing
  // for arbitrarily tall multipatched 1s textures.
  if (texture->patchcount > 1 && texture->height < 256)
  {
      // killough 12/98: Warn about a common column construction bug
      unsigned int limit = texture->height * 3 + 3;	// absolute column size limit

      for (i = texture->patchcount, patch = texture->patches; --i >= 0;)
      {
	  const patch_t *realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
	  int		x;
	  int		x1 = patch++->originx;
	  int		x2 = MIN(x1 + SHORT(realpatch->width), texture->width);
	  const int	*cofs = realpatch->columnofs - x1;

	  if (x1 < 0)
	    x1 = 0;

	  for (x = x1; x < x2; ++x)
	      if (patchcount[x] > 1)	// Only multipatched columns
	      {
		  const column_t *col = (column_t *)((byte *)realpatch + LONG(cofs[x]));
		  const byte	 *base = (const byte *)col;

		  // count posts
		  for (; col->topdelta != 0xFF; ++postcount[x])
		      if ((unsigned int)((byte *)col - base) <= limit)
			  col = (column_t *)((byte *)col + col->length + 4);
		      else
			  break;
	      }
      }
  }

  // Now count the number of columns
  //  that are covered by more than one patch.
  // Fill in the lump / offset, so columns
  //  with only a single patch are all done.
  for (x = 0; x < texture->width; ++x)
  {
      if (patchcount[x] > 1)
	  // Use the cached block.
	  // [crispy] moved up here, the rest in this loop
	  // applies to single-patched textures as well
	  collump[x] = -1;

      // killough 1/25/98, 4/9/98:
      //
      // Fix Medusa bug, by adding room for column header
      // and trailer bytes for each post in merged column.
      // For now, just allocate conservatively 4 bytes
      // per post per patch per column, since we don't
      // yet know how many posts the merged column will
      // require, and it's bounded above by this limit.
      colofs[x] = csize + 3;		// three header bytes in a column
					// killough 12/98: add room for one extra post
      csize += 4 * postcount[x] + 5;	// 1 stop byte plus 4 bytes per post

      csize += texture->height;		// height bytes of texture data
  }

  texturecompositesize[texnum] = csize;

  Z_Free(patchcount);
  Z_Free(postcount);
}

/* ------------------------------------------------------------------------------------------------ */

byte *R_GetColumn (int tex, int col, boolean opaque)
{
  int	lump;
  int	ofs;
  byte *p;
  byte *patch;

  column_t * column;

  if (((unsigned int) tex) >= numtextures)  tex = 0;

  col &= texturewidthmask[tex];
  lump = texturecolumnlump[tex][col];

  // [crispy] single-patched mid-textures on two-sided walls
  if (lump > 0)
  {
    patch = W_CacheLumpNum (lump, PU_CACHE);
    ofs = texturecolumnofs2[tex][col];
    p = patch + ofs;

    if (opaque == false)
      return (p);

    /* If the column is already a single post, then we */
    /* don't need to generate a composite. */

    column = (column_t*) (p-3);
    if (column->length == (textureheight[tex] >> FRACBITS))
      return (p);
  }

  if (!texturecomposite[tex])
    R_GenerateComposite(tex);

  ofs = texturecolumnofs[tex][col];
  return (texturecomposite[tex] + ofs);
}

/* ------------------------------------------------------------------------------------------------ */

static const char mask_ffffffff[] = { 255,255,255,255,255,255,255,255 };

static unsigned int R_merge_pnames (uintptr_t * patchlookup)
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
	  patchlookup[pl_pos++] = (uintptr_t)lumpinfo[lump].handle;
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
			unsigned int maxoff, unsigned int nummappatches, uintptr_t *patchlookup)
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

    if ((((byte *) directory) >= ((byte *)maptex+maxoff))
     || ((offset=LONG(*directory)) >= maxoff))
    {
#ifdef NORMALUNIX
      printf ("R_InitTextures: bad texture directory %X/%X %d/%d\n",
		directory,((byte *)maptex+maxoff), offset, maxoff);
#endif
      break;
    }

    directory++;
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
      texturecolumnofs2[pos] = texturecolumnofs2[p];
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
    texturecolumnlump[pos] = Z_Calloc (texture->width*sizeof(**texturecolumnlump), PU_STATIC,0);
    texturecolumnofs[pos]  = Z_Calloc (texture->width*sizeof(**texturecolumnofs), PU_STATIC,0);
    texturecolumnofs2[pos]  = Z_Calloc (texture->width*sizeof(**texturecolumnofs2), PU_STATIC,0);

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

static uintptr_t * R_find_matching_pnames (FILE * handle, uintptr_t *patchlookup)
{
  uintptr_t * pl;

  pl = patchlookup;

  while (pl [0])
  {
    if (pl [1] == (uintptr_t)handle)
      return (pl);

    pl = pl + pl [0] + 2;
  }

  return (patchlookup);
}

/* ------------------------------------------------------------------------------------------------ */

static unsigned int R_merge_textures (char * name, unsigned int total, uintptr_t *patchlookup)
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
static void R_InitTextures (void)
{
  unsigned int i,j;
  uintptr_t*	patchlookup;
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

  textures = Z_Calloc (numtextures * sizeof(*textures), PU_STATIC, NULL);
  texturecolumnlump = Z_Calloc (numtextures * sizeof(*texturecolumnlump), PU_STATIC, NULL);
  texturecolumnofs = Z_Calloc (numtextures * sizeof(*texturecolumnofs), PU_STATIC, NULL);
  texturecolumnofs2 = Z_Calloc (numtextures * sizeof(*texturecolumnofs2), PU_STATIC, NULL);
  texturecomposite = Z_Calloc (numtextures * sizeof(*texturecomposite), PU_STATIC, NULL);
  texturecompositesize = Z_Calloc (numtextures * sizeof(*texturecompositesize), PU_STATIC, NULL);
  texturewidthmask = Z_Calloc (numtextures * sizeof(*texturewidthmask), PU_STATIC, NULL);
  textureheight = Z_Calloc (numtextures * sizeof(*textureheight), PU_STATIC, NULL);


  j = R_merge_textures ("TEXTURE1", 0, patchlookup);
  j = R_merge_textures ("TEXTURE2", j, patchlookup);
  free (patchlookup);
  // printf ("numtextures = %u -> %u\n", numtextures, j);

  if (j > numtextures)
    I_Error ("R_InitTextures: Too many textures!");

  if (j < numtextures)
  {
    numtextures = j;
    textures = Z_Realloc (textures,numtextures * sizeof(*textures), PU_STATIC, NULL);
    texturecolumnlump = Z_Realloc (texturecolumnlump,numtextures * sizeof(*texturecolumnlump), PU_STATIC, NULL);
    texturecolumnofs = Z_Realloc (texturecolumnofs,numtextures * sizeof(*texturecolumnofs), PU_STATIC, NULL);
    texturecolumnofs2 = Z_Realloc (texturecolumnofs2,numtextures * sizeof(*texturecolumnofs2), PU_STATIC, NULL);
    texturecomposite = Z_Realloc (texturecomposite,numtextures * sizeof(*texturecomposite), PU_STATIC, NULL);
    texturecompositesize = Z_Realloc (texturecompositesize,numtextures * sizeof(*texturecompositesize), PU_STATIC, NULL);
    texturewidthmask = Z_Realloc (texturewidthmask,numtextures * sizeof(*texturewidthmask), PU_STATIC, NULL);
    textureheight = Z_Realloc (textureheight,numtextures * sizeof(*textureheight), PU_STATIC, NULL);
  }

  // Create translation table for global animation.
  texturetranslation = Z_Malloc ((numtextures+1)*sizeof(*texturetranslation), PU_STATIC, NULL);

  // Precalculate whatever possible.
  for (i=0 ; i<numtextures ; i++)
  {
    if ((i & 127) == 0)
      putchar ('.');
    texturetranslation[i] = i;
    R_GenerateLookup (i);
#if 1
    /* Experimental code... JAD */
    for (j=0; j<textures[i]->width; j++)
      if (texturecolumnlump[i][j] == -1)
      {
	R_GenerateComposite (i);
	break;
      }
#endif
  }
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
      return (1);
    return (0);
  }

  rc = 0;

  if (R_NameCompare (4,4))
    rc |= 1;

  if ((name1 [6])
   && (R_NameCompare (6,4)))
    rc |= 2;

  if (name2 [6])
  {
    if (R_NameCompare (4,6))
      rc |= 4;

    if ((name1 [6])
     && (R_NameCompare (6,6)))
      rc |= 8;
  }

  return (rc);
}

/* ------------------------------------------------------------------------------------------------ */

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
	if (pos)
	{
	  // printf ("Lump %u Result = %X (%s)(%s)\n", lump, pos, lump_ptr -> name, name);
	  switch (pos)
          {
            case 1:
	    case 4:
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
	    case 8:
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
	    case 5:
	    case 6:
	    case 9:
	      lump_ptr -> name [0] = 0;
	      break;

	    default:
	      printf ("Uncoded result = %X (%s)(%s)\n", pos, lump_ptr -> name, name);
          }
          // printf ("To '%s'\n", lump_ptr -> name);
        }
      }
    }
    lump_ptr++;
    lump++;
  }
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

static int R_CountEntities (char * start, char * end, int doing_sprites)
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
       || (W_CheckNumForNameLinear (lump_ptr->name) != lump)) /* If there's another one at a higher pos */
      {
	valid = false;
      }
      else if (doing_sprites)
      {
        // printf ("R_RemoveDuplicateSprites (%s,%u)\n", lump_ptr->name, lump);
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
  unsigned int	i;
  unsigned int	lump;
  unsigned int	loading;
  lumpinfo_t*	lump_ptr;

  numflats = R_CountEntities ("F_START", "F_END", 0);

  // Create translation table for global animation.
  flattranslation = Z_Calloc ((numflats+1)*sizeof(*flattranslation), PU_STATIC, 0);

  // Create translation table for flat lumps.
  flatlumps = Z_Calloc ((numflats+1)*sizeof(*flatlumps), PU_STATIC, 0);

  i = 0;
  lump = 0;
  loading = 0;
  lump_ptr = lumpinfo;
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
    printf ("\nInitTextures");
    R_InitTextures ();
    printf ("\nInitFlats");
    R_InitFlats ();
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

/* ------------------------------------------------------------------------------------------------ */
