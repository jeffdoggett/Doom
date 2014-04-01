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
//
//-----------------------------------------------------------------------------

#if 0
static const char rcsid[] = "$Id: m_argv.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef __riscos
#include "acorn.h"
#endif

#include "doomtype.h"
#include "dh_stuff.h"
#include "i_system.h"
#include "m_argv.h"

unsigned int	myargc;
char *	myargv [MAXARGVS];

/* ---------------------------------------------------------------------------- */

static unsigned int M_ReadResponseText (unsigned int mypos, const char * buf)
{
  char cc;
  unsigned int pos;
  unsigned int len;
  unsigned int pos1;
  const char * ptr;
  char * sptr;

  if ((*buf)
   && (*buf != '#'))			// Commented line?
  {
    ptr = buf;
    while (*ptr == ' ') ptr++;
    do
    {
      if (ptr [0] == '\"')
      {
	ptr++;
	pos = dh_inchar (ptr, '\"');
      }
      else
      {
	pos = dh_inchar (ptr, ' ');
	pos1 = dh_inchar (ptr, '\t');
	if ((pos1) && ((pos == 0) || (pos1 < pos)))
	  pos = pos1;
      }
      if (pos)
      {
	len = pos;
      }
      else
      {
	len = strlen (ptr);
	pos = len + 1;
      }
      sptr = malloc (pos);
      if (sptr == NULL)
	I_Error ("Out of memory\n");
      pos--;
      strncpy (sptr, ptr, pos);
      sptr [pos] = 0;
      myargv [mypos++] = sptr;
      ptr += len;
      while (((cc = *ptr) == ' ') || (cc == '\t')) ptr++;
    } while (*ptr);
  }

  return (mypos);
}

/* ---------------------------------------------------------------------------- */

static unsigned int M_ReadResponseFile (unsigned int mypos, const char * filename)
{
  FILE * fin;
  char buf [200];

  fin = fopen (filename, "rb");
  if (fin)
  {
    do
    {
      dh_fgets (buf, sizeof (buf) - 1, fin);
      mypos = M_ReadResponseText (mypos, buf);
    } while (!feof (fin));
    fclose (fin);
  }

  return (mypos);
}

/* ---------------------------------------------------------------------------- */

void M_CopyArgs (int argc, char ** argv)
{
  unsigned int pos;
  unsigned int mypos;
  const char * args;

  pos = 0;
  mypos = 0;
  do
  {
    if (argv [pos][0] == '@')
      mypos = M_ReadResponseFile (mypos, &argv[pos][1]);
    else
      myargv [mypos++] = argv [pos];
  } while (++pos < argc);

  args = getenv ("DoomArgs");
  if (args)
  {
    if (args [0] == '@')
      mypos = M_ReadResponseFile (mypos, &args[1]);
    else
      mypos = M_ReadResponseText (mypos, args);
  }

  myargc = mypos;
  myargv [mypos] = NULL;

#if 0
  // DISPLAY ARGS
  printf("%d command-line args:\n",myargc);
  for (pos=0;pos<myargc;pos++)
   printf("argv %d = %s\n", pos, myargv[pos]);
#endif
}

/* ---------------------------------------------------------------------------- */
//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
unsigned int M_CheckParm (const char *check)
{
  int i;

  if (myargc > 1)
  {
    i = 1;
    do
    {
      // printf ("M_CheckParm %s %s\n", check, myargv[i]);
      if (strcasecmp(check, myargv[i]) == 0)
	return (i);
    } while (++i < myargc);
  }
  return (0);
}

/* ---------------------------------------------------------------------------- */
