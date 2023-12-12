/*
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
*/

#include "includes.h"

//
// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
//

// Re-engineered patch support
static rpatch_t	 *patches = 0;
static rpatch_t	 *texture_composites = 0;

void R_InitPatches(void)
{
    if (!patches)
	patches = calloc(numlumps, sizeof(rpatch_t));

    if (!texture_composites)
	texture_composites = calloc(numtextures, sizeof(rpatch_t));
}

typedef struct
{
    dushort_t patches;
    dushort_t posts;
    dushort_t posts_used;
}
count_t;

static void switchPosts(rpost_t *post1, rpost_t *post2)
{
    rpost_t dummy;

    dummy.topdelta = post1->topdelta;
    dummy.length = post1->length;
    post1->topdelta = post2->topdelta;
    post1->length = post2->length;
    post2->topdelta = dummy.topdelta;
    post2->length = dummy.length;
}

static void removePostFromColumn(rcolumn_t *column, int post)
{
    if (post < column->numPosts)
    {
	int i;

	for (i = post; i < column->numPosts - 1; ++i)
	{
	    rpost_t *post1 = &column->posts[i];
	    rpost_t *post2 = &column->posts[i + 1];

	    post1->topdelta = post2->topdelta;
	    post1->length = post2->length;
	}
    }
    column->numPosts--;
}

