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


#include <math.h>
#include <stdlib.h>

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"

#if defined(LINUX) || defined(HAVE_ALLOCA)
#include  <alloca.h>
#else
extern void * alloca (unsigned int);
#endif

extern unsigned int P_SpawnMapThing (mapthing_t* mthing);
extern void A_Activate_Death_Sectors (unsigned int monsterbits);
extern void P_Init_Intercepts (void);
extern void P_Init_SpecHit (void);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int		numvertexes;
vertex_t*	vertexes;

int		numsegs;
seg_t*		segs;

int		numsectors;
sector_t*	sectors;

int		numsubsectors;
subsector_t*	subsectors;

int		numnodes;
node_t*		nodes;

int		numlines;
line_t*		lines;

int		numsides;
side_t*		sides;


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
short*		blockmap;	// int for larger maps
// offsets in blockmap are from here
short*		blockmaplump;
// origin of block map
fixed_t		bmaporgx;
fixed_t		bmaporgy;
// for thing chains
mobj_t**	blocklinks;


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



//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
    byte*		data;
    int			i;
    mapseg_t*		ml;
    seg_t*		li;
    line_t*		ldef;
    int			linedef;
    int			side;
    byte*		vertchanged;

#ifdef PADDED_STRUCTS
    numsegs = W_LumpLength (lump) / 12;
#else
    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
#endif
    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lump,PU_STATIC);

    i = (numvertexes+7)>>3;
    vertchanged = (byte *) alloca (i);
    if (vertchanged)
	memset(vertchanged, 0, i);

    ml = (mapseg_t *)data;
    li = segs;
    for (i=0 ; i<numsegs ; i++, li++)
    {
	int	ptp_angle;
	int	delta_angle;

	li->v1 = &vertexes[SHORT(ml->v1)];
	li->v2 = &vertexes[SHORT(ml->v2)];
	li->angle = (SHORT(ml->angle))<<16;

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


	li->offset = (SHORT(ml->offset))<<16;
	linedef = SHORT(ml->linedef);
	ldef = &lines[linedef];
	li->linedef = ldef;
	side = SHORT(ml->side);
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
	ss->numlines = SHORT(ms->numsegs);
	ss->firstline = SHORT(ms->firstseg);
    }

    Z_Free (data);
}



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
//	ss->touching_thinglist = NULL;
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


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
    byte*	data;
    int		i;
    int		j;
    int		k;
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
	    no->children[j] = SHORT(mn->children[j]);
	    for (k=0 ; k<4 ; k++)
		no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
	}
    }

    Z_Free (data);
}


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


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    byte*		data;
    int			i;
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
	ld->flags = SHORT(mld->flags);
	ld->special = SHORT(mld->special);
	ld->tag = SHORT(mld->tag);
	v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
	v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
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

	ld->sidenum[0] = SHORT(mld->sidenum[0]);
	ld->sidenum[1] = SHORT(mld->sidenum[1]);

	if (ld->sidenum[0] != -1)
	    ld->frontsector = sides[ld->sidenum[0]].sector;
	else
	    ld->frontsector = 0;

	if (ld->sidenum[1] != -1)
	    ld->backsector = sides[ld->sidenum[1]].sector;
	else
	    ld->backsector = 0;
#ifdef PADDED_STRUCTS
	mld = (maplinedef_t *) ((byte *) mld + 14);
#else
	mld++;
#endif

    }

    Z_Free (data);
}


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


//
// P_LoadBlockMap
//
void P_LoadBlockMap (int lump)
{
    int		i;
    int		count;

    blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
    blockmap = blockmaplump+4;
    count = W_LumpLength (lump)/2;

    for (i=0 ; i<count ; i++)
	blockmaplump[i] = SHORT(blockmaplump[i]);

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];

    // clear out mobj chains
    count = sizeof(*blocklinks)* bmapwidth*bmapheight;
    blocklinks = Z_Malloc (count,PU_LEVEL, 0);
    memset (blocklinks, 0, count);
}



//
/* In case something went wrong with the memory-requirements... */
static int P_TryGroupLines(int total)
{
    line_t**		linebuffer;
    line_t**            linebase;
    sector_t*		sector;
    fixed_t		bbox[4];
    int			block;
    int                 i, j;
    line_t*             li;

    /* build line tables for each sector        */
    linebase = Z_Malloc (total*sizeof(line_t*), PU_LEVEL, 0);
    linebuffer = linebase;

    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
	M_ClearBox (bbox);
	sector->lines = linebuffer;
	li = lines;
	for (j=0 ; j<numlines ; j++, li++)
	{
	    if ((li->frontsector == sector) || ((li->backsector == sector) && (li->backsector != li->frontsector)))
	    {
		if (linebuffer < linebase + total) *linebuffer = li;
		linebuffer++;
		M_AddToBox (bbox, li->v1->x, li->v1->y);
		M_AddToBox (bbox, li->v2->x, li->v2->y);
	    }
	}
	if (linebuffer - sector->lines != sector->linecount)
	{
	    fprintf(stderr, "P_GroupLines: miscounted by %d\n", (linebuffer - sector->lines) - sector->linecount);
	    sector->linecount = linebuffer - sector->lines;
	}
	/* set the degenmobj_t to the middle of the bounding box*/
	sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
	sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

	/* adjust bounding box to map blocks*/
	block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapheight ? bmapheight-1 : block;
	sector->blockbox[BOXTOP]=block;

	block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXBOTTOM]=block;

	block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block >= bmapwidth ? bmapwidth-1 : block;
	sector->blockbox[BOXRIGHT]=block;

	block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
	block = block < 0 ? 0 : block;
	sector->blockbox[BOXLEFT]=block;
    }

    if (linebuffer > linebase + total)
    {
	Z_Free(linebase);
	return (linebuffer - linebase);
    }
    return 0;
}

/* P_GroupLines*/
/* Builds sector line lists and subsector sector numbers.*/
/* Finds block bounding boxes for sectors.*/

static void P_GroupLines (void)
{
    int                 i;
    int                 total;
    line_t*             li;
    subsector_t*        ss;
    seg_t*              seg;

    /* look up sector number for each subsector*/
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
	seg = &segs[ss->firstline];
	ss->sector = seg->sidedef->sector;
    }

    /* count number of lines in each sector*/
    li = lines;
    total = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
//	if (li->frontsector != NULL)		// Removed By JAD as it causes off-by-one errors
	{					// in HR2 map 32.
	    /* This copied from Doom retro */
	    if (!li->frontsector && li->backsector)
	    {
		// swap front and backsectors if a one-sided linedef
		// does not have a front sector
		li->frontsector = li->backsector;
		li->backsector = NULL;
	    }

	    if (li->frontsector)		// And moved to here.
	    {
		li->frontsector->linecount++;
		total++;
	    }

	    if (li->backsector && li->backsector != li->frontsector)
	    {
		li->backsector->linecount++;
		total++;
	    }
	}
    }

    while (total != 0) total = P_TryGroupLines(total);
}




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



