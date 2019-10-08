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
//	Sky rendering.
//
//-----------------------------------------------------------------------------


#ifndef __R_SKY__
#define __R_SKY__


#ifdef __GNUG__
#pragma interface
#endif

// The sky map is 256*128*4 maps.
#define ANGLETOSKYSHIFT		22

extern int skyflatnum;
extern int skytexture;
extern int skytexturemid;
extern int skycolumnoffset;
extern int skyscrolldelta;
extern fixed_t	skyiscale;

extern const char sky_0 [];
extern const char sky_1 [];
extern const char sky_2 [];
extern const char sky_3 [];
extern const char sky_4 [];
extern const char sky_5 [];
extern const char sky_6 [];
extern const char sky_7 [];
extern const char sky_8 [];
extern const char sky_9 [];

void R_InitSkyMap (map_dests_t * map_info_p);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
