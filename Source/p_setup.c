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

extern unsigned int P_SpawnMapThing (mapthing_t* mthing);
extern void A_Activate_Death_Sectors (unsigned int monsterbits);
extern void P_Init_Intercepts (void);
extern void P_Init_SpecHit (void);

#define NO_INDEX 0xFFFF
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
dmstart_t*	deathmatchstartlist;
mapthing_t	playerstarts[MAXPLAYERS];


//-----------------------------------------------------------------------------
/* ARM cannot read from non-word aligned locations! */

static unsigned int read_32 (const unsigned char * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr++ << 8);
  rc |= (*ptr++ << 16);
  rc |= (*ptr << 24);
  return (rc);
}

//-----------------------------------------------------------------------------

static unsigned int read_16 (const unsigned char * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr << 8);
  return (rc);
}

//-----------------------------------------------------------------------------
#if 0
static unsigned int read_s16 (const unsigned char * ptr)
{
  unsigned int rc;

  rc = *ptr++;
  rc |= (*ptr << 8);
  if (rc & 0x8000)
    rc |= 0xFFFF0000;
  return (rc);
}
#endif
//-----------------------------------------------------------------------------

typedef enum
{
  DOOMBSP = 0,
  DEEPBSP = 1,
  ZDBSPX  = 2,
  ZDBCPX  = 3
} mapformat_t;

static fixed_t GetOffset (vertex_t *v1, vertex_t *v2)
{
  fixed_t     dx = (v1->x - v2->x) >> FRACBITS;
  fixed_t     dy = (v1->y - v2->y) >> FRACBITS;
  return (((fixed_t)sqrt((double)dx*dx + (double)dy*dy)) << FRACBITS);
}

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
    W_ReleaseLumpNum (lump);
}

//-----------------------------------------------------------------------------
// cph 2006/09/30 - our frontsector can be the second side of the
// linedef, so must check for NO_INDEX in case we are incorrectly
// referencing the back of a 1S line

static void P_CheckSides (seg_t* li, line_t* ldef, int side)
{
  int s;

  s = ldef->sidenum[side];
  if ((s != NO_INDEX)
   && ((unsigned) s < numsides))
  {
    li->sidedef = &sides[s];
    li->frontsector = sides[s].sector;
  }
  else
  {
    printf ("The front of seg %u has no sidedef.\n", segs-li);
    li->sidedef = NULL;
    li->frontsector = NULL;
  }

  // killough 5/3/98: ignore 2s flag if second sidedef missing:
  if ((ldef->flags & ML_TWOSIDED)
   && ((s = ldef->sidenum[side ^ 1]) != NO_INDEX)
   && ((unsigned) s < numsides))
  {
    li->backsector = sides[s].sector;
  }
  else
  {
//  printf ("The back of seg %u has no sidedef.\n", segs-li);
    li->backsector = NULL;
    ldef->flags &= ~ML_TWOSIDED;
  }
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

#ifdef PADDED_STRUCTS
  numsegs = W_LumpLength (lump) / 12;
#else
  numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
#endif
  segs = Z_Calloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump,PU_STATIC);

  ml = (mapseg_t *)data;
  li = segs;
  for (i=0 ; i<numsegs ; i++, li++)
  {
    unsigned int v1,v2;

    li->angle = (SHORT(ml->angle))<<16;
    li->offset = (SHORT(ml->offset))<<16;
    linedef = USHORT(ml->linedef);
    if (linedef >= numlines)
    {
      printf ("P_LoadSegs: Seg %i linedef = %u/%u\n", i, linedef, numlines);
      linedef = 0;
    }
    ldef = &lines[linedef];
    li->linedef = ldef;
    side = USHORT(ml->side);
    if (side > 1)
    {
      printf ("P_LoadSegs: Seg %i side = %u\n", i, side);
      side = 0;
    }

    P_CheckSides (li, ldef, side);

    v1 = USHORT(ml->v1);
    v2 = USHORT(ml->v2);

    // e6y
    // check and fix wrong references to non-existent vertexes
    // see e1m9 @ NIVELES.WAD
    // http://www.doomworld.com/idgames/index.php?id=12647
    if ((v1 >= numvertexes) || (v2 >= numvertexes))
    {
      printf ("P_LoadSegs: Seg %i references invalid vertex %i,%i (%i).\n", i, v1, v2, numvertexes);

      if (li->sidedef == &sides[li->linedef->sidenum[0]])
      {
	li->v1 = lines[linedef].v1;
	li->v2 = lines[linedef].v2;
      }
      else
      {
	li->v1 = lines[linedef].v2;
	li->v2 = lines[linedef].v1;
      }
    }
    else
    {
      li->v1 = &vertexes[v1];
      li->v2 = &vertexes[v2];
    }

#ifdef PADDED_STRUCTS
    ml = (mapseg_t *) ((byte *) ml + 12);
#else
    ml++;
#endif
  }

  W_ReleaseLumpNum (lump);
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
    subsectors = Z_Calloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
    data = W_CacheLumpNum (lump,PU_STATIC);

    ms = (mapsubsector_t *)data;
    ss = subsectors;

    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
	ss->numlines = USHORT(ms->numsegs);
	ss->firstline = USHORT(ms->firstseg);
    }

    W_ReleaseLumpNum (lump);
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
    sectors = Z_Calloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
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
//	ss->nextsec = -1;
//	ss->prevsec = -1;
	ss->heightsec = -1;
	ss->floorlightsec = -1;
	ss->ceilinglightsec = -1;
