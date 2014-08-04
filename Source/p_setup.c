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
//	Do all the WAD I/O, get map description,
//	set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_setup.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif

#include "includes.h"
#include <math.h>

#if defined(LINUX) || defined(HAVE_ALLOCA)
#include  <alloca.h>
#else
extern void * alloca (unsigned int);
#endif

extern unsigned int P_SpawnMapThing (mapthing_t* mthing);
extern void A_Activate_Death_Sectors (unsigned int monsterbits);
extern void P_Init_Intercepts (void);
extern void P_Init_SpecHit (void);


//-----------------------------------------------------------------------------
//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
unsigned int	numvertexes;
vertex_t*	vertexes;

unsigned int	numsegs;
seg_t*		segs;

unsigned int	numsectors;
sector_t*	sectors;

unsigned int	numsubsectors;
subsector_t*	subsectors;

unsigned int	numnodes;
node_t*		nodes;

unsigned int	numlines;
line_t*		lines;

unsigned int	numsides;
side_t*		sides;


//-----------------------------------------------------------------------------
// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int		bmapwidth;
int		bmapheight;	// size in mapblocks
uint32_t*	blockmapindex;	// int for larger maps
// offsets in blockmap are from here
uint32_t*	blockmaphead;
// origin of block map
fixed_t		bmaporgx;
fixed_t		bmaporgy;
// for thing chains
mobj_t**	blocklinks;


//-----------------------------------------------------------------------------
// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*		rejectmatrix;
unsigned int	rejectmatrixsize;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS	10

mapthing_t	deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*	deathmatch_p;
mapthing_t	playerstarts[MAXPLAYERS];


//-----------------------------------------------------------------------------
//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
    byte*		data;
    int			i;
    mapvertex_t*	ml;
    vertex_t*		li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);

    // Load data into cache.
    data = W_CacheLumpNum (lump,PU_STATIC);

    ml = (mapvertex_t *)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
	li->x = SHORT(ml->x)<<FRACBITS;
	li->y = SHORT(ml->y)<<FRACBITS;
    }

    // Free buffer memory.
    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
  byte*		data;
  int		i;
  mapseg_t*	ml;
  seg_t*	li;
  line_t*	ldef;
  unsigned int	linedef;
  unsigned int	side;
//  byte*		vertchanged;

#ifdef PADDED_STRUCTS
  numsegs = W_LumpLength (lump) / 12;
#else
  numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
#endif
  segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
  memset (segs, 0, numsegs*sizeof(seg_t));
  data = W_CacheLumpNum (lump,PU_STATIC);

#if 0
  i = (numvertexes+7)>>3;
  vertchanged = (byte *) alloca (i);
  if (vertchanged)
      memset(vertchanged, 0, i);
