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
//  Sky rendering. The DOOM sky is a texture map like any
//  wall, wrapping around. A 1024 columns equal 360 degrees.
//  The default sky map is 256 columns and repeats 4 times
//  on a 320 screen?
//
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif

#include "includes.h"

//
// sky mapping
//
int	skyflatnum;
int	skytexture;
int	skytexturemid;
int	skycolumnoffset;
int	skyscrolldelta;
fixed_t	skyiscale;

const char sky_0 [] = "SKY0";
const char sky_1 [] = "SKY1";
const char sky_2 [] = "SKY2";
const char sky_3 [] = "SKY3";
const char sky_4 [] = "SKY4";
const char sky_5 [] = "SKY5";
const char sky_6 [] = "SKY6";
const char sky_7 [] = "SKY7";
const char sky_8 [] = "SKY8";
const char sky_9 [] = "SKY9";
extern char * finale_backdrops[];

typedef struct skypatch_s
{
  struct skypatch_s * next;
  char name [12];
  fixed_t yscale;
  fixed_t texturemid;
} skypatch_t;

static skypatch_t * list_head = NULL;
static skypatch_t * current_sky_patch = NULL;

/* -------------------------------------------------------------------------------------------- */

static skypatch_t * find_skypatch (const char * skyname)
{
  skypatch_t * ptr = list_head;

  while (ptr)
  {
    if (strcasecmp (skyname, ptr -> name) == 0)
      break;
    ptr = ptr -> next;
  }

  return (ptr);
}

/* -------------------------------------------------------------------------------------------- */
/*
   Allow the sky size & position to be patched.
   E.g. Phobos.wad   Patch Sky "Sky1" yscale=56173
*/

void R_PatchSky (const char * a_line)
{
  int pos;
  char cc;
  unsigned int value;
  const char * qpos;
  const char * vpos;
  skypatch_t * ptr;
  char skyname [12];

  qpos = strchr (a_line, '\"');
  if (qpos == 0)
    return;

  pos = 0;
  ++qpos;
  do
  {
    cc = *qpos++;
    if (cc == '\"') cc = 0;
    skyname[pos++] = cc;
  } while (cc && (pos < sizeof (skyname)));

  ptr = find_skypatch (skyname);
  if (ptr == NULL)
  {
    ptr = malloc (sizeof (*ptr));
    if (ptr == NULL)
      return;
    ptr -> next = list_head;
    list_head = ptr;

    strncpy (ptr -> name, skyname, sizeof (ptr -> name));
    ptr -> yscale = FRACUNIT;
    ptr -> texturemid = 100*FRACUNIT;
  }

  while (*qpos == ' ') qpos++;

  vpos = strchr (qpos, '=');
  if (vpos == NULL)
    return;

  ++vpos;
  while (*vpos == ' ') vpos++;

  value = (unsigned int) strtoul (vpos, NULL, 0);

  if (strncasecmp (qpos, "yscale", 6) == 0)
  {
    ptr -> yscale = value;
    return;
  }

  if (strncasecmp (qpos, "texturemid", 10) == 0)
  {
    ptr -> texturemid = value;
    return;
  }
}

/* -------------------------------------------------------------------------------------------- */

void R_InitSkyMapScale (void)
{
  fixed_t sis;
  skypatch_t * ptr;

  sis = (FRACUNIT*SCREENWIDTH/viewwidth)>>detailshift;
  if (SCREENHEIGHT > 200)
  {
    sis = (sis * 200) / SCREENHEIGHT;
  }

  ptr = current_sky_patch;
  if (ptr)
  {
    sis = FixedMul (sis, ptr->yscale);
    skytexturemid = ptr->texturemid;
  }

  skyiscale = sis;
}

/* -------------------------------------------------------------------------------------------- */

static int patch_skytexture (const char * skyname)
{
  skypatch_t * ptr = list_head;

  ptr = find_skypatch (skyname);
  current_sky_patch = ptr;
  if (ptr == NULL)
    return (0);

  // It's possible to be called before the viewsize has been set.
  if (viewwidth == 0)
    return (1);

  R_InitSkyMapScale ();
  return (1);
}

/* -------------------------------------------------------------------------------------------- */