#ifdef PADDED_STRUCTS
	ms = (mapsector_t *) ((byte *) ms + 26);
#else
	ms++;
#endif
    }

    W_ReleaseLumpNum (lump);
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

    W_ReleaseLumpNum (lump);
}

//-----------------------------------------------------------------------------

static byte * P_LoadZSegs (const byte *data)
{
  int i;
  seg_t		*li = segs;
  const mapseg_znod_t	*ml = (const mapseg_znod_t *)data;

  for (i = 0; i < numsegs; i++)
  {
    line_t  		*ldef;
    unsigned int	v1, v2;
    unsigned int	linedef;
    unsigned char	side;

    v1 = read_32 ((unsigned char*) &ml->v1);
    v2 = read_32 ((unsigned char*) &ml->v2);

    linedef = read_16 ((unsigned char*) &ml->linedef);

    // e6y: check for wrong indexes
    if ((unsigned int)linedef >= (unsigned int)numlines)
    {
      I_Error("P_LoadZSegs: seg %d references a non-existent linedef %d",
	  i, (unsigned int)linedef);
    }

    ldef = &lines[linedef];
    li->linedef = ldef;
    side = ml->side;

    // e6y: fix wrong side index
    if (side != 0 && side != 1)
    {
      printf ("P_LoadZSegs: seg %d contains wrong side index %d. Replaced with 1.",
	  i, side);
      side = 1;
    }

    // e6y: check for wrong indexes
    if ((unsigned int)ldef->sidenum[side] >= (unsigned int)numsides)
    {
      I_Error("P_LoadZSegs: linedef %d for seg %d references a non-existent sidedef %d",
	  linedef, i, (unsigned int)ldef->sidenum[side]);
    }

    P_CheckSides (li, ldef, side);

    li->v1 = &vertexes[v1];
    li->v2 = &vertexes[v2];

    li->offset = GetOffset(li->v1, (side ? ldef->v2 : ldef->v1));
    li->angle = R_PointToAngle2(segs[i].v1->x, segs[i].v1->y, segs[i].v2->x, segs[i].v2->y);

    ml = (const mapseg_znod_t *) ((byte *) ml + 11);
    li++;
  }

  return ((byte *) ml);
}

//-----------------------------------------------------------------------------