#endif

  ml = (mapseg_t *)data;
  li = segs;
  for (i=0 ; i<numsegs ; i++, li++)
  {
    unsigned int v;

    v = USHORT(ml->v1);
    if (v >= numvertexes)
      I_Error ("P_LoadSegs: invalid vertex %u", v);

    li->v1 = &vertexes[v];

    v = USHORT(ml->v2);
    if (v >= numvertexes)
      I_Error ("P_LoadSegs: invalid vertex %u", v);

    li->v2 = &vertexes[v];

    li->angle = (SHORT(ml->angle))<<16;

#if 0
    {
      // Now done later in P_RemoveSlimeTrails
      int	ptp_angle;
      int	delta_angle;

      /* firelines fix -- taken from Boom source */
      ptp_angle = R_PointToAngle2(li->v1->x,li->v1->y,li->v2->x,li->v2->y);
      delta_angle = (abs(ptp_angle-li->angle)>>ANGLETOFINESHIFT)*360/8192;

      if ((delta_angle != 0) && (vertchanged != NULL))
      {
	int dis, dx, dy, vnum1, vnum2;

	dx = (li->v1->x - li->v2->x)>>FRACBITS;
	dy = (li->v1->y - li->v2->y)>>FRACBITS;
	dx = dx * dx;
	dy = dy * dy;
	dx = dx + dy;
	dis = ((int) sqrt(dx))<<FRACBITS;
	dx = finecosine[li->angle>>ANGLETOFINESHIFT];
	dy = finesine[li->angle>>ANGLETOFINESHIFT];
	vnum1 = li->v1 - vertexes;
	vnum2 = li->v2 - vertexes;
	if ((vnum2 > vnum1) && ((vertchanged[vnum2>>3] & (1 << (vnum2&7))) == 0))
	{
	    li->v2->x = li->v1->x + FixedMul(dis,dx);
	    li->v2->y = li->v1->y + FixedMul(dis,dy);
	    vertchanged[vnum2>>3] |= 1 << (vnum2&7);
	}
	else if ((vertchanged[vnum1>>3] & (1 << (vnum1&7))) == 0)
	{
	    li->v1->x = li->v2->x - FixedMul(dis,dx);
	    li->v1->y = li->v2->y - FixedMul(dis,dy);
	    vertchanged[vnum1>>3] |= 1 << (vnum1&7);
	}
      }
    }
#endif

    li->offset = (SHORT(ml->offset))<<16;
    linedef = USHORT(ml->linedef);
    if (linedef >= numlines)
    {
      printf ("P_LoadSegs: linedef = %u/%u\n", linedef, linedef);
      linedef = 0;
    }
    ldef = &lines[linedef];
    li->linedef = ldef;
    side = USHORT(ml->side);
    if (side > 1)
    {
      printf ("P_LoadSegs: side = %u\n", side);
      side = 0;
    }
    li->sidedef = &sides[ldef->sidenum[side]];
    li->frontsector = sides[ldef->sidenum[side]].sector;
    if (ldef-> flags & ML_TWOSIDED)
	li->backsector = sides[ldef->sidenum[side^1]].sector;
    else
	li->backsector = 0;
#ifdef PADDED_STRUCTS
    ml = (mapseg_t *) ((byte *) ml + 12);
#else
    ml++;
#endif
  }

  Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
    byte*		data;
    int			i;
    mapsubsector_t*	ms;
    subsector_t*	ss;

    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump,PU_STATIC);

    ms = (mapsubsector_t *)data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;

    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
	ss->numlines = USHORT(ms->numsegs);
	ss->firstline = USHORT(ms->firstseg);
    }

    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
    byte*		data;
    int			i;
    mapsector_t*	ms;
    sector_t*		ss;

#ifdef PADDED_STRUCTS
    numsectors = W_LumpLength (lump) / 26;
#else
    numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
#endif
    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    ms = (mapsector_t *)data;
    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++)
    {
	ss->floorheight = SHORT(ms->floorheight)<<FRACBITS;
	ss->ceilingheight = SHORT(ms->ceilingheight)<<FRACBITS;
	ss->floorpic = R_FlatNumForName(ms->floorpic);
	ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
	ss->lightlevel = SHORT(ms->lightlevel);
	ss->special = SHORT(ms->special);
	ss->tag = SHORT(ms->tag);
	ss->thinglist = NULL;
#ifdef USE_BOOM_P_ChangeSector
	ss->touching_thinglist = NULL;  // phares 3/14/98
#endif
//	ss->nextsec = -1;
//	ss->prevsec = -1;
	ss->floor_xoffs = 0;
	ss->floor_yoffs = 0;
	ss->ceiling_xoffs = 0;
	ss->ceiling_yoffs = 0;
	ss->heightsec = -1;
	ss->floorlightsec = -1;
	ss->ceilinglightsec = -1;
//	ss->bottommap = 0;
//	ss->midmap = 0;
//	ss->topmap = 0;
#ifdef PADDED_STRUCTS
	ms = (mapsector_t *) ((byte *) ms + 26);
#else
	ms++;
#endif
    }

    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
    byte*	data;
    int		i;
    int		j;
    int		k;
    unsigned int children;
    mapnode_t*	mn;
    node_t*	no;

    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump,PU_STATIC);

    mn = (mapnode_t *)data;
    no = nodes;

    for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
	no->x = SHORT(mn->x)<<FRACBITS;
	no->y = SHORT(mn->y)<<FRACBITS;
	no->dx = SHORT(mn->dx)<<FRACBITS;
	no->dy = SHORT(mn->dy)<<FRACBITS;
	for (j=0 ; j<2 ; j++)
	{
	  children = USHORT(mn->children[j]);
	  // e6y: support for extended nodes
	  if (children == 0xFFFF)
	  {
	      children = (unsigned int) -1;
	  }
#if NF_SUBSECTOR!=0x8000
	  else if (children & 0x8000)
	  {
	      // Convert to extended type
	      children &= ~0x8000;

	      // haleyjd 11/06/10: check for invalid subsector reference
	      if (children >= numsubsectors)
		  children = 0;

	      children |= NF_SUBSECTOR;
	  }
#endif
	  no->children[j] = children;

	  for (k=0 ; k<4 ; k++)
	       no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
	}
    }

    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadThings
