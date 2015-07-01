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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//	Remark: this was the only stuff that, according
//	 to John Carmack, might have been useful for
//	 Quake.
//
//---------------------------------------------------------------------

#ifndef __Z_ZONE__
#define __Z_ZONE__

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef NORMALUNIX
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#endif
#ifdef __riscos
#include "acorn.h"
#include <stdint.h>
#endif
#include "doomtype.h"

//
// ZONE MEMORY
// PU - purge tags.
//
enum
{
  PU_FREE,	// a free block
  PU_STATIC,	// static entire execution time
  PU_SOUND,	// static while playing
  PU_MUSIC,	// static while playing
  PU_LEVEL,	// static until level exited
  PU_LEVSPEC,	// a special thinker in a level
  PU_LEVMAP,	// sector contents list
  PU_CACHE,
  PU_MAX,	// Must always be last -- killough
};

#define PU_PURGELEVEL    PU_CACHE    // First purgable tag's level

void Z_Init (void);
void *Z_Malloc (size_t size, uint32_t tag, void **ptr);
void Z_Free (void *ptr);
void Z_FreeTags (uint32_t lowtag, uint32_t hightag);
void Z_ChangeTag (void *ptr, uint32_t tag);
void *Z_Calloc (size_t size, uint32_t tag, void **user);
void *Z_Realloc (void *ptr, size_t n, uint32_t tag, void **user);
char *Z_Strdup (const char *s, uint32_t tag, void **user);

#endif
