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
//	Refresh, visplane stuff (floor, ceilings).
//
//-----------------------------------------------------------------------------


#ifndef __R_PLANE__
#define __R_PLANE__


#include "r_data.h"

#ifdef __GNUG__
#pragma interface
#endif

// killough 10/98: special mask indicates sky flat comes from sidedef
#define PL_SKYFLAT (0x80000000)

// Visplane related.
extern unsigned int	MAXOPENINGS;
extern	dshort_t*	openings;
extern	dshort_t*	lastopening;


typedef void (*planefunction_t) (int top, int bottom);

extern planefunction_t	floorfunc;
extern planefunction_t	ceilingfunc_t;

extern dshort_t		floorclip[MAXSCREENWIDTH];
extern dshort_t		ceilingclip[MAXSCREENWIDTH];

extern fixed_t		yslope[MAXSCREENHEIGHT];
extern fixed_t		distscale[MAXSCREENWIDTH];

void R_InitPlanes (void);
void R_ClearPlanes (void);
pint R_IncreaseOpenings (size_t);
void R_MapPlane (int y, int x1, int x2);
void R_DrawPlanes (void);
visplane_t* R_FindPlane (fixed_t height, int picnum, int lightlevel, fixed_t xoffs, fixed_t yoffs);
visplane_t* R_CheckPlane (visplane_t* pl, int start, int stop);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