//
static unsigned int P_LoadThings (int lump)
{
    byte*		data;
    int			i;
    mapthing_t*		mt;
    mapthing_t		mtl;
    int			numthings;
    boolean		spawn;
    unsigned int	nomonsterbits;
    unsigned int	monsternotloaded;

#if 0
    boolean		bfgedition;
    bfgedition = false;
    if (W_CheckNumForName("DMENUPIC") >= 0 && W_CheckNumForName("M_ACPT") >= 0)
    {
      bfgedition = true;
      printf ("BFG edition\n");
    }
#endif

    data = W_CacheLumpNum (lump,PU_STATIC);
#ifdef PADDED_STRUCTS
    numthings = W_LumpLength (lump) / 10;
#else
    numthings = W_LumpLength (lump) / sizeof(mapthing_t);
#endif

    nomonsterbits = 0;

    mt = (mapthing_t *)data;
    for (i=0 ; i<numthings ; i++)
    {
	spawn = true;

	// Do not spawn cool, new monsters if !commercial
	if ( gamemode != commercial)
	{
	    switch(SHORT(mt->type))
	    {
	      case 68:	// Arachnotron
	      case 64:	// Archvile
	      case 88:	// Boss Brain
	      case 89:	// Boss Shooter
	      case 69:	// Hell Knight
	      case 67:	// Mancubus
	      case 71:	// Pain Elemental
	      case 65:	// Former Human Commando
	      case 66:	// Revenant
	      case 84:	// Wolf SS
		spawn = false;
		break;
	    }
	}

	if (spawn)
	{
	  // Do spawn all other stuff.
	  mtl.x = SHORT(mt->x);
	  mtl.y = SHORT(mt->y);
	  mtl.angle = SHORT(mt->angle);
	  mtl.type = SHORT(mt->type);
	  mtl.options = SHORT(mt->options);

#if 0
	  // [BH] change all wolfenstein ss' into zombiemen when using
	  //  BFG Edition DOOM2.WAD as censorship broke them
	  if ((mtl.type == 84) && (bfgedition))
	    mtl.type = 65;
#endif
	  monsternotloaded = P_SpawnMapThing (&mtl);
	  if ((monsternotloaded)		// Luckily the -nomonsters stuff that
	   && (monsternotloaded < 32))		// we are interested in fits in 1 - 31
	  {
	    nomonsterbits |= (1 << monsternotloaded);
	  }
	}

#ifdef PADDED_STRUCTS
	mt = (mapthing_t *) ((byte *) mt + 10);
#else
	mt++;
#endif
    }

    Z_Free (data);

    return (nomonsterbits);
}

//-----------------------------------------------------------------------------
//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    int			i;
    unsigned short	sidenum;
    byte*		data;
    maplinedef_t*	mld;
    line_t*		ld;
    vertex_t*		v1;
    vertex_t*		v2;

#ifdef PADDED_STRUCTS
    numlines = W_LumpLength (lump) / 14;
#else
    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