static int check_skytexture (const char * skyname)
{
  char cc;
  int i,j;

  i = R_CheckTextureNumForName (skyname);
  if (i != -1)
  {
    if ((patch_skytexture (skyname))

    // For the time being ignore tall textures as these are only
    // for ports with mouselook.
     || (textures[i].height == (0x80*FRACUNIT)))
      return (i);

#ifdef NORMALUNIX
    printf ("Map has a tall sky (%s %u)\n", skyname, textures[i].height >> FRACBITS);
#endif

    // Let us see if the name starts with the standard "SKYn".
    // (BTSX has SKY1TALL and SKY1 etc)
    if ((M_CheckParm ("-noskysubstitute") == 0)
     && ((j=dh_instr (skyname, "SKY")) != 0)
     && ((cc = skyname[j+2]) >= '0')
     && (cc <= '9'))
    {
      char buffer [12];
      strcpy (buffer, &skyname[j-1]);
      j = 3;
      do
      {
	cc = buffer [++j];
      } while ((cc >= '0') && (cc <= '9'));
      buffer [j] = 0;

      j = R_CheckTextureNumForName (buffer);
      if ((j != -1)
       && (textures[j].height == (0x80*FRACUNIT))
       && (textures[j].pnames_lump == textures[i].pnames_lump))	// and in the same PNAMES directory?
      {
#ifdef NORMALUNIX
	printf ("Substituted sky (%s)\n", buffer);
#endif
	patch_skytexture (buffer);
	return (j);
      }
    }

    // This fixes the skies in BTSX - but not sure about others.
    if (M_CheckParm ("-noskyoffset") == 0)
    {
#ifdef NORMALUNIX
      printf ("Adjusted sky offset\n");
#endif
      skytexturemid = -28*FRACUNIT;
    }
  }
  return (i);
}

/* -------------------------------------------------------------------------------------------- */

static int load_skytexture (map_dests_t * map_info_p)
{
  int i;

  if (M_CheckParm ("-usedefaultsky") == 0)
  {
    i = check_skytexture (map_info_p -> sky);
    if (i != -1)
      return (i);
  }

  // Not present, try the old defaults.
  if (gamemode == commercial)
  {
    if ((gamemap >= 21)
     && ((i = check_skytexture (sky_3)) != -1))
      return (i);

    if ((gamemap >= 12)
     && ((i = check_skytexture (sky_2)) != -1))
      return (i);

    return (check_skytexture (sky_1));
  }

  // DOOM determines the sky texture to be used
  // depending on the current episode, and the game version.

  // Work backwards 'till we get one.
  switch (gameepisode)
  {
    default:
      i = check_skytexture (sky_9);
      if (i != -1)
	return (i);

    case 8:
      i = check_skytexture (sky_8);
      if (i != -1)
	return (i);

    case 7:
      i = check_skytexture (sky_7);
      if (i != -1)
	return (i);

    case 6:
      i = check_skytexture (sky_6);
      if (i != -1)
	return (i);

    case 5:
      i = check_skytexture (sky_5);
      if (i != -1)
	return (i);

    case 4:
      i = check_skytexture (sky_4);
      if (i != -1)
	return (i);

    case 3:
      i = check_skytexture (sky_3);
      if (i != -1)
	return (i);

    case 2:
      i = check_skytexture (sky_2);
      if (i != -1)
	return (i);

    case 1:
      return (check_skytexture (sky_1));	// Error if this fails!

    case 0:
      i = check_skytexture (sky_0);
      if (i != -1)
	return (i);

      return (check_skytexture (sky_1));	// Error if this fails!
  }
}

/* -------------------------------------------------------------------------------------------- */
//
// R_InitSkyMap
//
void R_InitSkyMap (map_dests_t * map_info_p)
{
  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.
  skyflatnum = R_FlatNumForName (finale_backdrops[BG_F_SKY1]);
  skycolumnoffset = 0;
  skyscrolldelta = map_info_p -> skydelta;
  skytexturemid = 100*FRACUNIT;
  if ((skytexture = load_skytexture (map_info_p)) == -1)
    I_Error ("SKY texture missing");
}

/* -------------------------------------------------------------------------------------------- */
