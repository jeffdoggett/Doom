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


#ifndef __F_FINALE__
#define __F_FINALE__


#include "doomtype.h"
#include "d_event.h"
//
// FINALE
//

// Called by main loop.
boolean F_Responder (event_t* ev);

// Called by main loop.
void F_Ticker (void);

// Called by main loop.
void F_Drawer (void);
void F_DrawBackgroundFlat (const char * flat);
void F_StartFinale (int always);

typedef enum
{
  e0text,
  e1text,
  e2text,
  e3text,
  e4text,
  e5text,
  e6text,
  e7text,
  e8text,
  e9text,
  c1text,
  c2text,
  c3text,
  c4text,
  c5text,
  c6text,
  c7text,
  c8text,
  c9text,
  p1text,
  p2text,
  p3text,
  p4text,
  p5text,
  p6text,
  p7text,
  p8text,
  p9text,
  t1text,
  t2text,
  t3text,
  t4text,
  t5text,
  t6text,
  t7text,
  t8text,
  t9text
} finale_texts_t;

typedef enum
{
  BG_F_SKY1,
  BG_FLOOR4_8,
  BG_SFLR6_1,
  BG_MFLR8_4,
  BG_MFLR8_3,
  BG_SLIME16,
  BG_RROCK14,
  BG_RROCK07,
  BG_RROCK17,
  BG_RROCK13,
  BG_RROCK19,
  BG_BOSSBACK,
  BG_PFUB2,
  BG_PFUB1,
  BG_END0,
  BG_ENDI,
  BG_CREDIT,
  BG_HELP,
  BG_HELP1,
  BG_HELP2,
  BG_VICTORY2,
  BG_TITLEPIC,
  BG_ENDPIC,
  BG_ENDPIC05,
  BG_ENDPIC06,
  BG_ENDPIC07,
  BG_ENDPIC08,
  BG_ENDPIC09,
  BG_ENDPIC10,
  BG_ENDPIC11,
  BG_ENDPIC12,
  BG_ENDPIC13,
  BG_ENDPIC14,
  BG_ENDPIC15,
  BG_ENDPIC16,
  BG_ENDPIC17,
  BG_ENDPIC18,
  BG_ENDPIC19,
  BG_ENDPICE
} finale_floors_t;

#define FINALE_MAX_ENDNUM  ((BG_ENDPICE-BG_ENDPIC)+1+4)

typedef struct clusterdefs_s
{
  struct clusterdefs_s * next;
  unsigned int cnumber;
  char * entertext;
  char * normal_exittext;
  char * secret_exittext;
  char * flat;
  char * pic;
  char * music;
} clusterdefs_t;

clusterdefs_t * F_Access_ClusterDef (unsigned int num);
clusterdefs_t * F_Create_ClusterDef (unsigned int num);

typedef struct castlist_s
{
  struct castlist_s * next;
  char cast_name [1];
} castlist_t;

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
