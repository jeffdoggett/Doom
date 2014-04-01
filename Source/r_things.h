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
// DESCRIPTION:
//	Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------


#ifndef __R_THINGS__
#define __R_THINGS__


#ifdef __GNUG__
#pragma interface
#endif


// Constant arrays used for psprite clipping
//  and initializing clipping.
extern dshort_t		negonearray[MAXSCREENWIDTH];
extern dshort_t		screenheightarray[MAXSCREENWIDTH];

// vars for R_DrawMaskedColumn
extern dshort_t*	mfloorclip;
extern dshort_t*	mceilingclip;
extern fixed_t		spryscale;
extern fixed_t		sprtopscreen;

extern fixed_t		pspritescale;
extern fixed_t		pspriteiscale;


void R_DrawMaskedColumn (column_t* column);
void R_AddSprites (sector_t* sec);
void R_InitSprites (char** namelist);
void R_ClearSprites (void);
void R_DrawMasked (void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
