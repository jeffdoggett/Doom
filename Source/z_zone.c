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
//	Zone Memory Allocation. Neat.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: z_zone.c,v 1.4 1997/02/03 16:47:58 b1 Exp $";
#endif

#include "includes.h"
#include <stddef.h>				// For the offsetof() macro

/* ------------------------------------------------------------------------------------------------ */

typedef struct memblock
{
  uint32_t		zoneid;
  struct memblock	*next;
  struct memblock	*prev;
  size_t		size;
  void			**user;
  uint32_t		tag;
  byte			data [4];		// Must be the last element.
} memblock_t;

// signature for block header
#define ZONEID		0x931d4a11
#define HEADER_SIZE	(offsetof(memblock_t,data))

/* ------------------------------------------------------------------------------------------------ */

static memblock_t	*blockbytag[PU_MAX];

/* ------------------------------------------------------------------------------------------------ */
// 0 means unlimited, any other value is a hard limit
// static uint32_t		memory_size;
// static uint32_t		free_memory;

void Z_Init (void)
{
  unsigned int p;

//  memory_size = 0;
//  free_memory = 0;

  p = 0;
  do
  {
    blockbytag [p] = NULL;
  } while (++p < PU_MAX);
}

/* ------------------------------------------------------------------------------------------------ */

static void add_to_list (memblock_t *block)
{
  uint32_t tag;
  memblock_t *next;

  tag = block -> tag;
  next = blockbytag [tag];
  if (next)
    next -> prev = block;
  blockbytag [tag] = block;
  block -> next = next;
  block -> prev = NULL;
}

/* ------------------------------------------------------------------------------------------------ */

static void remove_from_list (memblock_t *block)
{
  memblock_t *next;
  memblock_t *prev;

  next = block -> next;
  prev = block -> prev;

  if (next)
    next -> prev = prev;

  if (prev == NULL)
    blockbytag [block->tag] = next;
  else
    prev -> next = next;
}

/* ------------------------------------------------------------------------------------------------ */

static void * zone_malloc (size_t size, uint32_t tag, void **user)
{
  byte * rc;
  memblock_t *block;

  rc = NULL;

  if (size)				// malloc(0) returns NULL
  {
    do
    {
      block = (memblock_t *) malloc (size + HEADER_SIZE);
      if (block)
      {
	//  free_memory -= size;
	block->zoneid = ZONEID;
	block->size = size;
	block->tag = tag;		// tag
	block->user = user;		// user
	add_to_list (block);

	rc = block->data;
	break;
      }

      /* failed to claim memory, can we release some from the cache? */

      if (!blockbytag[PU_CACHE])
	break;

      Z_FreeTags (PU_CACHE, PU_CACHE);	// Yes.
    } while (1);
  }

  if (user)				// if there is a user
    *user = rc;				// set user to point to new block

  return (rc);
}

/* ------------------------------------------------------------------------------------------------ */

static void block_free (memblock_t *block)
{
  void **user;

  if (block->zoneid != ZONEID)
    I_Error ("Z_Free: Attempt to free invalid memory");

  // printf ("Z_Free %d %X\n", block->tag, block);

  if ((user = block->user) != NULL)	// Nullify user if one exists
    *user = NULL;

  remove_from_list (block);
//free_memory += block->size;
  free (block);
}

/* ------------------------------------------------------------------------------------------------ */

void * Z_Malloc (size_t size, uint32_t tag, void **user)
{
  void * rc;

  rc = zone_malloc (size, tag, user);
  if ((rc == NULL)
   && (size))
    I_Error ("Z_Malloc: Failure trying to allocate %lu bytes", (uint32_t)size);

  return (rc);
}

/* ------------------------------------------------------------------------------------------------ */

void Z_Free (void *ptr)
{
  memblock_t * block;

  if (ptr)
  {
    block = (memblock_t *)((char *)ptr - HEADER_SIZE);
    block_free (block);
  }
}

/* ------------------------------------------------------------------------------------------------ */

void Z_FreeTags (uint32_t lowtag, uint32_t hightag)
{
  memblock_t *block;

  if (lowtag == PU_FREE)
    lowtag = PU_FREE + 1;
  if (hightag > PU_CACHE)
    hightag = PU_CACHE;

  for (; lowtag <= hightag; ++lowtag)
  {
    while ((block = blockbytag[lowtag]) != NULL)
    {
      // printf ("Free tag %d %X\n", block->tag, block);
      block_free (block);
    }
  }
}

/* ------------------------------------------------------------------------------------------------ */

void Z_ChangeTag (void *ptr, uint32_t tag)
{
  memblock_t *block;

  // proff - added sanity check, this can happen when an empty lump is locked
  if (ptr)
  {
    block = (memblock_t *)((char *)ptr - HEADER_SIZE);
    if (block->zoneid != ZONEID)
      I_Error ("Z_ChangeTag: Attempt to change invalid memory");

    // proff - do nothing if tag doesn't differ
    if (tag != block->tag)
    {
      remove_from_list (block);
      block->tag = tag;
      add_to_list (block);
    }
  }
}

/* ------------------------------------------------------------------------------------------------ */

void *Z_Realloc (void *ptr, size_t n, uint32_t tag, void **user)
{
  void *p;
  memblock_t *block;


  if ((ptr != NULL)			// Don't bother if same size as before
   && (n == (block = (memblock_t *)((char *)ptr - HEADER_SIZE))->size))
  {
    p = ptr;
  }
  else
  {
    p = zone_malloc (n, tag, user);

    /* If the allocation failed, but the new amount is */
    /* smaller then just go with the original size. */

    if (p == NULL)			// malloc(0) returns NULL
    {
      if (n == 0)			// Did we request 0 bytes?
      {
	if (ptr)
	  Z_Free (ptr);
      }
      else
      {
	if ((ptr == NULL)
	 || (n > block->size))
	  I_Error ("Z_Realloc: Failure trying to allocate %lu bytes", (uint32_t)n);
	p = ptr;
      }
    }
    else if (ptr)
    {
      memcpy (p, ptr, n <= block->size ? n : block->size);
      Z_Free (ptr);
    }
  }

  if (user)				// in case Z_Free nullified same user
    *user = p;

  return (p);
}

/* ------------------------------------------------------------------------------------------------ */

void *Z_Calloc (size_t size, uint32_t tag, void **user)
{
  void * rc;

  rc = Z_Malloc (size, tag, user);
  return (memset (rc, 0, size));
}

/* ------------------------------------------------------------------------------------------------ */

#if 0
char *Z_Strdup (const char *s, uint32_t tag, void **user)
{
    return strcpy ((char *)Z_Malloc(strlen(s) + 1, tag, user), s);
}
#endif

/* ------------------------------------------------------------------------------------------------ */