#endif
    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    mld = (maplinedef_t *)data;
    ld = lines;
    for (i=0 ; i<numlines ; i++, ld++)
    {
	ld->flags = USHORT(mld->flags);
	ld->special = USHORT(mld->special);
	ld->tag = USHORT(mld->tag);
	v1 = ld->v1 = &vertexes[USHORT(mld->v1)];
	v2 = ld->v2 = &vertexes[USHORT(mld->v2)];
	ld->dx = v2->x - v1->x;
	ld->dy = v2->y - v1->y;

	if (!ld->dx)
	    ld->slopetype = ST_VERTICAL;
	else if (!ld->dy)
	    ld->slopetype = ST_HORIZONTAL;
	else
	{
	    if (FixedDiv (ld->dy , ld->dx) > 0)
		ld->slopetype = ST_POSITIVE;
	    else
		ld->slopetype = ST_NEGATIVE;
	}

	if (v1->x < v2->x)
	{
	    ld->bbox[BOXLEFT] = v1->x;
	    ld->bbox[BOXRIGHT] = v2->x;
	}
	else
	{
	    ld->bbox[BOXLEFT] = v2->x;
	    ld->bbox[BOXRIGHT] = v1->x;
	}

	if (v1->y < v2->y)
	{
	    ld->bbox[BOXBOTTOM] = v1->y;
	    ld->bbox[BOXTOP] = v2->y;
	}
	else
	{
	    ld->bbox[BOXBOTTOM] = v2->y;
	    ld->bbox[BOXTOP] = v1->y;
	}

	// calculate sound origin of line to be its midpoint
	ld->soundorg.x = (ld->bbox[BOXLEFT] + ld->bbox[BOXRIGHT] ) / 2;
	ld->soundorg.y = (ld->bbox[BOXTOP] + ld->bbox[BOXBOTTOM]) / 2;

	sidenum = USHORT(mld->sidenum[0]);
	if (sidenum == 0xFFFF)
	{
	  ld->sidenum[0] = (dushort_t) -1;
	  ld->frontsector = 0;
	}
	else
	{
	  if (sidenum >= numsides)
	  {
	    printf ("P_LoadLineDefs: Front sidenum = %u\n", sidenum);
	    sidenum = 0;
	  }
	  ld->sidenum[0] = sidenum;
	  ld->frontsector = sides[sidenum].sector;
	}

	sidenum = USHORT(mld->sidenum[1]);
	if (sidenum == 0xFFFF)
	{
	  ld->sidenum[1] = (dushort_t) -1;
	  ld->backsector = 0;
	  ld->flags &= ~ML_TWOSIDED;		// Just in case (e.g. nova.wad map 32)
	}
	else
	{
	  if (sidenum >= numsides)
	  {
	    printf ("P_LoadLineDefs: Back sidenum = %u\n", sidenum);
	    sidenum = 0;
	  }
	  ld->sidenum[1] = sidenum;
	  ld->backsector = sides[sidenum].sector;
	}

#ifdef PADDED_STRUCTS
	mld = (maplinedef_t *) ((byte *) mld + 14);
#else
	mld++;
#endif

    }

    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
    byte*		data;
    int			i;
    mapsidedef_t*	msd;
    side_t*		sd;

#ifdef PADDED_STRUCTS
    numsides = W_LumpLength (lump) / 30;
#else
    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
#endif
    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);
    memset (sides, 0, numsides*sizeof(side_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i=0 ; i<numsides ; i++, sd++)
    {
	sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
	sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
	sd->toptexture = R_TextureNumForName(msd->toptexture);
	sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
	sd->midtexture = R_TextureNumForName(msd->midtexture);
	sd->sector = &sectors[SHORT(msd->sector)];
#ifdef PADDED_STRUCTS
	msd = (mapsidedef_t *) ((byte *) msd + 30);
#else
	msd++;
#endif

    }

    Z_Free (data);
}