static void P_LoadZNodes (int lump)
{
  byte			*data;
  byte			*zndata;
  unsigned int		i;

  unsigned int		orgVerts, newVerts;
  unsigned int		numSubs, currSeg;
  unsigned int		numSegs;
  unsigned int		numNodes;
  vertex_t		*newvertarray = NULL;
  const mapsubsector_znod_t	*mseg;
  node_t  		*no;
  const mapnode_znod_t	*mn;



  zndata = W_CacheLumpNum(lump, PU_LEVEL);

  // skip header
  data = zndata + 4;

  // Read extra vertices added during node building
  orgVerts = read_32 (data);
  data += sizeof(orgVerts);

  newVerts = read_32 (data);
  data += sizeof(newVerts);

  if (orgVerts + newVerts == (unsigned int)numvertexes)
  {
    newvertarray = vertexes;
  }
  else
  {
    newvertarray = Z_Calloc ((orgVerts + newVerts) * sizeof(vertex_t),PU_LEVEL,0);
    memcpy (newvertarray, vertexes, orgVerts * sizeof(vertex_t));
  }

  for (i = 0; i < newVerts; i++)
  {
    newvertarray[i + orgVerts].x = read_32 (data);
    data += sizeof(newvertarray[0].x);

    newvertarray[i + orgVerts].y = read_32 (data);
    data += sizeof(newvertarray[0].y);
  }

  if (vertexes != newvertarray)
  {
    for (i = 0; i < (unsigned int)numlines; i++)
    {
      lines[i].v1 = lines[i].v1 - vertexes + newvertarray;
      lines[i].v2 = lines[i].v2 - vertexes + newvertarray;
    }
    Z_Free (vertexes);
    vertexes = newvertarray;
    numvertexes = orgVerts + newVerts;
  }

  // Read the subsectors
  numSubs = read_32 (data);
  data += sizeof(numSubs);

  numsubsectors = numSubs;
  if (numsubsectors == 0)
      I_Error("P_LoadZNodes: no subsectors in level");
  subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);

  mseg = (const mapsubsector_znod_t *)data;
  for (i = currSeg = 0; i < numSubs; i++)
  {
    subsectors[i].firstline = currSeg;
    numSegs = read_32((unsigned char *) &mseg->numsegs);
    subsectors[i].numlines = numSegs;
    currSeg += numSegs;
    mseg++;
  }
  data += numSubs * sizeof(mapsubsector_znod_t);

  // Read the segs
  numSegs = read_32 (data);
  data += sizeof(numSegs);

  // The number of segs stored should match the number of
  // segs used by subsectors.
  if (numSegs != currSeg)
    I_Error("P_LoadZNodes: Incorrect number of segs in nodes. (%u/%u", numSegs, currSeg);

  numsegs = numSegs;
  segs = Z_Calloc (numsegs*sizeof(seg_t),PU_LEVEL,0);

  data = P_LoadZSegs (data);

  // Read nodes
  numNodes = read_32 (data);
  data += sizeof(numNodes);

  numnodes = numNodes;
  nodes = Z_Calloc (numNodes*sizeof(node_t),PU_LEVEL,0);

  no = nodes;
  mn = (const mapnode_znod_t *)data;

  for (i = 0; i < numNodes; i++)
  {
    int	j, k;

    no->x = read_16((unsigned char *) &mn->x) << FRACBITS;
    no->y = read_16((unsigned char *) &mn->y) << FRACBITS;
    no->dx = read_16((unsigned char *) &mn->dx) << FRACBITS;
    no->dy = read_16((unsigned char *) &mn->dy) << FRACBITS;

    for (j = 0; j < 2; j++)
    {
      no->children[j] = read_32((unsigned char *) &mn->children[j]);

      for (k = 0; k < 4; k++)
	no->bbox[j][k] = read_16((unsigned char *) &mn->bbox[j][k]) << FRACBITS;
    }
    mn++;
    no++;
  }

  W_ReleaseLumpNum (lump);
}

//-----------------------------------------------------------------------------

static void P_LoadSegs_V4 (int lump)
{
  int  i;
  const mapseg_v4_t *data;
  seg_t *li;
  const mapseg_v4_t *ml;

#ifdef PADDED_STRUCTS
  numsegs = W_LumpLength (lump) / 16;
#else
  numsegs = W_LumpLength (lump) / sizeof(mapseg_v4_t);
#endif
  segs = Z_Calloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
  data = (const mapseg_v4_t *) W_CacheLumpNum (lump,PU_STATIC);

  if ((!data) || (!numsegs))
    I_Error("P_LoadSegs_V4: no segs in level");

  ml = data;
  li = segs;
  for (i = 0; i < numsegs; i++)
  {
    int v1, v2;
    int side, linedef;
    line_t *ldef;

//  li->miniseg = false; // figgi -- there are no minisegs in classic BSP nodes

    li->angle  = read_16 ((unsigned char *) &ml->angle)<<FRACBITS;
    li->offset = read_16 ((unsigned char *) &ml->offset)<<FRACBITS;
    linedef = read_16 ((unsigned char *) &ml->linedef);

    //e6y: check for wrong indexes
    if ((unsigned)linedef >= (unsigned)numlines)
    {
      I_Error("P_LoadSegs_V4: seg %d references a non-existent linedef %d",
	i, (unsigned)linedef);
    }

    ldef = &lines[linedef];
    li->linedef = ldef;
    side = read_16 ((unsigned char *) &ml->side);

    //e6y: fix wrong side index
    if (side != 0 && side != 1)
    {
      printf ("P_LoadSegs_V4: seg %d contains wrong side index %d. Replaced with 1.\n", i, side);
      side = 1;
    }

    //e6y: check for wrong indexes
    if ((unsigned)ldef->sidenum[side] >= (unsigned)numsides)
    {
      I_Error("P_LoadSegs_V4: linedef %d for seg %d references a non-existent sidedef %d",
	linedef, i, (unsigned)ldef->sidenum[side]);
    }

    P_CheckSides (li, ldef, side);

    v1 = read_32((unsigned char *) &ml->v1);
    v2 = read_32((unsigned char *) &ml->v2);

    // e6y
    // check and fix wrong references to non-existent vertexes
    // see e1m9 @ NIVELES.WAD
    // http://www.doomworld.com/idgames/index.php?id=12647
    if ((v1 >= numvertexes) || (v2 >= numvertexes))
    {
      printf ("P_LoadSegs: Seg %i references invalid vertex %i,%i (%i).\n", i, v1, v2, numvertexes);

      if (li->sidedef == &sides[li->linedef->sidenum[0]])
      {
	li->v1 = lines[linedef].v1;
	li->v2 = lines[linedef].v2;
      }
      else
      {
	li->v1 = lines[linedef].v2;
	li->v2 = lines[linedef].v1;
      }
    }
    else
    {
      li->v1 = &vertexes[v1];
      li->v2 = &vertexes[v2];
    }

    // Recalculate seg offsets that are sometimes incorrect
    // with certain nodebuilders. Fixes among others, line 20365
    // of DV.wad, map 5
    li->offset = GetOffset(li->v1, (side ? ldef->v2 : ldef->v1));

    li++;
    ml++;
  }

  W_ReleaseLumpNum (lump);	// cph - release the data
}

