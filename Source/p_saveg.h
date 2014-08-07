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
//	Savegame I/O, archiving, persistence.
//
//-----------------------------------------------------------------------------


#ifndef __P_SAVEG__
#define __P_SAVEG__


#ifdef __GNUG__
#pragma interface
#endif


// Persistent storage/archiving.
// These are the load / save game routines.
byte * P_ArchivePlayers (byte * save_p);
byte * P_UnArchivePlayers (byte * save_p);
byte * P_ArchiveWorld (byte * save_p);
byte * P_UnArchiveWorld (byte * save_p);
byte * P_ArchiveThinkers (byte * save_p);
byte * P_UnArchiveThinkers (byte * save_p);
byte * P_ArchiveSpecials (byte * save_p);
byte * P_UnArchiveSpecials (byte * save_p);
void P_RestoreTargets (void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
