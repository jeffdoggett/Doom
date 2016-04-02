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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


#if 0
static const char rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";
#endif

#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#endif

#if defined(LINUX) || defined(HAVE_ALLOCA)
#include <alloca.h>
#else
extern void * alloca (unsigned int);
#endif

#ifdef __riscos
#include <ctype.h>
#include "acorn.h"
#endif

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"
#include "doomstat.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"

#include <sys/stat.h>

/* ---------------------------------------------------------------------------- */
//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;
int			numlumps;

void**			lumpcache;
static int*		sortedlumps;
static int		duplicates_removed = 0;

/* ---------------------------------------------------------------------------- */

unsigned int filesize (const char * path)
{
  struct stat fileinfo;

  if (stat (path,&fileinfo))
    return (0);

  return (unsigned int)(fileinfo.st_size);
}

/* ---------------------------------------------------------------------------- */

/* New functions to sort the lumpinfo and eliminate duplicates */

#define SWAP_LUMPS(x, y) \
    aux = sortedlumps[x]; sortedlumps[x] = sortedlumps[y]; sortedlumps[y] = aux;

/* Linearized quicksort */
void W_QuicksortLumps (int from, int to)
{
    int		from_stack[32];
    int		to_stack[32];
    int		stackitems = 0;

    do
    {
	while (from < to)
	{
	    int i, j, name0, name1;
	    int aux;

	    j = (from + to) / 2;
	    SWAP_LUMPS(from, j);
	    j = from;
	    name0 = ((int*)lumpinfo[sortedlumps[from]].name)[0];
	    name1 = ((int*)lumpinfo[sortedlumps[from]].name)[1];
	    for (i=from+1; i<=to; i++)
	    {
		int test0 = ((int*)lumpinfo[sortedlumps[i]].name)[0];
		int test1 = ((int*)lumpinfo[sortedlumps[i]].name)[1];
		if (test0 < name0 ||
		    (test0 == name0 &&
		     (test1 < name1)))
		{
		    j++;
		    SWAP_LUMPS(i, j);
		}
	    }
	    SWAP_LUMPS(from, j);

	    if ((j - from) < (to - j))
	    {
		from_stack[stackitems] = j+1;
		to_stack[stackitems] = to;
		to = j-1;
	    }
	    else
	    {
		from_stack[stackitems] = from;
		to_stack[stackitems] = j-1;
		from = j+1;
	    }
	    stackitems++;
	    if (stackitems >= 32)
	    {
		I_Error("W_QuicksortLumps: fatal overflow!");
	    }
	}
	if (--stackitems >= 0)
	{
	    from = from_stack[stackitems];
	    to = to_stack[stackitems];
	}
    }
    while (stackitems >= 0);
#if 0
  {
    unsigned int pos;
    lumpinfo_t* lump_p;
    char name [10];

    pos = 0;
    do
    {
      lump_p = &lumpinfo[sortedlumps[pos]];
      strncpy (name, lump_p->name,8);
      name [8] = 0;
      printf ("pos %u is lump %u (%s)\n", pos, sortedlumps[pos], name);
    } while (++pos < numlumps);
  }
#endif
}

/* ---------------------------------------------------------------------------- */
//
// Remove duplicate entity names when using pwads
// Fortunately the common map sections (THINGS, LINEDEFS, etc) are
// never searched for so it doesn't matter if I damage them.
//
void W_RemoveDuplicates (void)
{
  unsigned int count_1;
  unsigned int count_2;
  unsigned int w1,w2;
  lumpinfo_t* lump_p1;
  lumpinfo_t* lump_p2;

  // printf ("W_RemoveDuplicates (%u)\n", numlumps);
  lump_p1 = lumpinfo;
  count_1 = numlumps - 1;
  do
  {
    w1 = ((int*)lump_p1 -> name)[0];
    w2 = ((int*)lump_p1 -> name)[1];
    lump_p2 = lump_p1 + 1;
    count_2 = count_1;
    // printf ("Doing %u of %u\n", count_2, numlumps);
    do
    {
      if ((w1 == ((int*)lump_p2 -> name)[0])
       && (w2 == ((int*)lump_p2 -> name)[1]))
      {
	// printf ("Removed (%s)(%s) %X %X\n", lump_p1 -> name, lump_p2 -> name, lump_p1, lump_p2);
	memset (lump_p1 -> name, 0, sizeof (lump_p1 -> name));	// Duplicate found - remove it.
	break;
      }
      lump_p2++;
    } while (--count_2);
    lump_p1++;
  } while (--count_1);

  // printf ("W_RemoveDuplicates done\n");
  duplicates_removed = 1;
}

