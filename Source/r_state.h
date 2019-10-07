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
//	Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef __R_STATE__
#define __R_STATE__

// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"



#ifdef __GNUG__
#pragma interface
#endif



//
// Refresh internal data structures,
//  for rendering.
//

typedef struct
{
  texture_t *	texture;
  fixed_t	height;		// for texture pegging
  int		translation;	// for global animation
  int		lump;
} texture_info_t;

// needed for pre rendering (fracs)
extern fixed_t* 	spritewidth;

extern fixed_t* 	spriteoffset;
extern fixed_t* 	spritetopoffset;

/* Colourmap type fixed to byte by WAD! So use bytes, not lighttable_t */
#ifdef BOOM
//extern byte**		colormaps;
extern lighttable_t*	colormaps;
extern byte*		fullcolormap;
extern int		numcolormaps;
#else
//extern byte*		colormaps;
extern lighttable_t*	colormaps;
#endif

extern int		viewwidth;
extern int		scaledviewwidth;
extern int		viewheight;

// for global animation
extern int		numtextures;
extern texture_info_t * textures;

//
// Lookup tables for map data.
//
extern spritedef_t*	sprites;

extern unsigned int	numvertexes;
extern vertex_t*	vertexes;

extern unsigned int	numsegs;
extern seg_t*		segs;

extern unsigned int	numsectors;
extern sector_t*	sectors;

extern unsigned int	numsubsectors;
extern subsector_t*	subsectors;

extern unsigned int	numnodes;
extern node_t*		nodes;

extern unsigned int	numlines;
extern line_t*		lines;

extern unsigned int	numsides;
extern side_t*		sides;


//
// POV data.
//
extern fixed_t		viewx;
extern fixed_t		viewy;
extern fixed_t		viewz;

extern angle_t		viewangle;
extern player_t*	viewplayer;


// ?
extern angle_t		clipangle;

extern int		viewangletox[FINEANGLES/2];
extern angle_t*		xtoviewangle;
//extern fixed_t		finetangent[FINEANGLES/2];

extern angle_t		rw_normalangle;



// Segs count?
extern int		sscount;

extern visplane_t*	floorplane;
extern visplane_t*	ceilingplane;


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
