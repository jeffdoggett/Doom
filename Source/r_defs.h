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
//      Refresh/rendering module, shared data struct definitions.
//
//-----------------------------------------------------------------------------


#ifndef __R_DEFS__
#define __R_DEFS__

#include <sys/types.h>

// Screenwidth.
#include "doomdef.h"

// Some more or less basic data types
// we depend on.
#include "m_fixed.h"

// We rely on the thinker data struct
// to handle sound origins in sectors.
#include "d_think.h"
// SECTORS do store MObjs anyway.
#include "p_mobj.h"



#ifdef __GNUG__
#pragma interface
#endif



// Silhouette, needed for clipping Segs (mainly)
// and sprites representing things.
#define SIL_NONE		0
#define SIL_BOTTOM		1
#define SIL_TOP			2
#define SIL_BOTH		3

#define MAXDRAWSEGS		2048





//
// INTERNAL MAP TYPES
//  used by play and refresh
//

//
// Your plain vanilla vertex.
// Note: transformed values not buffered locally,
//  like some DOOM-alikes ("wt", "WebView") did.
//
typedef struct
{
  fixed_t	x;
  fixed_t	y;
  angle_t	viewangle;
  unsigned int	angletime;
} vertex_t;


// Forward of LineDefs, for Sectors.
struct line_s;

// Each sector has a degenmobj_t in its center
//  for sound origin purposes.
// I suppose this does not handle sound from
//  moving objects (doppler), because
//  position is prolly just buffered, not
//  updated.
typedef struct
{
    thinker_t		thinker;	// not used for anything
    fixed_t		x;
    fixed_t		y;
    fixed_t		z;

} degenmobj_t;

//
// The SECTORS record, at runtime.
// Stores things/mobjs.
//
typedef	struct
{
    fixed_t	floorheight;
    fixed_t	ceilingheight;
    dshort_t	floorpic;
    dshort_t	ceilingpic;
    dshort_t	lightlevel;
    dushort_t	special;
    dushort_t	tag;

    // 0 = untraversed, 1,2 = sndlines -1
    int		soundtraversed;

    // thing that made a sound (or null)
    mobj_t*	soundtarget;

    // mapblock bounding box for height changes
    int		blockbox[4];

    // origin for any sounds played by the sector
    degenmobj_t	soundorg;

    // if == validcount, already checked
    int		validcount;

    // list of mobjs in sector
    mobj_t*	thinglist;

    int			linecount;
    struct line_s**	lines;	// [linecount] size

  // killough 10/98: support skies coming from sidedefs. Allows scrolling
  // skies and other effects. No "level info" kind of lump is needed,
  // because you can use an arbitrary number of skies per level with this
  // method. This field only applies when skyflatnum is used for floorpic
  // or ceilingpic, because the rest of Doom needs to know which is sky
  // and which isn't, etc.

    int sky;

#ifdef USE_BOOM_P_ChangeSector
    struct msecnode_s*	touching_thinglist;
#endif

    dushort_t	oldspecial;
    void*	floordata;
    void*	ceilingdata;
    void*	lightingdata;

    fixed_t	floor_xoffs;
    fixed_t	floor_yoffs;
    fixed_t	ceiling_xoffs;
    fixed_t	ceiling_yoffs;
    int		heightsec;
    int		floorlightsec;
    int		ceilinglightsec;
    int		firsttag;
    int		nexttag;
    int		stairlock;
    int		cachedheight;	// [kb] for R_FixWiggle()
    int		scaleindex;	// [kb] for R_FixWiggle()
} sector_t;



//
// The SideDef.
//

typedef struct
{
    // add this to the calculated texture column
    fixed_t	textureoffset;

    // add this to the calculated texture top
    fixed_t	rowoffset;

    // Texture indices.
    // We do not maintain names here.
    dshort_t	toptexture;
    dshort_t	bottomtexture;
    dshort_t	midtexture;

    // Sector the SideDef is facing.
    sector_t*	sector;

#ifdef BOOM
    int		special;
#endif
} side_t;