//-----------------------------------------------------------------------------

static void P_LoadSubsectors_V4 (int lump)
{
  /* cph 2006/07/29 - make data a const mapsubsector_t *, so the loop below is simpler & gives no constness warnings */
  int i;
  const mapsubsector_v4_t *data;
  const mapsubsector_v4_t * ms;
  subsector_t*	ss;

  numsubsectors = W_LumpLength (lump) / 6 /*sizeof(mapsubsector_v4_t)*/;

  subsectors = Z_Calloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
  data = (const mapsubsector_v4_t *) W_CacheLumpNum (lump,PU_STATIC);

  if ((!data) || (!numsubsectors))
    I_Error("P_LoadSubsectors_V4: no subsectors in level");

  ms = data;
  ss = subsectors;
  for (i = 0; i < numsubsectors; i++)
  {
    ss->numlines = read_16 (ms->numsegs);
    ss->firstline = read_32 (ms->firstseg);
    ms = (const mapsubsector_v4_t *) ((byte *) ms + 6);
    ss++;
  }

  W_ReleaseLumpNum (lump);	// cph - release the data
}

//-----------------------------------------------------------------------------

static void P_LoadNodes_V4 (int lump)
{
  int  i;
  const byte *data; // cph - const*
  node_t *no;
  const mapnode_v4_t *mn;

  numnodes = (W_LumpLength (lump) - 8) / sizeof(mapnode_v4_t);
  nodes = Z_Calloc (numnodes * sizeof(node_t),PU_LEVEL,0);
  data = W_CacheLumpNum (lump,PU_STATIC); // cph - wad lump handling updated

  if ((!data) || (!numnodes))
  {
    // allow trivial maps
    if (numsubsectors == 1)
      printf ("P_LoadNodes_V4: trivial map (no nodes, one subsector)\n");
    else
      I_Error("P_LoadNodes_V4: no nodes in level");
  }

  no = nodes;
  mn = (const mapnode_v4_t *) (data + 8);	// skip header
  for (i = 0; i < numnodes; i++)
  {
    int j;

    no->x = read_16 ((unsigned char*) &mn->x)<<FRACBITS;
    no->y = read_16 ((unsigned char*) &mn->y)<<FRACBITS;
    no->dx = read_16 ((unsigned char*) &mn->dx)<<FRACBITS;
    no->dy = read_16 ((unsigned char*) &mn->dy)<<FRACBITS;

    for (j=0 ; j<2 ; j++)
    {
      int k;
      no->children[j] = read_32 ((unsigned char*) &mn->children[j]);

      for (k=0 ; k<4 ; k++)
	no->bbox[j][k] = read_16 ((unsigned char*) &mn->bbox[j][k])<<FRACBITS;
    }
    no++;
    mn++;
  }

  W_ReleaseLumpNum (lump);	// cph - release the data
}