/* ---------------------------------------------------------------------------- */
// Hash function used for lump names.
unsigned int W_LumpNameHash(const char *s)
{
    // This is the djb2 string hash function, modded to work on strings
    // that have a maximum length of 8.
    unsigned int        result = 5381;
    unsigned int        i;

    for (i = 0; i < 8 && s[i] != '\0'; ++i)
        result = ((result << 5) ^ result) ^ toupper(s[i]);

    return result;
}

/* ---------------------------------------------------------------------------- */
//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

int		reloadlump;
char*		reloadname;


void W_AddFile (const char *filename)
{
    wadinfo_t		header;
    lumpinfo_t* 	lump_p;
    unsigned		i;
    FILE* 		handle;
    int 		length;
    int 		startlump;
    filelump_t* 	fileinfo;
    filelump_t		singleinfo;
    FILE* 		storehandle;
    int 		its_a_wad;
    int			maps_found;

    // open the file and add to directory

    // handle reload indicator.
    if (filename[0] == '~')
    {
	filename++;
	reloadname = (char*)filename;
	reloadlump = numlumps;
    }

    handle = fopen (filename, "rb");
    if (handle == 0)
    {
	printf (" couldn't open %s\n",filename);
	return;
    }

    printf (" adding %s\n",filename);
    startlump = numlumps;

    its_a_wad = 0;
    if (strcasecmp (filename+strlen(filename)-3 , "wad" ) == 0)
    {
      its_a_wad = 1;
    }
    else
    {
      if ((filesize (filename)) >= (sizeof (header)))
      {
	fread (&header, 1, sizeof(header), handle);
	if (strncmp (header.identification,"IWAD",4) == 0)
	  its_a_wad = 2;
	else if (strncmp (header.identification,"PWAD",4) == 0)
	  its_a_wad = 3;
      }
    }

    switch (its_a_wad)
    {
      case 0:
	// single lump file
	fseek (handle, (long int) 0, SEEK_SET);
	fileinfo = &singleinfo;
	singleinfo.filepos = 0;
	singleinfo.size = filesize(filename);
	singleinfo.size = LONG(singleinfo.size);
	strncpy (singleinfo.name, leafname (filename), sizeof (singleinfo.name));
	numlumps++;
	break;

      case 1:
	// WAD file
	fread (&header, 1, sizeof (header), handle);
	if (strncmp (header.identification,"IWAD",4) == 0)
	{
	  its_a_wad = 2;
	}
	else if (strncmp (header.identification,"PWAD",4) == 0)	// Homebrew levels?
	{
	  its_a_wad = 3;
	}
	else
	{
	  I_Error ("Wad file %s doesn't have IWAD or PWAD id\n", filename);
	}

	// ???modifiedgame = true;
	// Fall through

     case 2:
     case 3:
	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	length = header.numlumps*sizeof(filelump_t);
	fileinfo = alloca (length);
	fseek (handle, header.infotableofs, SEEK_SET);
	fread (fileinfo, 1, length, handle);
	numlumps += header.numlumps;
    }


    // Fill in lumpinfo
    lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
	I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? 0 : handle;
    length = 0;
    i = startlump;
    maps_found = 0;
    do
    {
	lump_p->handle = storehandle;
	lump_p->position = LONG(fileinfo->filepos);
	lump_p->size = LONG(fileinfo->size);
	strncpy (lump_p->name, fileinfo->name, 8);
#if 0
	if (its_a_wad == 3)
	{
	  char cc;
	  if ((strncmp (lump_p->name, "MAP", 3) == 0)
	   && (lump_p->name [5] == 0)
	   && ((cc = lump_p->name [3]) >= '0')
	   && (cc <= '9')
	   && ((cc = lump_p->name [4]) >= '0')
	   && (cc <= '9'))
	  {
	    printf (" %s", lump_p->name);
	    if (++length > 11)
	    {
	      putchar ('\n');
	      length = 0;
	    }
	    maps_found |= 2;
	  }
	  if ((lump_p->name [0] == 'E')
	   && (lump_p->name [2] == 'M')
	   && (lump_p->name [4] == 0)
	   && ((cc = lump_p->name [1]) >= '0')
	   && (cc <= '9')
	   && ((cc = lump_p->name [3]) >= '0')
	   && (cc <= '9'))
	  {
	    printf (" %s", lump_p->name);
	    if (++length > 8)
	    {
	      putchar ('\n');
	      length = 0;
	    }
	    maps_found |= 1;
	  }
	}
#endif
      lump_p++;
      fileinfo++;
    } while (++i < numlumps);


#if 1
    if (its_a_wad == 3)
    {
      i = startlump;
      lump_p = &lumpinfo[startlump];
      do
      {
	if ((strcasecmp ((lump_p+1)->name, "THINGS") == 0)
	 && (strncasecmp ((lump_p+2)->name, "LINEDEFS", 8) == 0)
	 && (strncasecmp ((lump_p+3)->name, "SIDEDEFS", 8) == 0)
	 && (strncasecmp ((lump_p+4)->name, "VERTEXES", 8) == 0)
	 && (strcasecmp ((lump_p+5)->name, "SEGS") == 0)
	 && (strncasecmp ((lump_p+6)->name, "SSECTORS", 8) == 0)
	 && (strcasecmp ((lump_p+7)->name, "NODES") == 0)
	 && (strcasecmp ((lump_p+8)->name, "SECTORS") == 0)
	 && (strcasecmp ((lump_p+9)->name, "REJECT") == 0)
	 && (strncasecmp ((lump_p+10)->name, "BLOCKMAP", 8) == 0))
	{
	  if (strncasecmp (lump_p->name, "MAP", 3))
	    maps_found |= 1;
	  else
	    maps_found |= 2;

	  printf (" %s", lump_p->name);
	  if (++length > 11)
	  {
	    putchar ('\n');
	    length = 0;
	  }
	}
	lump_p++;
	fileinfo++;
      } while (++i < numlumps);
    }
#endif


    if (length)
      putchar ('\n');

    if (reloadname)
	fclose (handle);

    switch (maps_found)
    {
      case 1:
	if (gamemode == commercial)
	  I_Error ("This wadfile requires doom.wad or doomu.wad\n");
	break;

      case 2:
	if (gamemode != commercial)
	  I_Error ("This wadfile requires doom2.wad\n");
	break;
    }
}

