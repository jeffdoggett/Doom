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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------


#ifndef __W_WAD__
#define __W_WAD__


#ifdef __GNUG__
#pragma interface
#endif


//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char		identification[4];
    int 		numlumps;
    int 		infotableofs;

} wadinfo_t;


typedef struct
{
    int 		filepos;
    int 		size;
    char		name[8];

} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char	name[8];
    FILE* 	handle;
    int 	position;
    int 	size;
} lumpinfo_t;


extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;
extern	int		numlumps;

void	W_InitMultipleFiles (char** filenames);
void	W_Reload (void);

void	W_QuicksortLumps (int from, int to);
void	W_RemoveDuplicates (void);
int	W_CheckNumForNameBounded (const char * start, const char * end, char * name);
int	W_CheckNumForNameMasked (const char* name, char * mask, int startlump);
int	W_CheckNumForNameLinear (const char* name);
int	W_CheckNumForName (const char* name);
int	W_GetNumForName (const char* name);
int	W_NextLumpNumForName (const char* name, int startlump);
int	W_Strcmp_8 (const char * name1, const char * name2);

char *	strupr (char * name);
unsigned int filesize (const char * path);

unsigned int W_LumpLength (int lump);
void	W_ReadLump (int lump, void *dest);

void*	W_CacheLumpNum (int lump, int tag);
void*	W_CacheLumpName (const char* name, int tag);
void*	W_CacheLumpName0 (const char* name, int tag);
int W_SameWadfile (int lump1, int lump2);

#ifdef BOOM

typedef struct prelumpinfo_t {
    char name[8];
    int size;
    const void *data;
} prelumpinfo_t;

extern const prelumpinfo_t	predefined_lumps[];
extern const int		num_predefined_lumps;

#endif



#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