//-----------------------------------------------------------------------------
//
// P_LoadBlockMap
//
void P_LoadBlockMap (int lump)
{
  unsigned int i;
  unsigned int count;		// number of 16 bit blockmap entries
  uint16_t *wadblockmaplump;	// blockmap lump temp
  uint32_t firstlist, lastlist;	// blockmap block list bounds
  uint32_t overflow_corr;
  uint32_t prev_bme;		// for detecting overflow wrap


  count = W_LumpLength(lump) / 2;
  wadblockmaplump = W_CacheLumpNum(lump, PU_LEVEL);
  overflow_corr = 0;
  prev_bme = 0;

  // [WDJ] when zennode has not been run, this code will corrupt Zone memory.
  // It assumes a minimum size blockmap.
  if (count < 5)
      I_Error("Missing blockmap, node builder has not been run.\n");

  // [WDJ] Do endian as read from blockmap lump temp
  blockmaphead = Z_Malloc(sizeof(*blockmaphead) * count, PU_LEVEL, NULL);

  // killough 3/1/98: Expand wad blockmap into larger internal one,
  // by treating all offsets except -1 as unsigned and zero-extending
  // them. This potentially doubles the size of blockmaps allowed,
  // because Doom originally considered the offsets as always signed.
  // [WDJ] They are unsigned in Unofficial Doom Spec.

  blockmaphead[0] = USHORT(wadblockmaplump[0]);	// map orgin_x
  blockmaphead[1] = USHORT(wadblockmaplump[1]);	// map orgin_y
  blockmaphead[2] = USHORT(wadblockmaplump[2]);	// number columns (x size)
  blockmaphead[3] = USHORT(wadblockmaplump[3]);	// number rows (y size)

  bmaporgx = blockmaphead[0] << FRACBITS;
  bmaporgy = blockmaphead[1] << FRACBITS;
  bmapwidth = blockmaphead[2];
  bmapheight = blockmaphead[3];
  blockmapindex = &blockmaphead[4];
  firstlist = 4 + bmapwidth * bmapheight;
  lastlist = count - 1;

#if 0
  printf ("firstlist = %u\n", firstlist);
  printf ("lastlist = %u\n", lastlist);
  printf ("bmapwidth = %u\n", bmapwidth);
  printf ("bmapheight = %u\n", bmapheight);
#endif

  if (firstlist >= lastlist || bmapwidth < 1 || bmapheight < 1)
    I_Error("Blockmap corrupt, must run node builder on wad.\n");

  // read blockmap index array
  for (i = 4; i < firstlist; i++)		// for all entries in wad offset index
  {
    uint32_t bme = USHORT(wadblockmaplump[i]);	// offset

						// upon overflow, the bme will wrap to low values
    if (bme < firstlist				// too small to be valid
     && bme < 0x1000 && prev_bme > 0xf000)	// wrapped
    {
      // first or repeated overflow
      overflow_corr += 0x00010000;
    }
    prev_bme = bme; // uncorrected

    // correct for overflow, or else try without correction
    if (overflow_corr)
    {
      uint32_t bmec = bme + overflow_corr;

      // First entry of list is 0, but high odds of hitting one randomly.
      // Check for valid blockmap offset, and offset overflow
      if (bmec <= lastlist
       && wadblockmaplump[bmec] == 0		// valid start list
       && bmec - blockmaphead[i - 1] < 1000)	// reasonably close sequentially
      {
	bme = bmec;
      }
    }

    if (bme > lastlist)
	I_Error("Blockmap offset[%i]= %i, exceeds bounds.\n", i, bme);
    if (bme < firstlist
     || wadblockmaplump[bme] != 0) // not start list
      I_Error("Bad blockmap offset[%i]= %i.\n", i, bme);
    blockmaphead[i] = bme;
  }

  // read blockmap lists
  for (i = firstlist; i < count; i++) // for all list entries in wad blockmap
  {
    // killough 3/1/98
    // keep -1 (0xffff), but other values are unsigned
    uint16_t bme = USHORT(wadblockmaplump[i]);

    blockmaphead[i] = (bme == 0xffff ? (uint32_t)(-1) : (uint32_t)bme);
  }

  // clear out mobj chains
  count = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = Z_Malloc(count, PU_LEVEL, NULL);
  memset(blocklinks, 0, count);
}


//-----------------------------------------------------------------------------
//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
// killough 5/3/98: reformatted, cleaned up
// cph 18/8/99: rewritten to avoid O(numlines * numsectors) section
// It makes things more complicated, but saves seconds on big levels
// figgi 09/18/00 -- adapted for gl-nodes

// cph - convenient sub-function
static void P_AddLineToSector (line_t *li, sector_t *sector)
{
    fixed_t *bbox = (void *)sector->blockbox;

    sector->lines[sector->linecount++] = li;
    M_AddToBox (bbox, li->v1->x, li->v1->y);
    M_AddToBox (bbox, li->v2->x, li->v2->y);
}