/* ---------------------------------------------------------------------------- */
//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload (void)
{
    wadinfo_t		header;
    int 		lumpcount;
    lumpinfo_t* 	lump_p;
    unsigned		i;
    FILE* 		handle;
    int 		length;
    filelump_t* 	fileinfo;

    if (!reloadname)
	return;

    handle = fopen (reloadname, "rb");
    if (handle == NULL)
	I_Error ("W_Reload: couldn't open %s",reloadname);

    fread (&header,1, sizeof(header),handle);
    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount*sizeof(filelump_t);
    fileinfo = alloca (length);
    fseek (handle, header.infotableofs, SEEK_SET);
    fread (fileinfo, 1, length, handle);

    // Fill in lumpinfo
    lump_p = &lumpinfo[reloadlump];

    for (i=reloadlump ;
	 i<reloadlump+lumpcount ;
	 i++,lump_p++, fileinfo++)
    {
	if (lumpcache[i])
	    Z_Free (lumpcache[i]);

	lump_p->position = LONG(fileinfo->filepos);
	lump_p->size = LONG(fileinfo->size);
    }

    fclose (handle);
}

/* ---------------------------------------------------------------------------- */
//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (char** filenames)
{
    int size;
    int	i;

    // open all the files, load headers, and count lumps
    numlumps = 0;

    // will be realloced as lumps are added
    lumpinfo = malloc(1);

    for ( ; *filenames ; filenames++)
	W_AddFile (*filenames);

    if (!numlumps)
	I_Error ("W_InitFiles: no files found");

    // set up caching
    size = numlumps * sizeof(*lumpcache);
    lumpcache = malloc (size);

    if (!lumpcache)
	I_Error ("Couldn't allocate lumpcache");

    memset (lumpcache,0, size);

    /* Now start sorting. */
    if ((sortedlumps = malloc(numlumps*sizeof(int))) == NULL)
    {
	I_Error("Couldn't claim memory for sortedlumps!\n");
    }

    for (i=0; i<numlumps; i++) sortedlumps[i] = i;
    duplicates_removed = 0;

    // W_QuicksortLumps (0, numlumps-1);
}