//
// Move clipping aid for LineDefs.
//
typedef enum
{
    ST_HORIZONTAL,
    ST_VERTICAL,
    ST_POSITIVE,
    ST_NEGATIVE

} slopetype_t;



typedef struct line_s
{
    // Vertices, from v1 to v2.
    vertex_t*	v1;
    vertex_t*	v2;

    // Precalculated v2 - v1 for side checking.
    fixed_t	dx;
    fixed_t	dy;

    // Animation related.
    dushort_t	flags;
    dushort_t	special;
    dushort_t	tag;

    // Visual appearance: SideDefs.
    //  sidenum[1] will be -1 if one sided
    dushort_t	sidenum[2];

    // Neat. Another bounding box, for the extent
    //  of the LineDef.
    fixed_t	bbox[4];

    // To aid move clipping.
    slopetype_t	slopetype;

    // Front and back sector.
    // Note: redundant? Can be retrieved from SideDefs.
    sector_t*	frontsector;
    sector_t*	backsector;

    // if == validcount, already checked
    int		validcount;
    int		firsttag;
    int		nexttag;

    // thinker_t for reversable actions
//  void*	specialdata;

    // sound origin for switches/buttons
    degenmobj_t soundorg;
} line_t;




//
// A SubSector.
// References a Sector.
// Basically, this is a list of LineSegs,
//  indicating the visible walls that define
//  (all or some) sides of a convex BSP leaf.
//
typedef struct subsector_s
{
    sector_t*	sector;
    dushort_t	numlines;
    dushort_t	firstline;

} subsector_t;

#ifdef USE_BOOM_P_ChangeSector

// phares 3/14/98
//
// Sector list node showing all sectors an object appears in.
//
// There are two threads that flow through these nodes. The first thread
// starts at touching_thinglist in a sector_t and flows through the m_snext
// links to find all mobjs that are entirely or partially in the sector.
// The second thread starts at touching_sectorlist in an mobj_t and flows
// through the m_tnext links to find all sectors a thing touches. This is
// useful when applying friction or push effects to sectors. These effects
// can be done as thinkers that act upon all objects touching their sectors.
// As an mobj moves through the world, these nodes are created and
// destroyed, with the links changed appropriately.
//
// For the links, NULL means top or end of list.

typedef struct msecnode_s
{
  sector_t		*m_sector;	// a sector containing this object
  struct mobj_s		*m_thing;	// this object
  struct msecnode_s	*m_tprev;	// prev msecnode_t for this thing
  struct msecnode_s	*m_tnext;	// next msecnode_t for this thing
  struct msecnode_s	*m_sprev;	// prev msecnode_t for this sector
  struct msecnode_s	*m_snext;	// next msecnode_t for this sector
  boolean		visited;	// killough 4/4/98, 4/7/98: used in search algorithms
} msecnode_t;

extern msecnode_t *sector_list;		// phares 3/16/98
extern msecnode_t *headsecnode;

#endif

//
// The LineSeg.
//
typedef struct
{
    vertex_t*	v1;
    vertex_t*	v2;

    fixed_t	offset;

    angle_t	angle;

    side_t*	sidedef;
    line_t*	linedef;

    // Sector references.
    // Could be retrieved from linedef, too.
    // backsector is NULL for one sided lines
    sector_t*	frontsector;
    sector_t*	backsector;
    uint32_t	length;
} seg_t;



//
// BSP node.
//
typedef struct
{
    // Partition line.
    fixed_t	x;
    fixed_t	y;
    fixed_t	dx;
    fixed_t	dy;

    // Bounding box for each child.
    fixed_t	bbox[2][4];

    // If NF_SUBSECTOR its a subsector.
    unsigned int children[2];

} node_t;




// posts are runs of non masked source pixels
typedef struct
{
    byte		topdelta;	// -1 is the last post in a column
    byte		length; 	// length data bytes follows
} post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t	column_t;



