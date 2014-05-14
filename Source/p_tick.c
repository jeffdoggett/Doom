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
//	Archiving: SaveGame I/O.
//	Thinker, Ticker.
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: p_tick.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif

#include "includes.h"

//-----------------------------------------------------------------------------

int	leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



// Both the head and tail of the thinker list.
thinker_t * thinker_head;
static thinker_t * thinker_tail;

//-----------------------------------------------------------------------------
//
// P_InitThinkers
//
void P_InitThinkers (void)
{
  thinker_head = thinker_tail = NULL;
}

//-----------------------------------------------------------------------------
//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker (thinker_t* thinker, actionf_p1 func)
{
  if (thinker_head == NULL)
    thinker_head = thinker;
  else
    thinker_tail -> next = thinker;

  thinker_tail = thinker;

  thinker -> next = NULL;
  thinker -> function.acp1 = func;
}

//-----------------------------------------------------------------------------
//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//

void P_RemoveThinker (thinker_t* thinker)
{
  // FIXME: NOP.
  thinker->function.aci = -1;
#ifdef USE_BOOM_P_ChangeSector
  thinker->removedelay = 100;
#endif
}

//-----------------------------------------------------------------------------
//
// P_RunThinkers
//
static void P_RunThinkers (void)
{
  thinker_t*	lastthinker;
  thinker_t*	currentthinker;
  thinker_t**	prevptr;

  prevptr = &thinker_head;
  lastthinker = NULL;

  while ((currentthinker = *prevptr) != NULL)
  {
    if (currentthinker->function.aci == -1)
    {
#ifdef USE_BOOM_P_ChangeSector
      if (--currentthinker->removedelay != 0)
      {
	lastthinker = currentthinker;
	prevptr = &currentthinker -> next;
      }
      else
#endif
      {
	/* time to remove it*/
	if ((*prevptr = currentthinker->next) == NULL)	// Released the last block?
	{
	  thinker_tail = lastthinker;
	  // printf ("Last thinker %X %X\n", prevptr, lastthinker);
	}
        Z_Free (currentthinker);
      }
    }
    else
    {
      if (currentthinker->function.acp1)
	currentthinker->function.acp1 (currentthinker);
      lastthinker = currentthinker;
      prevptr = &currentthinker -> next;
    }
  }
}

//-----------------------------------------------------------------------------
//
// P_Ticker
//

void P_Ticker (void)
{
  int		i;

  // run the tic
  if (paused)
      return;

  // pause if in menu and at least one tic has been run
  if ( !netgame
   && menuactive
   && !demoplayback
   && players[consoleplayer].viewz != 1)
  {
    return;
  }


  for (i=0 ; i<MAXPLAYERS ; i++)
    if (playeringame[i])
      P_PlayerThink (&players[i]);

  P_RunThinkers ();
  P_UpdateSpecials ();
  P_RespawnSpecials ();

  // for par times
  leveltime++;
}

//-----------------------------------------------------------------------------