/* ---------------------------------------------------------------------------- */
//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile (const char* filename)
{
    char*	names[2];

    names[0] = (char*)filename;
    names[1] = NULL;
    W_InitMultipleFiles (names);
}



/* ---------------------------------------------------------------------------- */
//
// W_NumLumps
//
int W_NumLumps (void)
{
    return numlumps;
}

/* ---------------------------------------------------------------------------- */

#if 0
static void W_Gen_2x_ints (const char * name, unsigned int * v1, unsigned int * v2)
{
    union {
	char	s[9];
	int	x[2];
    } name8;

    // make the name into two integers for easy compares
    name8.x[0] = 0;
    name8.x[1] = 0;
    strncpy (name8.s,name,8);

    // in case the name was a full 8 chars
    name8.s[8] = 0;

    // case insensitive
    strupr (name8.s);

    *v1 = name8.x[0];
    *v2 = name8.x[1];
}

/* ---------------------------------------------------------------------------- */

/* Fast string compare of 8 characters */
/* Actually, I think it's worse! */

int W_Strcmp_8 (const char * name1, const char * name2)
{
  unsigned int v1;
  unsigned int v2;
  unsigned int v3;
  unsigned int v4;
  int rc;

  W_Gen_2x_ints (name1, &v1, &v2);
  W_Gen_2x_ints (name2, &v3, &v4);

  rc = v1 - v3;
  if (rc)
    return (rc);

  rc = v2 - v4;
  return (rc);
}
#endif

/* ---------------------------------------------------------------------------- */

// Searches for a lump name within a section - e.g. S_START - S_END
// Not currently in 'cos its *way* too slow
#if 0
int W_CheckNumForNameBounded (const char * start, const char * end, const char * name)
{
  int lump;
  int done;
  char sstart [12];
  char eend  [12];

  /* Some pwads have names with the first letter duplicated */
  /* e.g. S_START becomes SS_START */
  sstart [0] = start [0];
  strcpy (&sstart[1], start);

  eend [0] = end [0];
  strcpy (&eend[1], end);

  lump = numlumps;

  do
  {
    /* Search for 'end' */
    lump--;
    if ((strncasecmp (lumpinfo[lump].name, end, 8) == 0)
     || (strncasecmp (lumpinfo[lump].name, eend, 8) == 0))
    {
      /* Found it - now keep adding until found 'start' or no longer in the same WAD */
      done = 0;
      do
      {
	lump--;
	if ((strncasecmp (lumpinfo[lump].name, start, 8) == 0)
	 || (strncasecmp (lumpinfo[lump].name, sstart, 8) == 0)
	 || (lumpinfo[lump].handle != lumpinfo[lump+1].handle))
	{
	  done = 1;
	}
	else
	{
	  if (strncasecmp (lumpinfo[lump].name, name, 8) == 0)
	    return (lump);
	}
      }
      while (done == 0);
    }

  } while (lump);

  return (-1);
}
#endif

