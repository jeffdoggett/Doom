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
//
// $Log:$
//
// DESCRIPTION:
//	DOOM strings, by language.
//
//-----------------------------------------------------------------------------


#ifndef __DSTRINGS__
#define __DSTRINGS__


// All important printed strings.
// Language selection (message strings).
// Use -DFRENCH etc.

#ifdef FRENCH
#include "d_french.h"
#else
#include "d_englsh.h"
#endif

// Misc. other strings.
#ifdef __riscos
//#define SAVEGAMEDIR	"<DoomSavDir>."
#define SAVEGAMENAME	"dsav"
#define DIRSEP		"."
#define DIRSEPC		'.'
#define EXTSEP		"/"
#define EXTSEPC		'/'
#define PATHSEPC	','
#else
//#define SAVEGAMEDIR	""
#define SAVEGAMENAME	"doomsav"
#define DIRSEP		"/"
#define DIRSEPC		'/'
#define EXTSEP		"."
#define EXTSEPC		'.'
#define PATHSEPC	':'
#endif


//
// File locations,
//  relative to current position.
// Path names are OS-sensitive.
//
#ifdef __riscos
#define DEVMAPS "<DoomWadDir>/"
#define DEVDATA "<DoomWadDir>/"
#else
#define DEVMAPS "devmaps"
#define DEVDATA "devdata"
#endif
#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
