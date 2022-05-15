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
//	Lookup tables.
//	Do not try to look them up :-).
//	In the order of appearance:
//
//	int finetangent[4096]	- Tangens LUT.
//	 Should work with BAM fairly well (12 of 16bit,
//      effectively, by shifting).
//
//	int finesine[10240]		- Sine lookup.
//	 Guess what, serves as cosine, too.
//	 Remarkable thing is, how to use BAMs with this?
//
//	int tantoangle[2049]	- ArcTan LUT,
//	  maps tan(angle) to angle fast. Gotta search.
//
//-----------------------------------------------------------------------------


#ifndef __TABLES__
#define __TABLES__



#ifdef LINUX
#include <math.h>
#else
#define PI			3.141592657
#endif


#include "m_fixed.h"

#define FINEANGLES		8192
#define FINEMASK		(FINEANGLES-1)


// 0x100000000 to 0x2000
#define ANGLETOFINESHIFT	19

// Effective size is 10240.
extern const fixed_t		finesine[5*FINEANGLES/4];

// Re-use data, is just PI/2 phase shift.
extern const fixed_t*		finecosine;


// Effective size is 4096.
extern const fixed_t		finetangent[FINEANGLES/2];

// Binary Angle Measument, BAM.
#define ANG1			(ANG45 / 45)
#define ANG5			(ANG90 / 18)
#define ANG45			0x20000000
#define ANG90			0x40000000
#define ANG180			0x80000000
#define ANG270			0xc0000000
#define ANGLE_MAX           0xFFFFFFFF


#define SLOPERANGE		2048
#define SLOPEBITS		11
#define DBITS			(FRACBITS-SLOPEBITS)

typedef unsigned angle_t;


// Effective size is 2049;
// The +1 size is to handle the case when x==y
//  without additional checking.
extern const angle_t		tantoangle[SLOPERANGE+1];

// MBF21: More utility functions, courtesy of Quasar (James Haley).
// These are straight from Eternity so demos stay in sync.
static inline angle_t FixedToAngle(fixed_t a)
{
    return (angle_t)(((uint64_t)a * ANG1) >> FRACBITS);
}

static inline fixed_t AngleToFixed(angle_t a)
{
    return (fixed_t)(((uint64_t)a << FRACBITS) / ANG1);
}

// [XA] Clamped angle->slope, for convenience
static inline fixed_t AngleToSlope(int a)
{
    return finetangent[(a > ANG90 ? 0 : (-a > ANG90 ? FINEANGLES / 2 - 1 : (ANG90 - a) >> ANGLETOFINESHIFT))];
}

// [XA] Ditto, using fixed-point-degrees input
static inline fixed_t DegToSlope(fixed_t a)
{
    return AngleToSlope(a >= 0 ? FixedToAngle(a) : -(int)FixedToAngle(-a));
}

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
