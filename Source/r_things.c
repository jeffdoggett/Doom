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
//	Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: r_things.c,v 1.5 1997/02/03 16:47:56 b1 Exp $";
#endif

#include "includes.h"



#define MINZ				(FRACUNIT*4)
#define BASEYCENTER			100

//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);



typedef struct
{
    int 	x1;
    int 	x2;

    int 	column;
    int 	topclip;
    int 	bottomclip;

} maskdraw_t;



//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t 	pspritescale;
fixed_t 	pspriteiscale;

static lighttable_t**	spritelights;

// constant arrays
//  used for psprite clipping and initializing clipping
dshort_t	negonearray[MAXSCREENWIDTH];
dshort_t	screenheightarray[MAXSCREENWIDTH];


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*	sprites;
unsigned int	numsprites;

static spriteframe_t	sprtemp[29];
static int		maxframe;
static char*		spritename;

extern int	numspritelumps;

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
static void
R_InstallSpriteLump
( int		index,
  int		lump,
  unsigned	frame,
  unsigned	rotation,
  boolean	flipped )
{
    int 	r;

#if 0
    printf ("Adding sprite %d %d %d %d %d %s %s\n",
	index, lump, frame, rotation, flipped, spritename,
		    lumpinfo[lump].name);
#endif

    if (frame >= 29 || rotation > 8)
	I_Error("R_InstallSpriteLump: "
		"Bad frame characters in lump %i", lump);

    if ((int)frame > maxframe)
	maxframe = frame;

    if (rotation == 0)
    {
	// the lump should be used for all rotations
	if (sprtemp[frame].rotate == false)
	    I_Error ("R_InitSprites: Sprite %s frame %c has "
		     "multip rot=0 lump", spritename, 'A'+frame);

	if (sprtemp[frame].rotate == true)
	    I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
		     "and a rot=0 lump", spritename, 'A'+frame);

	sprtemp[frame].rotate = false;
	for (r=0 ; r<8 ; r++)
	{
	    sprtemp[frame].index[r] = index;
	    sprtemp[frame].lump[r] = lump;
	    sprtemp[frame].flip[r] = (byte)flipped;
	}
	return;
    }

    // the lump is only used for one rotation
    if (sprtemp[frame].rotate == false)
	I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
		 "and a rot=0 lump", spritename, 'A'+frame);

    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;
    if (sprtemp[frame].lump[rotation] != -1)
	I_Error ("R_InitSprites: Sprite %s : %c : %c "
		 "has two lumps mapped to it (%d and %d)",
		 spritename, 'A'+frame, '1'+rotation,
		 sprtemp[frame].lump[rotation], lump);

    sprtemp[frame].index[rotation] = index;
    sprtemp[frame].lump[rotation] = lump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}