/* ---------------------------------------------------------------------------- */

int W_NextLumpNumForName (const char* name, int startlump)
{
  do
  {
    if (++startlump >= numlumps)
      return (-1);
  } while (strncasecmp (lumpinfo[startlump].name, name, 8));
  return (startlump);
}

/* ---------------------------------------------------------------------------- */
//
// W_CheckNumForNameMasked
// Returns -1 if name not found.
//
#if 0
int W_CheckNumForNameMasked (const char* name, char * mask, int startlump)
{
    union {
	char	s[8];
	int	x[2];
    } name8;

    int 	v1;
    int 	v2;
    int 	m1;
    int 	m2;
    lumpinfo_t* lump_p;
    unsigned int p,cc;
    char *	ptr_d;

    // make the name into two integers for easy compares

    name8.x[0] = 0;
    name8.x[1] = 0;

    p = 8;
    ptr_d = name8.s;
    do
    {
      cc = *name++;
      if (cc == 0)
	break;
      cc = toupper (cc);	// case insensitive
      *ptr_d++ = cc;
    } while (--p);

    v1 = name8.x[0];
    v2 = name8.x[1];

    memcpy (name8.s,mask,8);

    m1 = name8.x[0];
    m2 = name8.x[1];

    v1 &= m1;
    v2 &= m2;

    // scan backwards so patch lump files take precedence
    if (startlump == -1)
      lump_p = lumpinfo + numlumps;
    else
      lump_p = lumpinfo + startlump + 1;

    while (lump_p-- != lumpinfo)
    {
	if ( ((*(int *)lump_p->name) & m1) == v1
	     && ((*(int *)&lump_p->name[4]) & m2) == v2)
	{
	    return lump_p - lumpinfo;
	}
    }

    // TFB. Not found.
    return -1;
}
#endif
/* ---------------------------------------------------------------------------- */
//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForNameLinear (const char* name)
{
    union {
	char	s[8];
	int	x[2];
    } name8;

    int 	v1;
    int 	v2;
    lumpinfo_t* lump_p;
    unsigned int p,cc;
    char *	ptr_d;

    // make the name into two integers for easy compares

    name8.x[0] = 0;
    name8.x[1] = 0;

    p = 8;
    ptr_d = name8.s;
    do
    {
      cc = *name++;
      if (cc == 0)
	break;
      cc = toupper (cc);	// case insensitive
      *ptr_d++ = cc;
    } while (--p);

    v1 = name8.x[0];
    v2 = name8.x[1];

    // scan backwards so patch lump files take precedence
    lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
	/* printf ("  Comparing %s %X %X with %X %X %s\n", name8.s, v1, v2, *(int *) lump_p->name, *(int *) &lump_p->name[4], lump_p->name); */
	if ( *(int *)lump_p->name == v1
	     && *(int *)&lump_p->name[4] == v2)
	{
	    return lump_p - lumpinfo;
	}
    }

    // TFB. Not found.
    return -1;
}

/* ---------------------------------------------------------------------------- */