static void createTextureCompositePatch(int id)
{
    rpatch_t	*composite_patch = &texture_composites[id];
    texture_t	*texture = textures[id].texture;
    texpatch_t	*texpatch;
    int		patchNum;
    const patch_t *oldPatch;
    const column_t *oldColumn;
    int		i, x, y;
    int		oy, count;
    int		pixelDataSize;
    int		columnsDataSize;
    int		postsDataSize;
    int		dataSize;
    int		numPostsTotal;
    const unsigned char *oldColumnPixelData;
    int		numPostsUsedSoFar;
    count_t	*countsInColumn;

    composite_patch->width = texture->width;
    composite_patch->height = texture->height;

    // work out how much memory we need to allocate for this patch's data
    pixelDataSize = (composite_patch->width * composite_patch->height + 4) & ~3;
    columnsDataSize = sizeof(rcolumn_t) * composite_patch->width;

    // count the number of posts in each column
    countsInColumn = (count_t *)calloc(sizeof(count_t), composite_patch->width);
    numPostsTotal = 0;

    for (i = 0; i < texture->patchcount; ++i)
    {
	texpatch = &texture->patches[i];
	patchNum = texpatch->patch;
	oldPatch = (const patch_t *)W_CacheLumpNum(patchNum, PU_STATIC);

	for (x = 0; x < SHORT(oldPatch->width); ++x)
	{
	    int tx = texpatch->originx + x;

	    if (tx < 0)
		continue;
	    if (tx >= composite_patch->width)
		break;

	    countsInColumn[tx].patches++;

	    oldColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->columnofs[x]));
	    while (oldColumn->topdelta != 0xFF)
	    {
		countsInColumn[tx].posts++;
		numPostsTotal++;
		oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
	    }
	}

	W_ReleaseLumpNum(patchNum);
    }

    postsDataSize = numPostsTotal * sizeof(rpost_t);

    // allocate our data chunk
    dataSize = pixelDataSize + columnsDataSize + postsDataSize;
    composite_patch->data = (unsigned char *)Z_Malloc(dataSize, PU_STATIC,
	(void **)&composite_patch->data);
    memset(composite_patch->data, 0, dataSize);

    // set out pixel, column, and post pointers into our data array
    composite_patch->pixels = composite_patch->data;
    composite_patch->columns = (rcolumn_t*)((unsigned char*)composite_patch->pixels
	+ pixelDataSize);
    composite_patch->posts = (rpost_t*)((unsigned char*)composite_patch->columns
	+ columnsDataSize);

    // sanity check that we've got all the memory allocated we need
    assert((((byte *)composite_patch->posts + numPostsTotal * sizeof(rpost_t))
	- (byte *)composite_patch->data) == dataSize);

    memset(composite_patch->pixels, 0xFF, (composite_patch->width * composite_patch->height));

    numPostsUsedSoFar = 0;

    for (x = 0; x < texture->width; ++x)
    {
	// setup the column's data
	composite_patch->columns[x].pixels = composite_patch->pixels
	    + (x * composite_patch->height);
	composite_patch->columns[x].numPosts = countsInColumn[x].posts;
	composite_patch->columns[x].posts = composite_patch->posts + numPostsUsedSoFar;
	numPostsUsedSoFar += countsInColumn[x].posts;
    }

    // fill in the pixels, posts, and columns
    for (i = 0; i < texture->patchcount; ++i)
    {
	texpatch = &texture->patches[i];
	patchNum = texpatch->patch;
	oldPatch = (const patch_t *)W_CacheLumpNum(patchNum, PU_STATIC);

	for (x = 0; x < SHORT(oldPatch->width); ++x)
	{
	    int	 top = -1;
	    int	 tx = texpatch->originx + x;

	    if (tx < 0)
		continue;
	    if (tx >= composite_patch->width)
		break;

	    oldColumn = (const column_t *)((const byte *)oldPatch + LONG(oldPatch->columnofs[x]));

	    while (oldColumn->topdelta != 0xFF)
	    {
		rpost_t *post = &composite_patch->columns[tx].posts[countsInColumn[tx].posts_used];

		// e6y: support for DeePsea's true tall patches
		if (oldColumn->topdelta <= top)
		    top += oldColumn->topdelta;
		else
		    top = oldColumn->topdelta;

		oldColumnPixelData = (const byte *)oldColumn + 3;
		oy = texpatch->originy;
		count = oldColumn->length;

		// set up the post's data
		post->topdelta = top + oy;
		post->length = count;
		if ((post->topdelta + post->length) > composite_patch->height)
		{
		    if (post->topdelta > composite_patch->height)
			post->length = 0;
		    else
			post->length = composite_patch->height - post->topdelta;
		}
		if (post->topdelta < 0)
		{
		    if ((post->topdelta + post->length) <= 0)
			post->length = 0;
		    else
			post->length -= post->topdelta;
		    post->topdelta = 0;
		}

		// fill in the post's pixels
		for (y = 0; y < count; ++y)
		{
		    int ty = oy + top + y;

		    if (ty < 0)
			continue;
		    if (ty >= composite_patch->height)
			break;
		    composite_patch->pixels[tx * composite_patch->height + ty]
			= oldColumnPixelData[y];
		}

		oldColumn = (const column_t *)((const byte *)oldColumn + oldColumn->length + 4);
		countsInColumn[tx].posts_used++;
		assert(countsInColumn[tx].posts_used <= countsInColumn[tx].posts);
	    }
	}

	W_ReleaseLumpNum(patchNum);
    }

    for (x = 0; x < texture->width; ++x)
    {
	rcolumn_t *column;

	if (countsInColumn[x].patches <= 1)
	    continue;

	// cleanup posts on multipatch columns
	column = &composite_patch->columns[x];

	i = 0;
	while (i < column->numPosts - 1)
	{
	    rpost_t *post1 = &column->posts[i];
	    rpost_t *post2 = &column->posts[i + 1];

	    if (post2->topdelta - post1->topdelta < 0)
		switchPosts(post1, post2);

	    if (post1->topdelta + post1->length >= post2->topdelta)
	    {
		int length = (post1->length + post2->length) - ((post1->topdelta
		    + post1->length) - post2->topdelta);

		if (post1->length < length)
		    post1->length = length;
		removePostFromColumn(column, i + 1);
		i = 0;
		continue;
	    }
	    i++;
	}
    }

    {
	const rcolumn_t *column;
	const rcolumn_t *prevColumn;

	// copy the patch image down and to the right where there are
	// holes to eliminate the black halo from bilinear filtering
	for (x = 0; x < composite_patch->width; ++x)
	{
	    column = R_GetPatchColumnClamped(composite_patch, x);
	    prevColumn = R_GetPatchColumnClamped(composite_patch, x - 1);

	    if (column->pixels[0] == 0xFF)
	    {
		// force the first pixel (which is a hole), to use
		// the color from the next solid spot in the column
		for (y = 0; y < composite_patch->height; ++y)
		{
		    if (column->pixels[y] != 0xFF)
		    {
			column->pixels[0] = column->pixels[y];
			break;
		    }
		}
	    }

	    // copy from above or to the left
	    for (y = 1; y < composite_patch->height; ++y)
	    {
		if (column->pixels[y] != 0xFF)
		    continue;

		if (x && prevColumn->pixels[y - 1] != 0xFF)
		    column->pixels[y] = prevColumn->pixels[y];  // copy the color from the left
		else
		    column->pixels[y] = column->pixels[y - 1];  // copy the color from above
	    }
	}
    }

    free(countsInColumn);
}

rpatch_t *R_CacheTextureCompositePatchNum(int id)
{
    if (!texture_composites)
	I_Error("R_CacheTextureCompositePatchNum: Composite patches not initialized");

    if (!texture_composites[id].data)
	createTextureCompositePatch(id);

    // cph - if wasn't locked but now is, tell z_zone to hold it
    if (!texture_composites[id].locks)
	Z_ChangeTag(texture_composites[id].data, PU_STATIC);
    texture_composites[id].locks++;

    return &texture_composites[id];
}

void R_UnlockTextureCompositePatchNum(int id)
{
    if (!--texture_composites[id].locks)
	Z_ChangeTag(texture_composites[id].data, PU_CACHE);
}

rcolumn_t *R_GetPatchColumnWrapped(rpatch_t *patch, int columnIndex)
{
    while (columnIndex < 0)
	columnIndex += patch->width;
    columnIndex %= patch->width;
    return &patch->columns[columnIndex];
}

rcolumn_t *R_GetPatchColumnClamped(rpatch_t *patch, int columnIndex)
{
  int lim;

  if (columnIndex < 0)
  {
    columnIndex = 0;
  }
  else
  {
    lim = patch->width - 1;
    if (columnIndex > lim)
      columnIndex = lim;
  }
  return (&patch->columns [columnIndex]);
}