static void P_GroupLines (void)
{
    register line_t *li;
    register sector_t *sector;
    int i, j, total = numlines;

    // figgi
    for (i = 0; i < numsubsectors; i++)
    {
	seg_t *seg = &segs[subsectors[i].firstline];

	subsectors[i].sector = NULL;
	for (j = 0; j < subsectors[i].numlines; j++)
	{
	    if (seg->sidedef)
	    {
		subsectors[i].sector = seg->sidedef->sector;
		break;
	    }
	    seg++;
	}
	if (subsectors[i].sector == NULL)
	    I_Error ("P_GroupLines: Subsector a part of no sector!");
    }

    // count number of lines in each sector
    for (i = 0, li = lines; i < numlines; i++, li++)
    {
	if (!li->frontsector && li->backsector)
	{
	    // swap front and backsectors if a one-sided linedef
	    // does not have a front sector.
	    // HR2 map 32.
	    li->frontsector = li->backsector;
	    li->backsector = NULL;
	    li->flags &= ~ML_TWOSIDED;		// Just in case
	    // printf ("P_GroupLines: No front sector\n");
	}

	li->frontsector->linecount++;
	if (li->backsector && li->backsector != li->frontsector)
	{
	    li->backsector->linecount++;
	    total++;
	}
    }

    // allocate line tables for each sector
    {
	line_t **linebuffer = Z_Malloc (total * sizeof(line_t *), PU_LEVEL, 0);

	// e6y: REJECT overrun emulation code
	// moved to P_LoadReject
	for (i = 0, sector = sectors; i < numsectors; i++, sector++)
	{
	    sector->lines = linebuffer;
	    linebuffer += sector->linecount;
	    sector->linecount = 0;
	    M_ClearBox(sector->blockbox);
	}
    }

    // Enter those lines
    for (i = 0, li = lines; i < numlines; i++, li++)
    {
	P_AddLineToSector (li, li->frontsector);
	if (li->backsector && li->backsector != li->frontsector)
	    P_AddLineToSector (li, li->backsector);
    }

    for (i = 0, sector = sectors; i < numsectors; i++, sector++)
    {
	fixed_t *bbox = (void*)sector->blockbox; // cph - For convenience, so
	int block; // I can use the old code unchanged

	//e6y: fix sound origin for large levels
	sector->soundorg.x = bbox[BOXRIGHT] / 2 + bbox[BOXLEFT] / 2;
	sector->soundorg.y = bbox[BOXTOP] / 2 + bbox[BOXBOTTOM] / 2;

	// adjust bounding box to map blocks
	block = (bbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;
	block = (block >= bmapheight ? bmapheight - 1 : block);
	sector->blockbox[BOXTOP] = block;

	block = (bbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
	block = (block < 0 ? 0 : block);
	sector->blockbox[BOXBOTTOM] = block;

	block = (bbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
	block = (block >= bmapwidth ? bmapwidth - 1 : block);
	sector->blockbox[BOXRIGHT] = block;

	block = (bbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
	block = (block < 0 ? 0 : block);
	sector->blockbox[BOXLEFT] = block;
    }
}

//-----------------------------------------------------------------------------
//
// killough 10/98
//
// Remove slime trails.
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
// 2 2 2 2
// dx x0 + dy x1 + dx dy (y0 - y1) dy y0 + dx y1 + dx dy (x0 - x1)
// {---------------------------------, ---------------------------------}
// 2 2 2 2
// dx + dy dx + dy
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//

static void P_RemoveSlimeTrails (void)		// killough 10/98
{
  int i;
  byte *hit;					// Hitlist for vertices

  hit = (byte *) calloc (1, numvertexes);
  if (hit)
  {
    for (i = 0; i < numsegs; i++)		// Go through each seg
    {
      const line_t *l = segs[i].linedef;	// The parent linedef

      if (l->dx && l->dy)			// We can ignore orthogonal lines
      {
	vertex_t *v = segs[i].v1;

	do
	  if (!hit [v - vertexes])		// If we haven't processed vertex
	  {
	    hit [v - vertexes] = 1;		// Mark this vertex as processed

	    if (v != l->v1 && v != l->v2)	// Exclude endpoints of linedefs
	    {
#if 0
	      // Project the vertex back onto the parent linedef
	      int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
	      int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
	      int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
	      int64_t s = dx2 + dy2;
	      int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;

	      v->x = (int)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
	      v->y = (int)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
#else
	      // The Norcroft ARM compiler threw
	      // "Warning: Lower precision in wider context" for the above....
	      // JAD 6/5/14
	      int32_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
	      int32_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
	      int32_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
	      int64_t s = (int64_t) dx2 + (int64_t) dy2;
	      int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;

	      v->x = (int)(((int64_t)dx2 * (int64_t)x0 + (int64_t)dy2 * (int64_t)x1 + (int64_t)dxy * (int64_t)(y0 - y1)) / s);
	      v->y = (int)(((int64_t)dy2 * (int64_t)y0 + (int64_t)dx2 * (int64_t)y1 + (int64_t)dxy * (int64_t)(x0 - x1)) / s);
#endif
//	      printf ("%X,%X\n", v->x, v->y);
	    }
	  } // Obsfucated C contest entry: :)
	while ((v != segs[i].v2) && ((v = segs[i].v2) != NULL));
      }
    }
    free(hit);
  }
}

//-----------------------------------------------------------------------------
//
// P_SetupLevel
//
void
P_SetupLevel
( int		episode,
  int		map,
  int		playermask,
  skill_t	skill)
{
    int		i;
    int		lumpnum;
    unsigned int nomonsterbits;

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 150*(35*5);
    wminfo.timesucks = 60 * 50;
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
	players[i].killcount = players[i].secretcount
	    = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1;

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start ();


#if 0 // UNUSED
    if (debugfile)
    {
	Z_FreeTags (PU_LEVEL, MAXINT);
	Z_FileDumpHeap (debugfile);
    }
    else
#endif
	Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);

#ifdef USE_BOOM_P_ChangeSector
    headsecnode = NULL;
#endif

    // UNUSED W_Profile ();
    P_InitThinkers ();

    // if working with a development map, reload it
    W_Reload ();

    // find map name
    lumpnum = G_MapLump (episode, map);
    if (lumpnum == -1)
      I_Error ("P_SetupLevel: Map not found! (%u,%u)", episode, map);


    leveltime = 0;

    // note: most of this ordering is important
    P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);

    rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
    rejectmatrixsize = W_LumpLength (lumpnum+ML_REJECT);
    P_GroupLines ();
    P_RemoveSlimeTrails ();
    G_Patch_Map ();


    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    nomonsterbits = P_LoadThings (lumpnum+ML_THINGS);

    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
	for (i=0 ; i<MAXPLAYERS ; i++)
	    if (playeringame[i])
	    {
		players[i].mo = NULL;
		G_DeathMatchSpawnPlayer (i);
	    }

    }

    // clear special respawning que
    iquehead = iquetail = 0;

    // set up world state
    P_SpawnSpecials ();

    // build subsector connect matrix
    //	UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
	R_PrecacheLevel ();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

    if (nomonsterbits)				// If no monsters then
      A_Activate_Death_Sectors (nomonsterbits);	// action the sectors that would have been
						// actioned when they die.
}

//-----------------------------------------------------------------------------

#ifndef PADDED_STRUCTS
static void P_Check_Structs (void)
{
    if ((sizeof (mapseg_t)     != 12)
     || (sizeof (mapsidedef_t) != 30)
     || (sizeof (maplinedef_t) != 14)
     || (sizeof (mapsector_t)  != 26)
     || (sizeof (mapthing_t)   != 10))
    {
      I_Error ("Your compiler has padded structures. Please recompile with -DPADDED_STRUCTS");
    }
}
#endif

//-----------------------------------------------------------------------------
//
// P_Init
//
void P_Init (void)
{
#ifndef PADDED_STRUCTS
  P_Check_Structs ();
#endif
  P_InitSwitchList ();
  P_InitPicAnims ();
  R_InitSprites (sprnames);
  P_Init_Intercepts ();
  P_Init_SpecHit ();
}

//-----------------------------------------------------------------------------