int W_CheckNumForName (const char* name)
{
    union {
	char	s[8];
	int	x[2];
    } name8;

    int		v1;
    int		v2;
    /* New for binary search */
    int		pos, step, count;
    unsigned int p,cc;
    char *	ptr_d;

    // printf ("W_CheckNumForName (%s) %d\n", name, duplicates_removed);
    if (duplicates_removed == 0)				// Cannot use this function until after the
    {								// duplicate stuff has been removed which doesn't
      return (W_CheckNumForNameLinear (name));			// happen until late in the startup.
    }

    // make the name into two integers for easy compares

    name8.x[0] = 0;
    name8.x[1] = 0;

    p = 8;
    ptr_d = name8.s;
    do
    {
      cc = *name++;
      if (cc == 0)
	break;
      cc = toupper (cc);	// case insensitive
      *ptr_d++ = cc;
    } while (--p);

    v1 = name8.x[0];
    v2 = name8.x[1];

    /* Re-write, does binary rather than linear search */
    pos = (numlumps+1) >> 1; step = (pos+1) >> 1; count = (pos << 1);
    while (count != 0)
    {
	int	    w1 = ((int*)lumpinfo[sortedlumps[pos]].name)[0];
	int	    w2 = ((int*)lumpinfo[sortedlumps[pos]].name)[1];

	// printf ("pos = %u, step = %u, count = %u (%X %X %X %X\n", pos, step, count, v1,v2,w1,w2);

	if ((v1 == w1) && (v2 == w2))
	    return sortedlumps[pos];

	if (v1 < w1 || (v1 == w1 && (v2 < w2)))
	{
	    pos -= step; if (pos < 0) pos = 0;
	}
	else
	{
	    pos += step; if (pos >= numlumps) pos = numlumps-1;
	}
	step = (step+1) >> 1; count >>= 1;
    }
    /* TFB. Not found.*/
    return -1;
}

/* ---------------------------------------------------------------------------- */
//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (const char* name)
{
    int i;

    i = W_CheckNumForName (name);

    if (i == -1)
      I_Error ("W_GetNumForName: %s not found!", name);

    return i;
}

/* ---------------------------------------------------------------------------- */
//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
unsigned int W_LumpLength (int lump)
{
    if (lump >= numlumps)
	I_Error ("W_LumpLength: %i >= numlumps (%i)",lump, numlumps);

    return lumpinfo[lump].size;
}

/* ---------------------------------------------------------------------------- */
//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void
W_ReadLump
( int		lump,
  void* 	dest )
{
    int 	c;
    lumpinfo_t* l;
    FILE* 	handle;

    if (lump >= numlumps)
	I_Error ("W_ReadLump: %i >= numlumps",lump);

    l = lumpinfo+lump;

    // ??? I_BeginRead ();

    handle = l->handle;
    if (handle == 0)
    {
	// reloadable file, so use open / read / close
	handle = fopen (reloadname, "rb");
	if (handle == NULL)
	    I_Error ("W_ReadLump: couldn't open %s",reloadname);
    }

    fseek (handle, l->position, SEEK_SET);
    c = fread (dest, 1, l->size,handle);

    if (c < l->size)
	I_Error ("W_ReadLump: only read %i of %i on lump %i",
		 c,l->size,lump);

    if (l->handle == NULL)
	fclose (handle);

    // ??? I_EndRead ();
}

/* ---------------------------------------------------------------------------- */
//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( int		lump,
  int		tag )
{
    byte*	ptr;

    if ((unsigned)lump >= numlumps)
	I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

    if (!lumpcache[lump])
    {
	// read the lump in

	//printf ("cache miss on lump %i\n",lump);
	ptr = Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
	W_ReadLump (lump, lumpcache[lump]);
    }
    else
    {
	//printf ("cache hit on lump %i\n",lump);
	Z_ChangeTag (lumpcache[lump],tag);
    }

    return lumpcache[lump];
}

/* ---------------------------------------------------------------------------- */
//
// W_CacheLumpName
//
void*
W_CacheLumpName
( const char* 	name,
  int		tag )
{
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}

/* ---------------------------------------------------------------------------- */
//
// W_CacheLumpName
//
void*
W_CacheLumpName0
( const char* 	name,
  int		tag )
{
  int i;

  i = W_CheckNumForName (name);
  if (i == -1)
    return (0);

  return W_CacheLumpNum (i, tag);
}

/* ---------------------------------------------------------------------------- */
//
// Release a lump back to the cache, so that it can be reused later
// without having to read from disk again, or alternatively, discarded
// if we run out of memory.
//
// Back in Vanilla Doom, this was just done using Z_ChangeTag
// directly, but now that we have WAD mmap, things are a bit more
// complicated ...
//
void W_ReleaseLumpNum (unsigned int lumpnum)
{
  if (lumpnum >= numlumps)
      I_Error("W_ReleaseLumpNum: %i >= numlumps", lumpnum);

  Z_ChangeTag (lumpcache[lumpnum], PU_CACHE);
}

