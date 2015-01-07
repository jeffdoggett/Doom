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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"

#ifdef __GNUG__
#pragma interface
#endif


typedef enum
{
  D_D_DEVSTR,
  D_D_CDROM,
  D_D_CDROM_RO,
  D_ULTIMATE,
  D_SHAREWARE,
  D_REGISTERED,
  D_HELL_ON_EARTH,
  D_PLUTONIA,
  D_TNT,
  D_PUBLIC,
  D_SYSTEM_STARTUP,
  D_MODIFIED,
  D_SHAREWARE_2,
  D_COMMERCIAL,
  D_NOT_SHAREWARE,
  D_DO_NOT_DIST
} main_texts_t;


unsigned int D_Startup_msg_number (void);

void D_AddFile (const char *file);


//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain (void);

// Called by IO functions when input is detected.
void D_PostEvent (event_t* ev);



//
// BASE LEVEL
//
void D_PageTicker (void);
void D_PageDrawer (const char * page);
void D_AdvanceDemo (void);
void D_StartTitle (void);

#endif
