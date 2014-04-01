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
//
//
//-----------------------------------------------------------------------------


#ifndef __P_ENEMY__
#define __P_ENEMY__


#ifdef __GNUG__
#pragma interface
#endif

typedef void (*actionf2)( void*, unsigned int);
typedef struct
{
  unsigned char episode;
  unsigned char map;
  unsigned int monsterbits;
  unsigned int tag;
  actionf2 func;
  unsigned int action;
} bossdeath_t;

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