//-----------------------------------------------------------------------------
//
// P_LoadThings
//
static unsigned int P_LoadThings (int lump)
{
    byte*		data;
    int			i;
    int			type;
    mapthing_t*		mt;
    mapthing_t		mtl;
    int			numthings;
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
	mtl.x = SHORT(mt->x);
	mtl.y = SHORT(mt->y);
	mtl.angle = SHORT(mt->angle);
	mtl.type = SHORT(mt->type);
	mtl.options = SHORT(mt->options);

	G_Patch_Map_Things (i, &mtl);

	// Do not spawn cool, new monsters if !commercial
	if ((gamemode == commercial)
	 || (((type = mtl.type) != 68)	// Arachnotron
	  && (type != 64)		// Archvile
	  && (type != 88)		// Boss Brain
	  && (type != 89)		// Boss Shooter
	  && (type != 69)		// Hell Knight
	  && (type != 67)		// Mancubus
	  && (type != 71)		// Pain Elemental
	  && (type != 65)		// Former Human Commando
	  && (type != 66)		// Revenant
	  && (type != 84)))		// Wolf SS
	{
	  // Do spawn all other stuff.

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

    W_ReleaseLumpNum (lump);

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
    lines = Z_Calloc (numlines*sizeof(line_t),PU_LEVEL,0);
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

	ld->tranlump = -1;	// killough 4/11/98: no translucency by default

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
	if (sidenum == NO_INDEX)
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
	if (sidenum == NO_INDEX)
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

    W_ReleaseLumpNum (lump);
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
    sides = Z_Calloc (numsides*sizeof(side_t),PU_LEVEL,0);
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

    W_ReleaseLumpNum (lump);
}

//-----------------------------------------------------------------------------
//
// P_VerifyBlockMap
//
// haleyjd 03/04/10: do verification on validity of blockmap.
//
static int P_VerifyBlockMap (unsigned int count)
{
  int      x, y;
  uint32_t *maxoffs = blockmaphead + count;

  for (y = 0; y < bmapheight; y++)
  {
    for (x = 0; x < bmapwidth; x++)
    {
      uint32_t offset;
      uint32_t block;
      uint32_t *list;
      uint32_t *blockoffset;

      offset = (y * bmapwidth) + x;
      blockoffset = blockmaphead + offset + 4;

      // check that block offset is in bounds
      if (blockoffset >= maxoffs)
	return (0);

      offset = *blockoffset;
      list = blockmaphead + offset;

      // scan forward for a -1 terminator before maxoffs
      do
      {
	// we have overflowed the lump?
	if (list >= maxoffs)
	  return (0);

	if (((int32_t) (block = *list)) == -1) // found -1
	  break;

	if (block >= numlines)
	  return (0);

	list++;
      } while (1);
    }
  }

  return (1);
}

//-----------------------------------------------------------------------------
//
// P_LoadBlockMap
//
static int P_LoadBlockMap (int lump)
{
  unsigned int i;
  unsigned int count;		// number of 16 bit blockmap entries
  unsigned int blockmapsize;
  uint16_t *wadblockmaplump;	// blockmap lump temp
  uint32_t firstlist, lastlist;	// blockmap block list bounds
  uint32_t overflow_corr;

  blockmapsize = W_LumpLength (lump);
//printf ("blockmap size = %u (0x%X) %u/%u\n", blockmapsize, blockmapsize, lump, numlumps);

  if (blockmapsize < 8)
  {
#ifdef NORMALUNIX
    printf ("Blockmap is too small (%u)\n", blockmapsize);
#endif
    return (1);
  }

  count = blockmapsize / 2;
  wadblockmaplump = W_CacheLumpNum(lump, PU_LEVEL);

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
  firstlist = 4 + bmapwidth * bmapheight;
  lastlist = count - 1;

#if 0
  printf ("firstlist = %u\n", firstlist);
  printf ("lastlist = %u\n", lastlist);
  printf ("bmapwidth = %u\n", bmapwidth);
  printf ("bmapheight = %u\n", bmapheight);
#endif

#ifdef NORMALUNIX
  if ((lastlist - firstlist) > 0x10000)
    printf ("Very large blockmap %X\n", lastlist - firstlist);
  else if (lastlist >= 0x10000)
    printf ("Large blockmap %X\n", lastlist);
#endif

  if (firstlist >= lastlist || bmapwidth < 1 || bmapheight < 1)
  {
//  I_Error("Blockmap corrupt, must run node builder on wad.\n");
    Z_Free (blockmaphead);
    Z_Free (wadblockmaplump);
    return (1);
  }

  overflow_corr = 0;

  // read blockmap index array
  for (i = 4; i < firstlist; i++)		// for all entries in wad offset index
  {
    uint32_t bme = USHORT(wadblockmaplump[i]);	// offset

    bme += overflow_corr;

    if (bme > lastlist)
    {
      if (overflow_corr)
      {
	overflow_corr -= 0x10000;
	bme -= 0x10000;
      }
    }
    else if (bme < firstlist)
    {
      overflow_corr += 0x10000;
      bme += 0x10000;
    }

    if (((lastlist - firstlist) > 0x10000)
     && (wadblockmaplump [bme] != 0))
    {
      uint32_t bmec = bme & 0xFFFF;
      do
      {
	if (bmec >= firstlist)
	{
	  if (wadblockmaplump [bmec] == 0)
	  {
	    bme = bmec;
	    overflow_corr = bme & ~0xFFFF;
	    break;
	  }
	}
	bmec += 0x10000;
      } while (bmec <= lastlist);
    }

    if ((bme > lastlist)
     || (bme < firstlist))
    {
#ifdef NORMALUNIX
      printf ("Invalid Blockmap offset[%u]= %u,%u (%u-%u)\n", i, bme, overflow_corr, firstlist, lastlist);
#endif
      Z_Free (blockmaphead);
      Z_Free (wadblockmaplump);
      return (1);
    }

#if 0
    if (wadblockmaplump [bme])
      printf ("Blockmap[0x%X] starts with 0x%X\n", bme, USHORT(wadblockmaplump [bme]));
#endif

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

  Z_Free (wadblockmaplump);

  if (!P_VerifyBlockMap (count))
  {
#ifdef NORMALUNIX
    printf ("Invalid Blockmap\n");
#endif
    Z_Free (blockmaphead);
    return (1);
  }

  return (0);			// All ok!
}

//-----------------------------------------------------------------------------
//
// killough 10/98:
//
// Rewritten to use faster algorithm.
//
// New procedure uses Bresenham-like algorithm on the linedefs, adding the
// linedef to each block visited from the beginning to the end of the linedef.
//
// The algorithm's complexity is on the order of nlines*total_linedef_length.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
static void P_CreateBlockMap (void)
{
  int		i;
  fixed_t	minx = MAXINT;
  fixed_t	miny = MAXINT;
  fixed_t	maxx = MININT;
  fixed_t	maxy = MININT;
  vertex_t*	vertex;
  fixed_t	j;

#ifdef NORMALUNIX
  printf ("Creating new blockmap\n");
#endif

  // First find limits of map
  vertex = vertexes;
  i = numvertexes;
  do
  {
    j = vertex->x >> FRACBITS;
    if (j < minx) minx = j;
    if (j > maxx) maxx = j;
    j = vertex->y >> FRACBITS;
    if (j < miny) miny = j;
    if (j > maxy) maxy = j;
    vertex++;
  } while (--i);

  // Save blockmap parameters
  bmaporgx = minx << FRACBITS;
  bmaporgy = miny << FRACBITS;
  bmapwidth = ((maxx - minx) >> MAPBTOFRAC) + 1;
  bmapheight = ((maxy - miny) >> MAPBTOFRAC) + 1;

  // Compute blockmap, which is stored as a 2d array of variable-sized lists.
  //
  // Pseudocode:
  //
  // For each linedef:
  //
  //   Map the starting and ending vertices to blocks.
  //
  //   Starting in the starting vertex's block, do:
  //
  //     Add linedef to current block's list, dynamically resizing it.
  //
  //     If current block is the same as the ending vertex's block, exit loop.
  //
  //     Move to an adjacent block by moving towards the ending block in
  //     either the x or y direction, to the block which contains the linedef.
  {
    // blocklist structure
    typedef struct
    {
      int n, nalloc, *list;
    } bmap_t;

    unsigned int    tot = bmapwidth * bmapheight;	// size of blockmap
    bmap_t	    *bmap = calloc(sizeof(*bmap), tot);	// array of blocklists

    if (bmap == NULL)
      I_Error ("P_CreateBlockMap: Unable to claim bmap\n");

    for (i = 0; i < numlines; i++)
    {
      // starting coordinates
      int x = (lines[i].v1->x >> FRACBITS) - minx;
      int y = (lines[i].v1->y >> FRACBITS) - miny;

      // x - y deltas
      int adx = lines[i].dx >> FRACBITS;
      int dx = (adx < 0 ? -1 : 1);
      int ady = lines[i].dy >> FRACBITS;
      int dy = (ady < 0 ? -1 : 1);

      // difference in preferring to move across y (>0) instead of x (<0)
      int diff = !adx ? 1 : !ady ? -1 :
	  (((x >> MAPBTOFRAC) << MAPBTOFRAC)
	  + (dx > 0 ? MAPBLOCKUNITS - 1 : 0) - x) * (ady = abs(ady)) * dx
	  - (((y >> MAPBTOFRAC) << MAPBTOFRAC)
	  + (dy > 0 ? MAPBLOCKUNITS - 1 : 0) - y) * (adx = abs(adx)) * dy;

      // starting block, and pointer to its blocklist structure
      int b = (y >> MAPBTOFRAC) * bmapwidth + (x >> MAPBTOFRAC);

      // ending block
      int bend = (((lines[i].v2->y >> FRACBITS) - miny) >> MAPBTOFRAC) * bmapwidth
	  + (((lines[i].v2->x >> FRACBITS) - minx) >> MAPBTOFRAC);

      // delta for pointer when moving across y
      dy *= bmapwidth;

      // deltas for diff inside the loop
      adx <<= MAPBTOFRAC;
      ady <<= MAPBTOFRAC;

      // Now we simply iterate block-by-block until we reach the end block.
      while ((unsigned int)b < tot)       // failsafe -- should ALWAYS be true
      {
	bmap_t * bp = &bmap[b];

	// Increase size of allocated list if necessary
	if ((bp->n >= bp->nalloc)
	 && ((bp->list = realloc(bp->list, (bp->nalloc = bp->nalloc ?
	  bp->nalloc * 2 : 8)*sizeof(*bp->list))) == NULL))
	    I_Error ("P_CreateBlockMap: Unable to claim bmap\n");

	// Add linedef to end of list
	bp->list[bp->n++] = i;

	// If we have reached the last block, exit
	if (b == bend)
	  break;

	// Move in either the x or y direction to the next block
	if (diff < 0)
	{
	  diff += ady;
	  b += dx;
	}
	else
	{
	  diff -= adx;
	  b += dy;
	}
      }
    }

    // Compute the total size of the blockmap.
    //
    // Compression of empty blocks is performed by reserving two offset words
    // at tot and tot+1.
    //
    // 4 words, unused if this routine is called, are reserved at the start.
    {
      int count = tot + 6;  // we need at least 1 word per block, plus reserved's

      for (i = 0; (unsigned int)i < tot; i++)
      {
	if (bmap[i].n)
	  count += bmap[i].n + 2;     // 1 header word + 1 trailer word + blocklist
      }

      // Allocate blockmap lump with computed count
      blockmaphead = Z_Malloc(sizeof(*blockmaphead) * count, PU_LEVEL, 0);
    }

    // Now compress the blockmap.
    {
      int	ndx = tot += 4; // Advance index to start of linedef lists
      bmap_t	*bp = bmap;     // Start of uncompressed blockmap

      blockmaphead[ndx++] = 0;    // Store an empty blockmap list at start
      blockmaphead[ndx++] = -1;   // (Used for compression)

      for (i = 4; (unsigned int)i < tot; i++, bp++)
      {
	if (bp->n)					// Non-empty blocklist
	{
	  blockmaphead[blockmaphead[i] = ndx++] = 0;	// Store index & header
	  do
	    blockmaphead[ndx++] = bp->list[--bp->n];	// Copy linedef list
	  while (bp->n);
	  blockmaphead[ndx++] = -1;			// Store trailer
	  free(bp->list);				// Free linedef list
	}
	else
	{
	  // Empty blocklist: point to reserved empty blocklist
	  blockmaphead[i] = tot;
	}
      }

      free (bmap);					// Free uncompressed blockmap
    }
  }
}

//-----------------------------------------------------------------------------

static void P_RemoveBlockMapheader (void)
{
  uint32_t i;
  uint32_t bme;
  uint32_t firstlist;

  firstlist = 4 + bmapwidth * bmapheight;

  // read blockmap index array
  for (i = 4; i < firstlist; i++)		// for all entries in wad offset index
  {
    bme = blockmaphead [i];
    if (blockmaphead [bme])
      return;
  }

  /* So all of the offsets start with a zero... */
  for (i = 4; i < firstlist; i++)
  {
    blockmaphead [i]++;
  }
}

//-----------------------------------------------------------------------------

static void P_ClearMobjChains (void)
{
  unsigned int count;

  // clear out mobj chains
  count = sizeof(*blocklinks) * bmapwidth * bmapheight;
  blocklinks = Z_Malloc (count, PU_LEVEL, NULL);
  memset (blocklinks, 0, count);
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

	if (li->frontsector)
	{
	  li->frontsector->linecount++;
	  if (li->backsector && li->backsector != li->frontsector)
	  {
	      li->backsector->linecount++;
	      total++;
	  }
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
      if (li->frontsector)
      {
	P_AddLineToSector (li, li->frontsector);
	if (li->backsector && li->backsector != li->frontsector)
	    P_AddLineToSector (li, li->backsector);
      }
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
	      // [crispy] wait a minute... moved more than 8 map units?
	      // maybe that's a linguortal then, back to the original coordinates
	      if ((ABS(v->x - x0) > (8 * FRACUNIT)) || (ABS(v->y - y0) > (8 * FRACUNIT)))
	      {
		v->x = x0;
		v->y = y0;
	      }

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

void R_CalcSegsLength (void)
{
  int i;

  for (i = 0; i < numsegs; i++)
  {
    seg_t   *li = segs + i;
    int64_t dx = (int64_t)li->v2->x - li->v1->x;
    int64_t dy = (int64_t)li->v2->y - li->v1->y;

    li->length = (uint32_t)(sqrt((double)dx * dx + (double)dy * dy) / 2);

    // [crispy] re-calculate angle used for rendering
    li->angle = R_PointToAngleEx2(li->v1->x, li->v1->y, li->v2->x, li->v2->y);
  }
}

//-----------------------------------------------------------------------------
// [crispy] support maps with NODES in compressed or uncompressed ZDBSP
// format or DeePBSP format and/or LINEDEFS and THINGS lumps in Hexen format
static mapformat_t P_CheckMapFormat (int lumpnum)
{
  mapformat_t 	format = DOOMBSP;
  byte		*nodes;
  int	 	b;

  b = lumpnum + ML_NODES;
  if (b < numlumps)
  {
    nodes = W_CacheLumpNum (b, PU_CACHE);
    if (nodes)
    {
      if (W_LumpLength (b) != 0)
      {
	if (!memcmp(nodes, "xNd4\0\0\0\0", 8))
	{
#ifdef NORMALUNIX
	  printf ("This map has DeePBSP v4 Extended nodes.\n");
#endif
	  format = DEEPBSP;
	}
	else if (!memcmp(nodes, "XNOD", 4))
	{
#ifdef NORMALUNIX
	  printf ("This map has ZDoom uncompressed normal nodes.\n");
#endif
	  format = ZDBSPX;
	}
	else if (!memcmp(nodes, "ZNOD", 4))
	{
#ifdef NORMALUNIX
	  printf ("This map has Compressed ZDoom nodes.\n");
#endif
	  format = ZDBCPX;
	}
      }
      W_ReleaseLumpNum (b);
    }
  }

//printf ("P_CheckMapFormat = %u\n", (int) format);
  return (format);
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
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);
    P_LoadLineDefs (lumpnum+ML_LINEDEFS);
    if (P_LoadBlockMap (lumpnum+ML_BLOCKMAP))
      P_CreateBlockMap ();
    P_RemoveBlockMapheader ();
    P_ClearMobjChains ();


    switch ((int) P_CheckMapFormat (lumpnum))
    {
      case DOOMBSP:
	P_LoadSubsectors (lumpnum+ML_SSECTORS);
	P_LoadNodes (lumpnum+ML_NODES);
	P_LoadSegs (lumpnum+ML_SEGS);
	break;

      case ZDBSPX:
	P_LoadZNodes (lumpnum + ML_NODES);
	break;

      case DEEPBSP:
	P_LoadSubsectors_V4 (lumpnum+ML_SSECTORS);
	P_LoadNodes_V4 (lumpnum+ML_NODES);
	P_LoadSegs_V4 (lumpnum+ML_SEGS);
	break;

      default: // ZDBCPX
	I_Error ("Compressed ZDoom maps not supported\n");
    }

    rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
    rejectmatrixsize = W_LumpLength (lumpnum+ML_REJECT);
    P_GroupLines ();
    P_RemoveSlimeTrails ();
    G_Patch_Map ();
    R_CalcSegsLength ();


    bodyqueslot = 0;
    deathmatchstartlist = NULL;
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
  R_InitSprites ();
  P_Init_Intercepts ();
  P_Init_SpecHit ();
}

//-----------------------------------------------------------------------------