//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
static void R_InitSpriteDefs (char** namelist)
{
    char**	check;
    int 	i;
    int 	index;
    int 	intname;
    int 	frame;
    int 	rotation;
    int 	rot;
    int 	lump;
    int 	done;

    // count the number of sprite names
    check = namelist;
    while (*check != NULL)
	check++;

    numsprites = check-namelist;

    if (!numsprites)
	return;

    sprites = Z_Malloc(numsprites *sizeof(*sprites), PU_STATIC, NULL);

    // scan all the lump names for each of the names,
    //	noting the highest frame letter.
    // Just compare 4 characters as ints
    for (i=0 ; i<numsprites ; i++)
    {
	// printf ("i = %d of %d\n", i, numsprites);
	spritename = namelist[i];
	memset (sprtemp,-1, sizeof(sprtemp));

	maxframe = -1;
	intname = *(int *)namelist[i];

	// scan the lumps,
	//  filling in the frames for whatever is found
	lump = numlumps;
	index = numspritelumps;
	do
	{
	  /* Search for 'end' */
	  lump--;
	  if (strncasecmp (lumpinfo[lump].name, "S_END", 8) == 0)
	  {
	    done = 0;
	    do
	    {
	      lump--;
	      if ((strncasecmp (lumpinfo[lump].name, "S_START", 8) == 0)
	       || (lumpinfo[lump].handle != lumpinfo[lump+1].handle))
	      {
		done = 1;
	      }
	      else
	      {
		if (lumpinfo[lump].name[0])
		  index--;
		if (*(int *)lumpinfo[lump].name == intname)
		{
		  frame = lumpinfo[lump].name[4] - 'A';
		  rotation = lumpinfo[lump].name[5] - '0';

		  if (frame >= 0)
		    R_InstallSpriteLump (index, lump, frame, rotation, false);

		  if (lumpinfo[lump].name[6])
		  {
		    frame = lumpinfo[lump].name[6] - 'A';
		    rotation = lumpinfo[lump].name[7] - '0';
		    R_InstallSpriteLump (index, lump, frame, rotation, true);
		  }
		}
	      }
	    } while (done == 0);
	  }
	} while (lump);


	// check the frames that were found for completeness
	if (maxframe == -1)
	{
	    sprites[i].numframes = 0;
	    continue;
	}

	maxframe++;

	for (frame = 0 ; frame < maxframe ; frame++)
	{
	    spriteframe_t * sprptr;

	    sprptr = &sprtemp[frame];
	    switch ((int)sprptr->rotate)
	    {
	      case -1:
#if 0
		if (M_CheckParm ("-showunknown"))
		{
		  // no rotations were found for that frame at all
		  printf ("R_InitSprites: No patches found for %s frame %c\n",
				namelist[i], frame+'A');
		}
#endif
		break;

	      case 0:
		// only the first rotation is needed
		break;

	      case 1:
		// must have all 8 frames
		for (rotation=0 ; rotation<8 ; rotation++)
		{
		  if (sprptr->lump[rotation] == -1)
		  {
		    if (M_CheckParm ("-showunknown"))
		    {
		      printf ("R_InitSprites: Sprite %s frame %c is missing rotation %u\n",
				namelist[i], frame+'A', rotation);
		    }

		    switch (rotation)
		    {
		      case 1-1:
			if (sprptr->lump[5-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[5-1];
			  sprptr->flip[rotation] = !sprptr->flip[5-1];
			  sprptr->index[rotation] = sprptr->index[5-1];
			}
			break;

		      case 5-1:
			if (sprptr->lump[1-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[1-1];
			  sprptr->flip[rotation] = !sprptr->flip[1-1];
			  sprptr->index[rotation] = sprptr->index[1-1];
			}
			break;

		      case 3-1:
			if (sprptr->lump[7-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[7-1];
			  sprptr->flip[rotation] = !sprptr->flip[7-1];
			  sprptr->index[rotation] = sprptr->index[7-1];
			}
			break;

		      case 7-1:
			if (sprptr->lump[3-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[3-1];
			  sprptr->flip[rotation] = !sprptr->flip[3-1];
			  sprptr->index[rotation] = sprptr->index[3-1];
			}
			break;

		      case 4-1:
			if (sprptr->lump[6-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[6-1];
			  sprptr->flip[rotation] = !sprptr->flip[6-1];
			  sprptr->index[rotation] = sprptr->index[6-1];
			}
			break;

		      case 6-1:
			if (sprptr->lump[4-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[4-1];
			  sprptr->flip[rotation] = !sprptr->flip[4-1];
			  sprptr->index[rotation] = sprptr->index[4-1];
			}
			break;

		      case 2-1:
			if (sprptr->lump[8-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[8-1];
			  sprptr->flip[rotation] = !sprptr->flip[8-1];
			  sprptr->index[rotation] = sprptr->index[8-1];
			}
			break;

		      case 8-1:
			if (sprptr->lump[2-1] != -1)
			{
			  sprptr->lump[rotation] = sprptr->lump[2-1];
			  sprptr->flip[rotation] = !sprptr->flip[2-1];
			  sprptr->index[rotation] = sprptr->index[2-1];
			}
			break;
		    }

		    if (sprptr->lump[rotation] == -1)
		    {
		      if (rotation)
		      {
			sprptr->lump[rotation] = sprptr->lump[rotation-1];
			sprptr->index[rotation] = sprptr->index[rotation-1];
		      }
		      else
		      {
			done = 0;
			for (rot=rotation ; rot<8 ; rot++)
			{
			  if (sprptr->lump[rot] != -1)
			  {
			    sprptr->lump[rotation] = sprptr->lump[rot];
			    sprptr->index[rotation] = sprptr->index[rot];
			    done = 1;
			    break;
			  }
			}
			if (done == 0)
			{
			  sprptr->rotate = false;
			}
		      }
		    }
		  }
		}
		break;
	    }
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[i].numframes = maxframe;
	sprites[i].spriteframes =
	    Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (sprites[i].spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));
    }

}




//
// GAME FUNCTIONS
//
//#define MAXVISSPRITES  	1024
static vissprite_t*	vissprites;
static vissprite_t**	vissprites_xref;
unsigned int		max_vissprites;
static unsigned int	num_vissprite;
static unsigned int	qty_vissprites;
static boolean		showvisstats;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (char** namelist)
{
    int 	i;

    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	negonearray[i] = -1;
    }

    R_InitSpriteDefs (namelist);

    num_vissprite = 0;
    qty_vissprites = 128;			// Start low.
    if (((vissprites = malloc (qty_vissprites * sizeof (vissprite_t))) == NULL)
     || ((vissprites_xref = malloc (qty_vissprites * sizeof (vissprite_t*))) == NULL))
      I_Error ("Failed to claim memory for vissprites\n");
    showvisstats = (boolean) M_CheckParm ("-showvissprites");
}



static int R_IncreaseVissprites (void)
{
  vissprite_t*	new_vissprites;
  vissprite_t**	new_vissprites_xref;

  if ((max_vissprites)
   && (qty_vissprites >= max_vissprites))
    return (0);

  new_vissprites = realloc (vissprites, (qty_vissprites+128) * sizeof (vissprite_t));
  if (new_vissprites == NULL)
    return (0);

  vissprites = new_vissprites;

  new_vissprites_xref = realloc (vissprites_xref, (qty_vissprites+128) * sizeof (vissprite_t*));
  if (new_vissprites_xref == NULL)
    return (0);

  vissprites_xref = new_vissprites_xref;

  qty_vissprites += 128;
  // printf ("qty_vissprites = %u\n", qty_vissprites);
  return (1);
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (void)
{
  if (num_vissprite >= qty_vissprites)		// Did we overflow last time?
   R_IncreaseVissprites ();

  num_vissprite = 0;
}


//
// R_NewVisSprite
//
static vissprite_t* R_NewVisSprite (fixed_t distance)
{
  int pos;
  unsigned int pos2;
  unsigned int step;
  unsigned int count;
  vissprite_t* rc;
  vissprite_t* vis;

  switch (num_vissprite)
  {
    case 0:			// 1st one?
      rc = &vissprites [0];
      vissprites_xref [0] = rc;
      num_vissprite = 1;
      return (rc);

    case 1:
      vis = &vissprites [0];
      rc = &vissprites [1];
      if (distance > vis->distance)
      {
	vissprites_xref [0] = rc;
	vissprites_xref [1] = vis;
      }
      else
      {
	vissprites_xref [1] = rc;
      }
      num_vissprite = 2;
      return (rc);
  }

  /* Do a binary search */
  /* Possibly overwrite a visplane that is furthest away. */
  /* Note: distance is actually the scaling factor, */
  /* i.e. bigger = draw bigger, hence nearer. */

  pos = (num_vissprite+1) >> 1;
  step = (pos+1) >> 1;
  count = (pos << 1);
  do
  {
    fixed_t	    d1;
    fixed_t	    d2;

    vis = vissprites_xref [pos];
    d1 = MAXINT;
    d2 = vis->distance;

    // printf ("pos = %u, step = %u, count = %u (%X %X)\n", pos, step, count, distance, d2);

    if (distance >= d2)
    {
      if (pos == 0)
	break;

      vis = vissprites_xref [pos-1];
      d1 = vis->distance;

      if (distance <= d1)
	break;
    }

    if (distance > d1)
    {
      pos -= step;
      if (pos < 0)
	pos = 0;
    }
    else
    {
      pos += step;
      if (pos >= num_vissprite)
	pos = num_vissprite-1;
    }

    step = (step+1) >> 1;
    count >>= 1;

    if (count == 0)
    {
      pos = num_vissprite;
      break;
    }
  } while (1);


  if (num_vissprite >= qty_vissprites)
  {
    if (pos >= num_vissprite)
      return (NULL);

    rc = vissprites_xref [num_vissprite-1];
  }
  else
  {
    rc = &vissprites [num_vissprite];
    num_vissprite++;
  }

  // printf ("Inserting at pos %u\n", pos);
  pos2 = num_vissprite-1;
  do
  {
    vissprites_xref [pos2] = vissprites_xref [pos2-1];
    pos2--;
  } while (pos2 > pos);

  vissprites_xref [pos] = rc;
  return (rc);
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
dshort_t*	mfloorclip;
dshort_t*	mceilingclip;

fixed_t 	spryscale;
//fixed_t 	sprydistance;
fixed_t 	sprtopscreen;

void R_DrawMaskedColumn (column_t* column)
{
    int 	topscreen;
    int 	bottomscreen;
    fixed_t	basetexturemid;
    int		td;
    int		topdelta;
    int		lastlength;

    basetexturemid = dc_texturemid;
    topdelta = -1;
    lastlength = 0;

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
	// calculate unclipped screen coordinates
	//  for post
	topscreen = sprtopscreen + spryscale*topdelta;
	bottomscreen = topscreen + spryscale*lastlength;

	dc_yl = (topscreen+FRACUNIT-1)>>FRACBITS;
	dc_yh = (bottomscreen-1)>>FRACBITS;

	if (dc_yh >= mfloorclip[dc_x])
	    dc_yh = mfloorclip[dc_x]-1;
	if (dc_yl <= mceilingclip[dc_x])
	    dc_yl = mceilingclip[dc_x]+1;

	dc_ylim = ~0;

	if (dc_yl <= dc_yh)
	{
	    dc_source = (byte *)column + 3;
	    dc_texturemid = basetexturemid - (topdelta<<FRACBITS);
	    // dc_source = (byte *)column + 3 - topdelta;

	    // Drawn by either R_DrawColumn
	    //	or (SHADOW) R_DrawFuzzColumn.
	    colfunc ();
	}
	column = (column_t *)(	(byte *)column + lastlength + 4);
    }

    dc_texturemid = basetexturemid;

#ifdef RANGECHECK_REMOVED
    if (dc_yl < 0)
     //|| (dc_yh >= SCREENHEIGHT))
	I_Error ("R_DrawMaskedColumn: %i to %i", dc_yl, dc_yh);
#endif
}



//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
static void
R_DrawVisSprite
( vissprite_t*		vis,
  int			x1,
  int			x2 )
{
    column_t*		column;
    int 		texturecolumn;
    fixed_t		frac;
    patch_t*		patch;


    patch = W_CacheLumpNum (vis->patch, PU_CACHE);

    dc_colormap = vis->colormap;

    if (!dc_colormap)
    {
	// NULL colormap = shadow draw
	colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
	if (vis->mobjflags & MF_TRANSLUCENT)
	  colfunc = R_DrawTranslatedTranslucentColumn;
	else
	  colfunc = R_DrawTranslatedColumn;
	dc_translation = translationtables - 256 +
	    ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
    else if (vis->mobjflags & MF_TRANSLUCENT)
    {
	colfunc = R_DrawTranslucentColumn;
    }

    dc_iscale = abs(vis->xiscale)>>detailshift;
    dc_texturemid = vis->texturemid1;
    frac = vis->startfrac;
    spryscale = vis->scale;
    //sprydistance = vis->distance;
    //sprtopscreen = centeryfrac - FixedMul(dc_texturemid,sprydistance);
    sprtopscreen = centeryfrac - FixedMul(vis->texturemid2,vis->distance);

    dc_x = vis -> x1;
    do
    {
	texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
	if ((dc_x < 0) || (dc_x >= SCREENWIDTH)
	 || (texturecolumn < 0 || texturecolumn >= SHORT(patch->width)))
	    printf ("R_DrawVisSprite: bad texturecolumn %u (%d,%d)\n",
		texturecolumn, dc_x,SHORT(patch->width));
	else
#endif
      {
	column = (column_t *) ((byte *)patch +
			       LONG(patch->columnofs[texturecolumn]));
	R_DrawMaskedColumn (column);
      }
      frac += vis->xiscale;
    } while (++dc_x <= vis->x2);

    colfunc = basecolfunc;
}



//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
static void R_ProjectSprite (mobj_t* thing)
{
  int x1;
  int x2;
  int lump;
  int index;
  fixed_t tx;
  fixed_t tz;
  angle_t ang;
  fixed_t gxt;
  fixed_t gyt;
  fixed_t tr_x;
  fixed_t tr_y;
  fixed_t xscale;
  fixed_t iscale;
  fixed_t distance;
  fixed_t spritescale;
  boolean flip;
  unsigned int rot;
  vissprite_t* vis;
  spritedef_t* sprdef;
  spriteframe_t* sprframe;

  // transform the origin point
  tr_x = thing->x - viewx;
  tr_y = thing->y - viewy;

  gxt = FixedMul(tr_x,viewcos);
  gyt = -FixedMul(tr_y,viewsin);

  tz = gxt-gyt;

  // thing is behind view plane?
  if (tz < MINZ)
    return;

  distance = FixedDiv(projection, tz);

  gxt = -FixedMul(tr_x,viewsin);
  gyt = FixedMul(tr_y,viewcos);
  tx = -(gyt+gxt);

  // too far off the side?
  if (abs(tx)>(tz<<2))
    return;

  // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
  if ((unsigned)thing->sprite >= numsprites)
    I_Error ("R_ProjectSprite: invalid sprite number %i/%i", thing->sprite,numsprites);
#endif
  sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
  if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
#if 1
    thing->frame &= ~(FF_FRAMEMASK);
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
      return;
#else
    I_Error ("R_ProjectSprite: invalid sprite frame %i : %i/%i ",
		thing->sprite, thing->frame, sprdef->numframes);
#endif
#endif
  sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

  if (sprframe->rotate)
  {
    // choose a different rotation based on player view
    ang = R_PointToAngle (thing->x, thing->y);
    rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
    if (rot >= (sizeof (sprframe->index) / sizeof (sprframe->index[0])))
      rot = 0;
    index = sprframe->index[rot];
    lump = sprframe->lump[rot];
    flip = (boolean)sprframe->flip[rot];
  }
  else
  {
    // use single rotation for all views
    index = sprframe->index[0];
    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];
  }

  spritescale = thing -> info -> scale;

  // calculate edges of the shape
  //tx -= spriteoffset[index];
  tx -= FixedMul (flip ? (spritewidth[index] - spriteoffset [index]) : spriteoffset[index], spritescale);
  x1 = (centerxfrac + FixedMul (tx,distance) ) >>FRACBITS;

  // off the right side?
  if (x1 >= viewwidth)
    return;

  tx += FixedMul (spritewidth[index], spritescale);
  x2 = ((centerxfrac + FixedMul (tx,distance) ) >>FRACBITS) - 1;

  // off the left side?
  if ((x2 < 0) || (x2 < x1))
    return;

  // store information in a vissprite
  vis = R_NewVisSprite (distance << detailshift);
  if (vis == NULL)
    return;

  xscale = FixedMul (distance, spritescale);

  vis->mobjflags = thing->flags;
  vis->distance = distance<<detailshift;
  vis->scale = xscale<<detailshift;
  vis->gx = thing->x;
  vis->gy = thing->y;
  vis->gz = thing->z;
  vis->gzt = thing->z + FixedMul (spritetopoffset[index], spritescale);
  vis->texturemid1 = FixedDiv ((vis->gzt - viewz), spritescale);
  vis->texturemid2 = vis->gzt - viewz;
  vis->x1 = x1 < 0 ? 0 : x1;
  vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;

  iscale = FixedDiv (FRACUNIT, xscale);

  if (flip)
  {
    vis->startfrac = spritewidth[index]-1;
    vis->xiscale = -iscale;
  }
  else
  {
    vis->startfrac = 0;
    vis->xiscale = iscale;
  }

  if (vis->x1 > x1)
    vis->startfrac += vis->xiscale*(vis->x1-x1);

  vis->patch = lump;

  // get light level
  if (thing->flags & MF_SHADOW)
  {
    // shadow draw
    vis->colormap = NULL;
  }
  else if (fixedcolormap)
  {
    // fixed map
    vis->colormap = fixedcolormap;
  }
  else if (thing->frame & FF_FULLBRIGHT)
  {
    // full bright
    vis->colormap = colormaps;
  }
  else
  {
    // diminished light
    index = distance>>(lightScaleShift-detailshift);

    if (index >= MAXLIGHTSCALE)
	index = MAXLIGHTSCALE-1;

    vis->colormap = spritelights[index];
  }
}




//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting

void R_AddSprites(sector_t* sec, int lightlevel)
{
  mobj_t *thing;
  int    lightnum;

  // BSP is traversed by subsector.
  // A sector might have been split into several
  //  subsectors during BSP building.
  // Thus we check whether its already added.

  if (sec->validcount == validcount)
    return;

  // Well, now it will be done.
  sec->validcount = validcount;

  lightnum = (lightlevel >> LIGHTSEGSHIFT)+extralight;

  if (lightnum < 0)
    spritelights = scalelight[0];
  else if (lightnum >= LIGHTLEVELS)
    spritelights = scalelight[LIGHTLEVELS-1];
  else
    spritelights = scalelight[lightnum];

  // Handle all things in sector.

  for (thing = sec->thinglist; thing; thing = thing->snext)
    R_ProjectSprite(thing);
}


//
// R_DrawPSprite
//
static void R_DrawPSprite (pspdef_t* psp)
{
    fixed_t		tx;
    int 		x1;
    int 		x2;
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int 		index;
    int 		lump;
    boolean		flip;
    vissprite_t*	vis;
    vissprite_t 	avis;

    // decide which patch to use
#ifdef RANGECHECK
    if ( (unsigned)psp->state->sprite >= numsprites)
	I_Error ("R_DrawPSprite: invalid sprite number %i ",
		 psp->state->sprite);
#endif
    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
#if 1
	psp->state->frame &= ~FF_FRAMEMASK;
#else
	I_Error ("R_DrawPSprite: invalid sprite frame %i : %i/%i\n",
		 psp->state->sprite, psp->state->frame, sprdef->numframes);
#endif
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    index = sprframe->index[0];
    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];

    // calculate edges of the shape
    tx = psp->sx-160*FRACUNIT;

    tx -= spriteoffset[index];
    x1 = (centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

    // off the right side
    if (x1 >= viewwidth)
	return;

    tx += spritewidth[index];
    x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

    // off the left side?
    if ((x2 < 0) || (x2 < x1))
	return;

    // store information in a vissprite
    vis = &avis;
    vis->mobjflags = 0;
    vis->texturemid1 =
    vis->texturemid2 = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[index]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
    vis->scale =
    vis->distance = pspritescale<<detailshift;

    if (flip)
    {
	vis->xiscale = -pspriteiscale;
	vis->startfrac = spritewidth[index]-1;
    }
    else
    {
	vis->xiscale = pspriteiscale;
	vis->startfrac = 0;
    }

    if (vis->x1 > x1)
	vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    if (viewplayer->powers[pw_invisibility] > 4*32
	|| viewplayer->powers[pw_invisibility] & 8)
    {
	// shadow draw
	vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
	// fixed color
	vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = colormaps;
    }
    else
    {
	// local light
	vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
static void R_DrawPlayerSprites (void)
{
    int 	i;
    int 	lightnum;
    pspdef_t*	psp;

    // get light level
    lightnum = (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0)
	spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
	spritelights = scalelight[LIGHTLEVELS-1];
    else
	spritelights = scalelight[lightnum];

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    // add all active psprites
    for (i=0, psp=viewplayer->psprites;
	 i<NUMPSPRITES;
	 i++,psp++)
    {
	if (psp->state)
	    R_DrawPSprite (psp);
    }
}




//
// R_DrawSprite
//
static void R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*		ds;
    dshort_t		clipbot[MAXSCREENWIDTH];
    dshort_t		cliptop[MAXSCREENWIDTH];
    int 		x;
    int 		r1;
    int 		r2;
    fixed_t		scale;
    fixed_t		lowscale;

    // [RH] Quickly reject sprites with bad x ranges.
    if (spr->x1 > spr->x2)
      return;

    for (x = spr->x1 ; x<=spr->x2 ; x++)
	clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //	is the clip seg.
    //      for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

    for (ds=ds_p ; ds-- > drawsegs ; )  // new -- killough
    {
	// determine if the drawseg obscures the sprite
	if (ds->x1 > spr->x2
	    || ds->x2 < spr->x1
	    || (!ds->silhouette
		&& !ds->maskedtexturecol) )
	{
	    // does not cover sprite
	    continue;
	}

	r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
	r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

	if (ds->scale1 > ds->scale2)
	{
	    lowscale = ds->scale2;
	    scale = ds->scale1;
	}
	else
	{
	    lowscale = ds->scale1;
	    scale = ds->scale2;
	}

	if (scale < spr->distance
	    || ( lowscale < spr->distance
		 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
	{
	    // masked mid texture?
	    if (ds->maskedtexturecol)
		R_RenderMaskedSegRange (ds, r1, r2);
	    // seg is behind sprite
	    continue;
	}


	// clip this piece of the sprite
	// killough 3/27/98: optimized and made much shorter

	if (ds->silhouette&SIL_BOTTOM && spr->gz < ds->bsilheight) //bottom sil
	    for (x = r1; x <= r2; x++)
		if (clipbot[x] == -2)
		    clipbot[x] = ds->sprbottomclip[x];

	if (ds->silhouette&SIL_TOP && spr->gzt > ds->tsilheight)   // top sil
	    for (x = r1; x <= r2; x++)
		if (cliptop[x] == -2)
		    cliptop[x] = ds->sprtopclip[x];

    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	if (clipbot[x] == -2 || clipbot[x] > viewheight)
	    clipbot[x] = viewheight;

	if (cliptop[x] < 0)
	    cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked (void)
{
    unsigned int	pos;
    vissprite_t*	spr;
    drawseg_t*		ds;

    if ((pos = num_vissprite) != 0)
    {
      if (showvisstats)
      {
#ifdef __riscos
	extern void _kernel_oswrch (int);
	_kernel_oswrch (31);
	_kernel_oswrch (0);
	_kernel_oswrch (0);
#endif
	printf ("VisSprites = %u/%u\n", pos, qty_vissprites);
      }

      // draw all vissprites back to front
      do
      {
	pos--;
	spr = vissprites_xref [pos];
	R_DrawSprite (spr);
      } while (pos);
    }

    // render any remaining masked mid textures
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
	if (ds->maskedtexturecol)
	    R_RenderMaskedSegRange (ds, ds->x1, ds->x2);

    // draw the psprites on top of everything
    //	but does not draw on side views
    if (!viewangleoffset)
	R_DrawPlayerSprites ();
}



