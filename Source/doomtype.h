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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__


#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool boolean;
#else
typedef enum {false, true} boolean;
#endif
typedef unsigned char byte;
#endif


// Predefined with some OS.
#ifdef LINUX
#include <values.h>
#else
#define MAXCHAR		((char)0x7f)
#define MAXSHORT	((short)0x7fff)

// Max pos 32-bit int.
#define MAXINT		((int)0x7fffffff)
#define MAXLONG		((long)0x7fffffff)
#define MINCHAR		((char)0x80)
#define MINSHORT	((short)0x8000)

// Max negative 32-bit integer.
#define MININT		((int)0x80000000)
#define MINLONG		((long)0x80000000)
#endif

/* Type to use where the original version used short. */
#ifndef USE_SHORTS_ONLY

typedef int dshort_t;
typedef unsigned int dushort_t;
#define MAXDSHORT	MAXINT
#define MAXDUSHORT	0xffffffff

#else

typedef short dshort_t;
typedef unsigned short dushort_t;
#define MAXDSHORT	MAXSHORT
#define MAXDUSHORT	0xffff

#endif

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#ifdef _64_BIT_PTRS
typedef unsigned long long pint;
#else
typedef unsigned int pint;
#endif

#ifdef BOOM
#define BOOMSTATEMENT(stmt) stmt
#else
#define BOOMSTATEMENT(stmt)
#endif


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