/* ---------------------------------------------------------------------------- */
//
// W_Profile
//
#if 0

int		info[2500][10];
int		profilecount;

void W_Profile (void)
{
    int 	i;
    memblock_t* block;
    void*	ptr;
    char	ch;
    FILE*	f;
    int 	j;
    char	name[9];


    for (i=0 ; i<numlumps ; i++)
    {
	ptr = lumpcache[i];
	if (!ptr)
	{
	    ch = ' ';
	    continue;
	}
	else
	{
	    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
	    if (block->tag < PU_PURGELEVEL)
		ch = 'S';
	    else
		ch = 'P';
	}
	info[i][profilecount] = ch;
    }
    profilecount++;

    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
	memcpy (name,lumpinfo[i].name,8);

	for (j=0 ; j<8 ; j++)
	    if (!name[j])
		break;

	for ( ; j<8 ; j++)
	    name[j] = ' ';

	fprintf (f,"%s ",name);

	for (j=0 ; j<profilecount ; j++)
	    fprintf (f,"    %c",info[i][j]);

	fprintf (f,"\n");
    }
    fclose (f);
}
#endif
/* ---------------------------------------------------------------------------- */

int W_SameWadfile (int lump1, int lump2)
{
  if (lumpinfo[lump1].handle == lumpinfo[0].handle)
  {
    if (lumpinfo[lump2].handle != lumpinfo[0].handle)
    {
      return (0);
    }
  }
  else
  {
    if (lumpinfo[lump2].handle == lumpinfo[0].handle)
    {
      return (0);
    }
  }

  return (1);
}

/* -------------------------------------------------------------------------------------------- */
/*
   Sort out the ?_START and ?_END markers.
   Some wads have ??_START or ??_END and also can be badly nested.
*/

void W_Find_Start_Ends (void)
{
  int loading;
  int lump;
  int endlump;
  int startlump;
  lumpinfo_t* lump_ptr;

  lump = 0;
  loading = 0;
  endlump = 0;
  lump_ptr = &lumpinfo[0];

  do
  {
    if (strncasecmp (lump_ptr->name+2, "_START", 6) == 0)
    {
      /* Found a ??_START marker, shuffle it down */
      // printf ("Shuffled %s %u\n", lump_ptr->name, lump);
      memcpy (lump_ptr->name+1, lump_ptr->name+2, 6);
      lump_ptr->name [7] = 0;
    }
    if (strncasecmp (lump_ptr->name+1, "_START", 7) == 0)
    {
      if (loading)		// Nested?
      {
	// printf ("Nested start (%s) %u\n", lump_ptr->name, lump);
	lump_ptr->name [1] = '-';
      }
      else
      {
	loading = 1;
	startlump = lump;
	endlump = 0;
	// printf ("%s lump = %u\n", lump_ptr->name, lump);
      }
    }
    if (strncasecmp (lump_ptr->name+2, "_END", 5) == 0)
    {
      /* Found a ??_END marker, shuffle it down */
      // printf ("Shuffled %s %u\n", lump_ptr->name, lump);
      memcpy (lump_ptr->name+1, lump_ptr->name+2, 6);
      lump_ptr->name [7] = 0;
    }
    if (strncasecmp (lump_ptr->name+1, "_END", 5) == 0)
    {
      if ((loading == 0)
       && (endlump))
      {
	lumpinfo[endlump].name [1] = '-';
	// printf ("Previous end was nested\n");
      }
      loading = 0;
      endlump = lump;
      // printf ("%s lump = %u\n", lump_ptr->name, lump);
    }

    if ((loading)
     && (lump_ptr->handle != lumpinfo[startlump].handle))
    {
      loading = 0;
      endlump = 0;
      // printf ("Ran off end of WAD without finding END marker\n");
    }

    lump_ptr++;
  } while (++lump < numlumps);
}

/* -------------------------------------------------------------------------------------------- */