// PC direct to screen pointers
//B UNUSED - keep till detailshift in r_draw.c resolved
//extern byte*	destview;
//extern byte*	destscreen;





//
// OTHER TYPES
//

// This could be wider for >8 bit display.
// Indeed, true color support is posibble
//  precalculating 24bpp lightmap/colormap LUT.
//  from darkening PLAYPAL to all black.
// Could even us emore than 32 levels.
typedef byte	lighttable_t;




//
// ?
//
typedef struct drawseg_s
{
    seg_t*		curline;
    int			x1;
    int			x2;

    fixed_t		scale1;
    fixed_t		scale2;
    fixed_t		scalestep;

    // 0=none, 1=bottom, 2=top, 3=both
    int			silhouette;

    // do not clip sprites above this
    fixed_t		bsilheight;

    // do not clip sprites below this
    fixed_t		tsilheight;

    // Pointers to lists for sprite clipping,
    //  all three adjusted so [x1] is first value.
    dshort_t*		sprtopclip;
    dshort_t*		sprbottomclip;
    dshort_t*		maskedtexturecol;

} drawseg_t;



// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.
typedef struct
{
    short		width;		// bounding box size
    short		height;
    short		leftoffset;	// pixels to the left of origin
    short		topoffset;	// pixels below the origin
    int			columnofs[8];	// only [width] used
    // the [0] is &columnofs[width]
} patch_t;







// A vissprite_t is a thing
//  that will be drawn during a refresh.
// I.e. a sprite object that is partly visible.
typedef struct
{
    int			x1;
    int			x2;

    // for line side calculation
    fixed_t		gx;
    fixed_t		gy;

    // global bottom / top for silhouette clipping
    fixed_t		gz;
    fixed_t		gzt;

    // horizontal position of x1
    fixed_t		startfrac;

    // how far away the sprite is from the viewer
    fixed_t		distance;

    fixed_t		scale;

    // negative if flipped
    fixed_t		xiscale;

    fixed_t		texturemid1;
    fixed_t		texturemid2;
    int			patch;

    // for color translation and shadow draw,
    //  maxbright frames as well
    lighttable_t*	colormap;

    int			mobjflags;

#ifdef BOOM
    int			heightsec;
#endif
} vissprite_t;


//
// Sprites are patches with a special naming convention
//  so they can be recognized by R_InitSprites.
// The base name is NNNNFx or NNNNFxFx, with
//  x indicating the rotation, x = 0, 1-7.
// The sprite and frame specified by a thing_t
//  is range checked at run time.
// A sprite is a patch_t that is assumed to represent
//  a three dimensional object and may have multiple
//  rotations pre drawn.
// Horizontal flipping is used to save space,
//  thus NNNNF2F5 defines a mirrored patch.
// Some sprites will only have one picture used
// for all views: NNNNF0
//
typedef struct
{
    // If false use 0 for any position.
    // Note: as eight entries are available,
    //  we might as well insert the same name eight times.
    boolean	rotate;

    // The sprite number - same as lump_number-firstspritenum except when
    // using partial PWADs.
    dshort_t	index[8];

    // Lump to use for view angles 0-7.
    dshort_t	lump[8];

    // Flip bit (1 = flip) to use for view angles 0-7.
    byte	flip[8];

} spriteframe_t;



//
// A sprite definition:
//  a number of animation frames.
//
typedef struct
{
    int			numframes;
    spriteframe_t*	spriteframes;

} spritedef_t;



//
// Now what is a visplane, anyway?
//
typedef struct
{
  fixed_t		height;
  int			picnum;
  int			lightlevel;
  int			minx;
  int			maxx;

  // Here lies the rub for all
  // dynamic resize/change of resolution.
  // leave pads for [minx-1]/[maxx+1]
  dushort_t*		top;
  dushort_t*		bottom;
  fixed_t		xoffs;
  fixed_t		yoffs;
} visplane_t;




#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
