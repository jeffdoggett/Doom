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
#define MAXZ				(FRACUNIT*8192)
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
dshort_t*	 negonearray;
dshort_t*	 screenheightarray;
static dshort_t* clipbot;
static dshort_t* cliptop;


//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*	sprites;

static spriteframe_t	sprtemp[29];

extern int	numspritelumps;

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
static void
R_InstallSpriteLump
(
  char*		spritename,
  int		index,
  int		lump,
  unsigned	frame,
  unsigned	rotation,
  boolean	flipped )
{
  if (rotation > 9)			// Sort out A - G
    rotation -= 7;

#if 0
  printf ("Adding sprite %d %d %d %d %d %s %s\n",
	index, lump, frame, rotation, flipped, spritename,
	lumpinfo[lump].name);
#endif

  if (frame >= ARRAY_SIZE(sprtemp) || rotation > ARRAY_SIZE(sprtemp[frame].lump))
      I_Error("R_InstallSpriteLump: "
	      "Bad frame characters in lump %i frame %i (%i/%i)", lump, frame, rotation, ARRAY_SIZE(sprtemp[frame].lump));

  if (rotation == 0)
  {
    do
    {
      sprtemp[frame].index[rotation] = index;
      sprtemp[frame].lump[rotation] = lump;
    } while (++rotation < ARRAY_SIZE(sprtemp[frame].lump));
    if (flipped)
      sprtemp[frame].flip = ~0;
    else
      sprtemp[frame].flip = 0;		// Just in case.
  }
  else
  {
    // make 0 based
    // Appears to be 0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15
    // Note: Lookup table not required as the ARM compiler only
    // uses four instructions for the following 4 lines.
    // E3580008 : CMP     R8,#8
    // E1A01088 : MOV     R1,R8,LSL #1
    // 92418002 : SUBLS   R8,R1,#2
    // 82418011 : SUBHI   R8,R1,#&11	; =17

    if (rotation > 8)
      rotation = ((rotation - 9) * 2) + 1;
    else
      rotation = (rotation - 1) * 2;

    if (sprtemp[frame].lump[rotation] != -1)
	I_Error ("R_InitSprites: Sprite %s : %c : %c "
		 "has two lumps mapped to it (%d and %d)",
		 spritename, 'A'+frame, '1'+rotation,
		 sprtemp[frame].lump[rotation], lump);

    sprtemp[frame].index[rotation] = index;
    sprtemp[frame].lump[rotation] = lump;
    if (flipped)
      sprtemp[frame].flip |= (1<<rotation);
    else
      sprtemp[frame].flip &= ~(1<<rotation);
  }
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
static void R_InitSpriteDefs (void)
{
  int 	i;
  int 	index;
  int 	frame;
  int 	rotation;
  int 	rot;
  int 	lump;
  int	maxframe;
  spriteframe_t * sprptr;

  sprites = Z_Calloc (NUMSPRITES * sizeof(*sprites), PU_STATIC, NULL);

  // scan all the lump names for each of the names,
  //	noting the highest frame letter.
  i = 0;
  do
  {
    if (sprnames[i] != NULL)
    {
      // printf ("Looking for sprite %u of %u (%s)\n", i, NUMSPRITES, sprnames[i]);

      sprptr = &sprtemp[0];
      maxframe = ARRAY_SIZE(sprtemp);
      do
      {
	for (rotation=0 ; rotation < ARRAY_SIZE(sprptr->lump) ; rotation++)
	{
	  sprptr->lump [rotation] = -1;
	}
	sprptr -> flip = 0;
	sprptr++;
      } while (--maxframe);

      maxframe = -1;

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
	  do
	  {
	    lump--;
	    if ((strncasecmp (lumpinfo[lump].name, "S_START", 8) == 0)
	     || (lumpinfo[lump].handle != lumpinfo[lump+1].handle))
	    {
	      break;
	    }

	    if (lumpinfo[lump].name[0])
	    {
	      if (--index < 0)
		I_Error ("R_InitSpriteDefs: index < 0\n");

	      if (strncasecmp (lumpinfo[lump].name, sprnames[i], 4) == 0)
	      {
		frame = lumpinfo[lump].name[4] - 'A';
		rotation = lumpinfo[lump].name[5] - '0';

		if (frame >= 0)
		{
		  if (frame > maxframe)
		    maxframe = frame;
		  R_InstallSpriteLump (sprnames[i], index, lump, frame, rotation, false);
		}

		if (lumpinfo[lump].name[6])
		{
		  frame = lumpinfo[lump].name[6] - 'A';
		  rotation = lumpinfo[lump].name[7] - '0';
		  if (frame > maxframe)
		    maxframe = frame;
		  R_InstallSpriteLump (sprnames[i], index, lump, frame, rotation, true);
		}
	      }
	    }
	  } while (1);
	}
      } while (lump);

      maxframe++;
      sprites[i].numframes = maxframe;

      // check the frames that were found for completeness
      if (maxframe)
      {
	sprptr = &sprtemp[0];
	frame = 0;
	do
	{
	  // must have all 16 rotations
	  rotation = 0;
	  do
	  {
	    if (sprptr->lump[rotation] == -1)
	    {
	      if (rotation & 1)			// We expect rotations 8-15 to be missing in most cases.
	      {
		sprptr->lump[rotation] = sprptr->lump[rotation-1];
		sprptr->index[rotation] = sprptr->index[rotation-1];

		if (sprptr->flip & (1<<(rotation-1)))
		  sprptr->flip |= (1<<rotation);
		else
		  sprptr->flip &= ~(1<<rotation);
	      }
	      else
	      {
		if (M_CheckParm ("-showunknown"))
		  printf ("R_InitSprites: Sprite %s frame %c is missing rotation %u\n",
			sprnames[i], frame+'A', rotation);

		switch (rotation)
		{
		  case 0*2:
		    if (sprptr->lump[4*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[4*2];
		      sprptr->index[rotation] = sprptr->index[4*2];
		      if (sprptr->flip & (1<<(4*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 4*2:
		    if (sprptr->lump[0*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[0*2];
		      sprptr->index[rotation] = sprptr->index[0*2];
		      if (sprptr->flip & (1<<(0*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 2*2:
		    if (sprptr->lump[6*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[6*2];
		      sprptr->index[rotation] = sprptr->index[6*2];
		      if (sprptr->flip & (1<<(6*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 6*2:
		    if (sprptr->lump[2*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[2*2];
		      sprptr->index[rotation] = sprptr->index[2*2];
		      if (sprptr->flip & (1<<(2*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 3*2:
		    if (sprptr->lump[5*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[5*2];
		      sprptr->index[rotation] = sprptr->index[5*2];
		      if (sprptr->flip & (1<<(5*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 5*2:
		    if (sprptr->lump[3*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[3*2];
		      sprptr->index[rotation] = sprptr->index[3*2];
		      if (sprptr->flip & (1<<(3*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 1*2:
		    if (sprptr->lump[7*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[7*2];
		      sprptr->index[rotation] = sprptr->index[7*2];
		      if (sprptr->flip & (1<<(7*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;

		  case 7*2:
		    if (sprptr->lump[1*2] != -1)
		    {
		      sprptr->lump[rotation] = sprptr->lump[1*2];
		      sprptr->index[rotation] = sprptr->index[1*2];
		      if (sprptr->flip & (1<<(1*2)))
			sprptr->flip &= ~(1<<rotation);	// Invert the flip bit.
		      else
			sprptr->flip |= (1<<rotation);
		    }
		    break;
		}

		if (sprptr->lump[rotation] == -1)
		{
		  if (rotation)
		  {
		    sprptr->lump[rotation] = sprptr->lump[rotation-1];
		    sprptr->index[rotation] = sprptr->index[rotation-1];
		    if (sprptr->flip & (1<<(rotation-1)))
		      sprptr->flip |= (1<<rotation);
		    else
		      sprptr->flip &= ~(1<<rotation);
		  }
		  else
		  {
		    rot = rotation;
		    do
		    {
		      if (sprptr->lump[rot] != -1)
		      {
			sprptr->lump[rotation] = sprptr->lump[rot];
			sprptr->index[rotation] = sprptr->index[rot];
			if (sprptr->flip & (1<<rot))
			  sprptr->flip |= (1<<rotation);
			else
			  sprptr->flip &= ~(1<<rotation);
			break;
		      }
		    } while (++rot < ARRAY_SIZE(sprptr->lump));
		  }
		}
	      }
	    }
	  } while (++rotation < ARRAY_SIZE(sprptr->lump));
	  sprptr++;
	} while (++frame < maxframe);

	// allocate space for the frames present and copy sprtemp to it
	sprites[i].spriteframes = Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
      }
    }
  } while (++i < NUMSPRITES);

  /* In theory, if we counted everything correctly, index should be 0 at this point */
  if ((index)
   && (M_CheckParm ("-showunknown")))
    printf ("R_InitSpriteDefs: index = %d\n", index);
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
void R_InitSprites (void)
{
    int 	i;

    negonearray = malloc (SCREENWIDTH * sizeof (*negonearray));
    screenheightarray = malloc (SCREENWIDTH * sizeof (*screenheightarray));
    clipbot = malloc (SCREENWIDTH * sizeof (*clipbot));
    cliptop = malloc (SCREENWIDTH * sizeof (*cliptop));

    if ((negonearray == NULL)
     || (screenheightarray == NULL)
     || (clipbot == NULL)
     || (cliptop == NULL))
      I_Error ("Failed to claim memory\n");

    for (i=0 ; i<SCREENWIDTH ; i++)
    {
	negonearray[i] = -1;
    }

    R_InitSpriteDefs ();

    num_vissprite = 0;
    qty_vissprites = 256;			// Start low.
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

dshort_t*	mfloorclip;
dshort_t*	mceilingclip;

fixed_t 	spryscale;
//fixed_t 	sprydistance;
fixed_t 	sprtopscreen;



static void R_DrawMaskedSpriteColumn(column_t *column)
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
	    dc_texturefrac = R_CalcFrac ();
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
( vissprite_t*		vis)
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
    spryscale = vis->xscale;
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
	R_DrawMaskedSpriteColumn (column);
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
  int heightsec;
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
  sector_t *sector;
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
  if ((tz < MINZ) || (tx > MAXZ))
    return;

  distance = FixedDiv(projection, tz);

  gxt = -FixedMul(tr_x,viewsin);
  gyt = FixedMul(tr_y,viewcos);
  tx = -(gyt+gxt);

  // too far off the side?
  if (abs(tx/4) > tz)
    return;

  // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
  if ((unsigned)thing->sprite >= NUMSPRITES)
    I_Error ("R_ProjectSprite: invalid sprite number %i/%i", thing->sprite,NUMSPRITES);
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

  // choose a different rotation based on player view
  ang = R_PointToAngle (thing->x, thing->y);

  if (sprframe->lump[0] != sprframe->lump[1])		// Sprite with 16 rotations?
    rot = ((ang-thing->angle+(unsigned)(ANG45/2)*9)-(1<<27))>>28;
  else
    rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>28;

  if (rot >= ARRAY_SIZE(sprframe->index))
    rot = 0;

  lump = sprframe->lump[rot];
  if (lump == -1)
    return;

  index = sprframe->index[rot];
  flip = (boolean) (sprframe->flip & (1<<rot));

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


  // killough 3/27/98: exclude things totally separated
  // from the viewer, by either water or fake ceilings
  // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
  sector = thing->subsector->sector;
  heightsec = sector->heightsec;

  if (heightsec != -1)		// only clip things which are in special sectors
  {
    fixed_t fz;
    fixed_t gzt;
    int phs;

    fz = thing->z;
    gzt = fz + spritetopoffset[index];
    phs = viewplayer->mo->subsector->sector->heightsec;

    if (phs != -1 && (viewz < sectors[phs].floorheight ?
     fz >= sectors[heightsec].floorheight :
     gzt < sectors[heightsec].floorheight))
	return;
    if (phs != -1 && (viewz > sectors[phs].ceilingheight ?
     gzt < sectors[heightsec].ceilingheight &&
     viewz >= sectors[heightsec].ceilingheight :
     fz >= sectors[heightsec].ceilingheight))
	return;
  }


  // store information in a vissprite
  vis = R_NewVisSprite (distance << detailshift);
  if (vis == NULL)
    return;

  // killough 3/27/98: save sector for special clipping later
  vis->heightsec = heightsec;

  xscale = FixedMul (distance, spritescale);

  vis->mobjflags = thing->flags;
  vis->distance = distance<<detailshift;
  vis->xscale = xscale<<detailshift;
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
    state_t*		state;
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int 		index;
    int 		lump;
    boolean		flip;
    vissprite_t*	vis;
    vissprite_t 	avis;

    // decide which patch to use
    state = psp->state;

#ifdef RANGECHECK
    if ( (unsigned)state->sprite >= NUMSPRITES)
    {
	I_Error ("R_DrawPSprite: invalid sprite number %i ",
		 state->sprite);
	return;
    }
#endif
    sprdef = &sprites[state->sprite];
#ifdef RANGECHECK
    if ( (state->frame & FF_FRAMEMASK) >= sprdef->numframes)
#if 1
	state->frame &= ~FF_FRAMEMASK;
#else
	I_Error ("R_DrawPSprite: invalid sprite frame %i : %i/%i\n",
		 state->sprite, state->frame, sprdef->numframes);
#endif
#endif

    if (sprdef->spriteframes == NULL)
    {
//    printf ("Missing sprite %u (%s)\n", state->sprite, sprnames [state->sprite]);
      return;
    }

    sprframe = &sprdef->spriteframes [state->frame & FF_FRAMEMASK];

    lump = sprframe->lump[0];
    if (lump == -1)
      return;

    index = sprframe->index[0];
    flip = (boolean) (sprframe->flip & (1<<0));

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
    vis->mobjflags = (int) (state->frame & FF_TRANSLUCENT);	// Same as MF_TRANSLUCENT
    vis->texturemid1 =
    vis->texturemid2 = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-(psp->sy-spritetopoffset[index]);
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
    vis->xscale =
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
    else if (state->frame & FF_FULLBRIGHT)
    {
	// full bright
	vis->colormap = colormaps;
    }
    else
    {
	// local light
	vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }

    R_DrawVisSprite (vis);
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
    int 		x;
    int 		r1;
    int 		r2;
    fixed_t		scale;
    fixed_t		lowscale;

    // [RH] Quickly reject sprites with bad x ranges.
    if (spr->x1 > spr->x2)
      return;

    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
	cliptop[x] = -1;
	clipbot[x] = viewheight;
    }

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

	if (ds->silhouette & SIL_BOTTOM)
	    for (x = r1; x <= r2; x++)
		if (clipbot[x] > ds->sprbottomclip[x])
		    clipbot[x] = ds->sprbottomclip[x];

	if (ds->silhouette & SIL_TOP)
	    for (x = r1; x <= r2; x++)
		if (cliptop[x] < ds->sprtopclip[x])
		    cliptop[x] = ds->sprtopclip[x];
    }

    // killough 3/27/98:
    // Clip the sprite against deep water and/or fake ceilings.
    // killough 4/9/98: optimize by adding mh
    // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
    // killough 11/98: fix disappearing sprites
    if (spr->heightsec != -1)  // only things in specially marked sectors
    {
	fixed_t	h, mh;
	int	phs = viewplayer->mo->subsector->sector->heightsec;

	if ((mh = sectors[spr->heightsec].floorheight) > spr->gz
	    && (h = centeryfrac - FixedMul(mh -= viewz, spr->distance)) >= 0
	    && (h >>= FRACBITS) < viewheight)
	{
	    if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floorheight))
	    {								// clip bottom
		for (x = spr->x1; x <= spr->x2; x++)
		    if (h < clipbot[x])
			clipbot[x] = h;
	    }
	    else							// clip top
		if (phs != -1 && viewz <= sectors[phs].floorheight)	// killough 11/98
		    for (x = spr->x1; x <= spr->x2; x++)
			if (h > cliptop[x])
			    cliptop[x] = h;
	}

	if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt
	    && (h = centeryfrac - FixedMul(mh - viewz, spr->distance)) >= 0
	    && (h >>= FRACBITS) < viewheight)
	{
	    if (phs != -1 && viewz >= sectors[phs].ceilingheight)
	    {								// clip bottom
		for (x = spr->x1; x <= spr->x2; x++)
		    if (h < clipbot[x])
			clipbot[x] = h;
	    }
	    else							// clip top
		for (x = spr->x1; x <= spr->x2; x++)
		    if (h > cliptop[x])
			cliptop[x] = h;
	}
    }

    // all clipping has been performed, so draw the sprite

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr);
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



